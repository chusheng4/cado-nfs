#include "cado.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <gmp.h>
#include "bwc_config.h"
#include "portability.h"
#include "macros.h"
#include "utils.h"

/* the files below are not useful for the standalone program */
#ifndef WANT_MAIN
#include "parallelizing_info.h"
#include "random_matrix.h"
#include "balancing.h"
#endif

/* The random generation works as follows.
 *
 * We consider that for each coefficient of the matrix, the probability
 * of being non-zero is independent from the others, and given by a
 * probability distribution function which is scale/(i+offset)^alpha.
 *
 * In order to sample this, we approximate as follows.
 * For each row, the expectation of the row weight follows a binomial
 * distribution. We approximate it as a poisson distribution. Then, given
 * the count for the number of non-zero coefficients in the row, and
 * knowing the cumulative distribution function from above, we do inverse
 * transform sampling to find the location of the non-zero coefficients.
 *
 */

/*{{{ random picking */
double random_uniform(gmp_randstate_t rstate)
{
    /* the constant is 2^-53 */
    // return gmp_urandomm_ui(rstate, 1UL<<53) * 1.11022302462515654042363166809E-16;
    return gmp_urandomm_ui(rstate, ULONG_MAX) / (double) ULONG_MAX;
    /*
    mpf_t x;
    mpf_init2(x, 53);
    mpf_urandomb(x, rstate, 53);
    double y = mpf_get_d(x);
    mpf_clear(x);
    return y;
    */
}

double random_normal_standard(gmp_randstate_t rstate)
{
    /* Box-Muller transform */
#ifdef  ALLOW_NON_REENTRANT_random_normal_standard
    static int last = 0;
    static double vlast = 0;
    if (last) { --last; return vlast; }
#endif
    double u = random_uniform(rstate);
    double v = random_uniform(rstate);
    double rho = sqrt(-2*log(u));
    double theta = 2*M_PI*v;
    double x = rho*cos(theta);
#ifdef ALLOW_NON_REENTRANT_random_normal_standard
    double y = rho*sin(theta);
    vlast=y;
    last++;
#endif
    return x;
}

double random_normal(gmp_randstate_t rstate, double mean, double sdev)
{
    return mean + sdev * random_normal_standard(rstate);
}

/* return the expected maximum among n picks for a normal low with given
 * mean and sdev */
double extreme_normal(double n, double mean, double sdev)
{
    /* See Cramer, Mathematical methods of statistics, p. 575 (7th printing). */
    double x = sqrt(2*log(n))+(log(log(n))+log(4*M_PI)-2*0.5772)/(2*sqrt(2*log(n)));
    return mean + x * sdev;
}

double random_normal_constrained(gmp_randstate_t rstate, double mean, double sdev, double a, double b)
{
    /* By cutting the tail, we're doing a real heresy. This increases the
     * average and standard deviation significantly. See the function
     * below. In fact, the normal approximation will never be useful in
     * our case.
     */
    for(;;) {
        double x = round(random_normal(rstate, mean, sdev));
        if (x >= a && x < b) return x;
    }
}

/* given a probability mass function which gives the gaussian with mean
 * and sdev given my mx[0] and mx[1], but truncated to the interval
 * [a,b[, return the mean and sdev of the resulting distribution.
 *
 * This is just an illustration, which can be used to witness how the
 * normal approximation can end up being catastrophic if we're more
 * Poisson-like.
 */
void accuracy_of_normal_approximation_to_binomial(double * my, double *mx, unsigned long a, unsigned long b)
{
    /* Let e(x) = 1/sqrt(2*pi)*exp(-x^2/2).
     * We have:
     *  e'(x) = -x e(x).
     * \int_{-\infty}^{+\infty} e(x) = 1
     * \int_{-\infty}^{+\infty} xe(x) = 0
     * \int_{-\infty}^{+\infty} x^2e(x) = 1
     *
     * and more generally:
     * \int_a^b e(x) = 1/2*(erf(b/sqrt(2))-erf(a/sqrt(2))) = S0(a,b)
     * \int_a^b xe(x) = e(a) - e(b)                        = S1(a,b)
     * \int_a^b x^2e(x) = a*e(a)-b*e(b)+S0(a,b)            = S2(a,b)
     *
     * Let now e*(x) = 1/s*e((x-m)/s), pdf of a gaussian with mean and sdev
     * equal to m and s. Let x*=(x-m)/s, a*=(a-m)/s, and b*=(b-m)/s.
     * So that dx = s d{x*} ; note that e*(x)dx = e(x*)d{x*}.
     *
     * We have:
     * M0(a,b) = \int_a^b e*(x) dx
     *         = \int_{a*}^{b*}e(x*)d{x*}
     *         = S0(a*,b*)
     * M1(a,b) = \int_a^b x e*(x) dx
     *         = \int_{a*}^{b*} (m+s*x*) e(x*)d{x*}
     *         = m*S0(a*,b*) + s*S1(a*,b*)
     * M2(a,b) = \int_a^b x^2 e*(x) dx
     *         = \int_{a*}^{b*} (m+s*x*)^2 e(x*) d{x*}
     *         = m^2*S0(a*,b*) + 2*m*s*S1(a*,b*) + s^2*S2(a*,b*)
     * 
     * when scaled, we get:
     *
     * M0 = 1
     * M1 = m + s * (S1/S0)(a*,b*)
     * M2 = m^2 + 2*m*s * (S1/S0)(a*,b*) + s^2 * (S2/S0)(a*,b*)
     * sdev = s * sqrt(((S2-S1^2)/S0)(a*,b*))
     */
    double m = mx[0];
    double s = mx[1];
    double as = (a-m)/s;
    double bs = (b-m)/s;
    double eas = exp(-as*as/2)/sqrt(2*M_PI);
    double ebs = exp(-bs*bs/2)/sqrt(2*M_PI);
    double S0 = (erf(bs/sqrt(2))-erf(as/sqrt(2)))/2;
    double S1 = eas - ebs;
    double S2 = as*eas - bs*ebs + S0;
    /*
       double M0 = s * S0;
       double M1 = s * (m*S0 + s*S1);
       double M2 = s * (m^2*S0 + 2*m*s*S1 + s^2*S2);
       */
    // double M0 = 1;
    // double M1 = (m + s*S1/S0);
    // double M2 = (m^2 + 2*m*s*S1/S0 + s^2*S2/S0);
    my[0] = m + s * S1/S0;
    my[1] = s * sqrt((S2-S1*S1)/S0);
}

double random_poisson(gmp_randstate_t rstate, double lambda)
{
    /* "method PA" from "The Computer Generation of Poisson Random
     * Variables" by A. C. Atkinson, Journal of the Royal Statistical
     * Society Series C (Applied Statistics) Vol. 28, No. 1. (1979),
     * pages 29-35.
     */
    if (lambda < 10) {
        return random_uniform(rstate)*2*lambda;
    }
    double c = 0.767 - 3.36/lambda;
    double beta = M_PI/sqrt(3.0*lambda);
    double alpha = beta*lambda;
    double k = log(c) - lambda - log(beta);

    for(;;) {
        double u = random_uniform(rstate);
        double x = (alpha - log((1.0 - u)/u))/beta;
        int n = (int) floor(x + 0.5);
        if (n < 0)
            continue;
        double v = random_uniform(rstate);
        double y = alpha - beta*x;
        double temp = 1.0 + exp(y);
        double lhs = y + log(v/(temp*temp));
        double rhs = k + n*log(lambda) - lgamma(n-1);
        if (lhs <= rhs)
            return n;
    }
}

/* This is the random variable associated to the *size* of the sample */
double random_binomial(gmp_randstate_t rstate, unsigned long n, double p)
{
    /*
     * This first way of doing things is appropriate when mean \pm 3
     * times sdev is good.
     */
    double mean = n * p;
    double sdev = sqrt(n * p * (1-p));
    if (0 <= mean - 3 * sdev && mean + 3 * sdev <= n) {
        return random_normal_constrained(rstate, mean, sdev, 0, n);
    }
    /* otherwise we'll return the Poisson approximation, which does not
     * care much about the standard deviation, but matches relatively
     * well as far as our application is concerned. */

    double r;
    for( ; (r = random_poisson(rstate, mean)) >= n ; ) ;
    return r;
}
/*}}}*/

/* {{{ random_matrix_ddata type -- characteristics of the distribution */
struct random_matrix_ddata_s {
    double alpha;
    double offset;         /* this controls the peakedness for the leftmost
                           columns. It is difficult to make this much
                           smaller than 32 presently. Quite unsafe to
                           change. */
    double scale;       /* event function is scale/(x+offset)^alpha */
    double mean;        /* computed */
    double sdev;        /* computed */
    double spread;
    unsigned long ncols;        /* only for constraint correction */
    unsigned long nrows;        /* informational */

    uint64_t total_coeffs;   /* informational, after generation */
    double row_avg;     /* informational, after generation */
    double row_sdev;    /* informational, after generation */
};
typedef struct random_matrix_ddata_s random_matrix_ddata[1];
typedef struct random_matrix_ddata_s * random_matrix_ddata_ptr;

void random_matrix_ddata_init(random_matrix_ddata_ptr d);
void random_matrix_ddata_set_default(random_matrix_ddata_ptr d);
void random_matrix_ddata_clear(random_matrix_ddata_ptr d);
void random_matrix_ddata_adjust(random_matrix_ddata_ptr f, unsigned long nrows, unsigned long ncols, double density, double spread);
void random_matrix_ddata_info(FILE * out, random_matrix_ddata_ptr f);
void random_matrix_ddata_init(random_matrix_ddata_ptr F);
void random_matrix_ddata_clear(random_matrix_ddata_ptr F);
/* }}} */

/* the probability mass function for value i is scale/(i+offset)^alpha.
 * the cumulative distribution function (sum on [0,j[ ) is thus:
 * we'll do inverse transform sampling for computing position of non-zero
 * coefficients.
 *
 * scale * ((j+offset)^beta - offset^beta)/beta
 *
 * with beta = 1-alpha
 *
 * the reverse cumulative function is:
 *
 * (x*beta/scale+offset^beta)^(1/beta)-offset
 *
 * the variance is \sum p(i) - \sum p(i)^2  [proof left to reader]
 *
 * and based on this, the last term is:
 *
 * scale^2*((j+offset)^gamma- offset^gamma)/gamma
 *
 * with gamma = 1-2*alpha
 */

/* {{{ direct queries on the random_matrix_ddata type */

/* probability mass function */
double dist_p(random_matrix_ddata_ptr f, double x)
{
    return f->alpha<=0 ? f->scale : f->scale*pow(x*f->spread+f->offset,-f->alpha);
}

/* cumulative distribution function */
double dist_q(random_matrix_ddata_ptr f, double x)
{
    if (f->alpha <= 0) {
        return x * f->scale;
    }
    double beta = 1 - f->alpha;
    double u = f->scale / beta / f->spread;
    return u * (pow(x*f->spread + f->offset, beta) - pow(f->offset, beta));
}

/* reciprocal of the cumulative distribution function */
double dist_qrev(random_matrix_ddata_ptr f, double y)
{
    if (f->alpha <= 0) {
        return y / f->scale;
    }
    double beta = 1 - f->alpha;
    double u = f->scale / beta / f->spread;
    double r = pow(y / u + pow(f->offset, beta), 1 / beta) - f->offset;
    return r;
}

/* variance for the count of successes */
double dist_qq(random_matrix_ddata_ptr f, double x)
{
    if (f->alpha < 0) {
        /* don't need it */
        abort();
    }
    double gamma = 1 - 2 * f->alpha;
    double v = f->scale * f->scale / gamma / f->spread;
    return v * (pow(x + f->offset, gamma) - pow(f->offset, gamma));
}
/* }}} */

/* {{{ more random_matrix_ddata things */
void random_matrix_ddata_init(random_matrix_ddata_ptr F)
{
    memset(F, 0, sizeof(*F));
    F->scale = 1;
    F->spread = 1;
}
void random_matrix_ddata_clear(random_matrix_ddata_ptr F MAYBE_UNUSED)
{
}

void random_matrix_ddata_set_default(random_matrix_ddata_ptr F)
{
    F->alpha = 0.94;
    F->offset = 32;
    F->scale = 1;
    F->spread = 1;
}

void random_matrix_ddata_info(FILE * out, random_matrix_ddata_ptr f)
{
    unsigned long nrows = f->nrows;
    unsigned long ncols = f->ncols;
    /* some checking and info */
    double p0 = dist_p(f, 0);
    double mean0 = nrows * p0;
    double sdev0 = sqrt(nrows * p0 * (1-p0));
    double pn = dist_p(f, ncols-1);
    double mean_n = nrows * pn;
    double sdev_n = sqrt(nrows * pn * (1-pn));
    fprintf(out, "Expected row weight: %.3f, sdev %.3f\n", f->mean, f->sdev);
    fprintf(out, "Expected weight for first column is %.3f (sdev %.3f, m/sdev=%.1f)\n",
            mean0, sdev0, mean0 / sdev0);
    fprintf(out, "Expected weight for last column is %.3f (sdev %.3f, m/sdev=%.1f)\n",
            mean_n, sdev_n, mean_n / sdev_n);
    fprintf(out, "Worst-case expectation for last column weight by normal approximation: %.3f\n",
            extreme_normal(nrows, mean_n, -sdev_n));
}

void random_matrix_ddata_adjust(random_matrix_ddata_ptr f, unsigned long nrows, unsigned long ncols, double density, double spread)
{
    /* sets the scale parameter so that the expected row weight matches
     * our desired density target */
    f->scale = density / dist_q(f, ncols);
    f->spread = spread;
    f->mean = dist_q(f, ncols);
    f->sdev = sqrt(f->mean * f->mean - dist_qq(f, ncols));
    f->nrows = nrows;
    f->ncols = ncols;
    if (dist_p(f, 0) >= 1.0) {
        fprintf(stderr, "Error: this density is not acceptable for the current distribution equation. Please adjust the internal offset parameter to something larger.\nrows");
        exit(1);
    }
}
/* }}} */

/* {{{ punch intervals */
struct punched_interval_s;
struct punched_interval_s {
    double b0, b1;
    double holes;
    int has_left, has_right;
    /* free blocks use the "left" pointer below for the next argument in
     * the free list */
    struct punched_interval_s * left;
    struct punched_interval_s * right;
};
typedef struct punched_interval_s * punched_interval_ptr;

void punched_interval_free(punched_interval_ptr c, punched_interval_ptr * pool)
{
    if (!c) return;
    /* enqueue both children to the free pool */
    punched_interval_free(c->left, pool);
    punched_interval_free(c->right, pool);
    c->left = *pool;
    /* also store the count */
    c->has_left = 1 + ((*pool) ? (*pool)->has_left : 0);
    *pool = c;
}

void punched_interval_set_full(punched_interval_ptr x, double b0, double b1)
{
    x->b0 = b0;
    x->b1 = b1;
    x->has_left = 0;
    x->has_right = 0;
    x->holes = 0;
}

punched_interval_ptr punched_interval_alloc(punched_interval_ptr * pool, double b0, double b1)
{
    punched_interval_ptr x;
    if (*pool) {
        x = *pool;
        *pool = x->left;
    } else {
        x = malloc(sizeof(struct punched_interval_s));
    }
    memset(x, 0, sizeof(struct punched_interval_s));
    punched_interval_set_full(x, b0, b1);
    return x;
}

void punched_interval_free_pool(punched_interval_ptr * pool)
{
    for(punched_interval_ptr q = *pool, v ; q ; q = v) {
        v = q->left;
        free(q);
    }
    *pool = NULL;
}

void punched_interval_pre_free_pool(punched_interval_ptr * pool, int max, int print)
{
    if (!*pool) return;
    if ((*pool)->has_left < 2 * max) return;
    if (print) {
        fprintf(stderr, "Reducing punched_interval pool from size %d to %d\n",
                (*pool)->has_left, max);
    }
    punched_interval_ptr q = * pool;
    int size = (*pool)->has_left;
    for(int i = 0 ; q->has_left >= max ; i++) {
        ASSERT_ALWAYS(q->left);
        ASSERT_ALWAYS(q->has_left == size - i);
        punched_interval_ptr nq = q->left;
        free(q);
        q = nq;
    }
    *pool = q;
}


void punched_interval_punch(punched_interval_ptr * pool, punched_interval_ptr c, double x0, double x1)
{
    c->holes += x1 - x0;
    if (!c->left) {
        c->left = punched_interval_alloc(pool, c->b0, x0);
    } else {
        punched_interval_set_full(c->left, c->b0, x0);
    }
    c->has_left=1;
    if (!c->right) {
        c->right = punched_interval_alloc(pool, x1, c->b1);
    } else {
        punched_interval_set_full(c->right, x1, c->b1);
    }
}


unsigned long pick_and_punch(random_matrix_ddata_ptr f, punched_interval_ptr * pool, punched_interval_ptr c, double x)
{
    /* x should be within [c->b0, c->b1 - c->holes] */
    ASSERT_ALWAYS(x >= c->b0);
    ASSERT_ALWAYS(x + c->holes < c->b1);
    if (!c->has_left) {
        /* no holes ! */
        double r = dist_qrev(f, x);
        unsigned long i;
        if (r < 0) {
            i = 0;
        } else if (r >= f->ncols) {
            i = f->ncols - 1;
        } else {
            i = floor(r);
        }
        double x0 = dist_q(f, i);
        double x1 = dist_q(f, i + 1);
        punched_interval_punch(pool, c, x0, x1);
        return i;
    }
    /* try to correct x with all left holes */
    double xc = x + c->left->holes;
    if (xc < c->left->b1) {
        double h = c->left->holes;
        unsigned long i = pick_and_punch(f, pool, c->left, x);
        c->holes += c->left->holes - h;
        return i;
    } else {
        /* modify x. It's more than just xc ! */
        xc += c->right->b0 - c->left->b1;
        double h = c->right->holes;
        unsigned long i = pick_and_punch(f, pool, c->right, xc);
        c->holes += c->right->holes - h;
        return i;
    }
}

/*
void punched_interval_print_rec(FILE * f, punched_interval_ptr c)
{
    if (!c->has_left) return;
    punched_interval_print_rec(f, c->left);
    fprintf(f, "(\e[31m%.2f...%.2f\e[0m)...", c->left->b1, c->right->b0);
    punched_interval_print_rec(f, c->right);
}

void punched_interval_print(FILE * f, punched_interval_ptr c)
{
    fprintf(f, "%.2f...", c->b0);
    punched_interval_print_rec(f, c);
    fprintf(f, "%.2f\n", c->b1);
}
   */

/* }}} */



typedef int (*sortfunc_t)(const void *, const void *);

int cmp_u32(uint32_t * a, uint32_t * b)
{
    return (*a > *b) - (*b > *a);
}

uint32_t generate_row(gmp_randstate_t rstate, random_matrix_ddata_ptr f, uint32_t * ptr, punched_interval_ptr range, punched_interval_ptr * pool)
{
    /* pick a row weight */
    /*
       unsigned long weight = random_normal_constrained(rstate, f->mean, f->sdev, 0, f->ncols);
       */
    uint32_t weight;
    for( ; (weight = random_poisson(rstate, f->mean)) >= f->ncols ; );
    // punched_interval_ptr range = punched_interval_alloc(0, f->mean);
    punched_interval_set_full(range, 0, f->mean);
    for(uint32_t i = 0 ; i < weight ; i++) {
        // punched_interval_print(stdout, range);
        double x = random_uniform(rstate) * (range->b1 - range->holes);
        uint32_t k = pick_and_punch(f, pool, range, x);
        ptr[i] = k;
    }
    qsort(ptr, weight, sizeof(uint32_t), (sortfunc_t) &cmp_u32);
    // punched_interval_free(range);
    return weight;
}


/* {{{ Some tests */
void test_random_normal_standard(gmp_randstate_t rstate)
{
    double s = 0, ss = 0;
    for(int i = 0, l = 10 ; l <= 10000000 ; l *= 10) {
        for(  ; i < l ; i++) {
            double x = random_normal_standard(rstate);
            s += x;
            ss += x * x;
        }
        double m = s / l;
        fprintf(stderr, "after %d picks, mean=%.3f sdev=%.3f\n",
                l, s / l, sqrt(ss/l - m*m));
    }
}

void test_random_normal(gmp_randstate_t rstate, double xm, double xs)
{
    double s = 0, ss = 0;
    for(int i = 0, l = 10 ; l <= 10000000 ; l *= 10) {
        for(  ; i < l ; i++) {
            double x = random_normal(rstate, xm, xs);
            s += x;
            ss += x * x;
        }
        double m = s / l;
        fprintf(stderr, "after %d picks, mean=%.3f sdev=%.3f\n",
                l, s / l, sqrt(ss/l - m*m));
    }
}

void test_random_normal_constrained(gmp_randstate_t rstate, double xm, double xs, unsigned long a, unsigned long b)
{
    double s = 0, ss = 0;
    double mmx[2]={xm, xs}, mmy[2];
    accuracy_of_normal_approximation_to_binomial(mmy, mmx, a, b);
    fprintf(stderr, "want (%.3f,%.3f), expect instead (%.3f,%.3f)\n",
            mmx[0], mmx[1],
            mmy[0], mmy[1]);
    for(int i = 0, l = 10 ; l <= 100000 ; l *= 10) {
        for(  ; i < l ; i++) {
            double x = random_normal_constrained(rstate, xm, xs, a, b);
            s += x;
            ss += x * x;
        }
        double m = s / l;
        fprintf(stderr, "after %d picks, mean=%.3f sdev=%.3f\n",
                l, s / l, sqrt(ss/l - m*m));
    }
}

void test_random_poisson(gmp_randstate_t rstate, double xm, unsigned long n)
{
    double s = 0, ss = 0;
    for(int i = 0, l = 10 ; l <= 100000 ; l *= 10) {
        for(  ; i < l ; i++) {
            unsigned long x;
            for( ; (x = random_poisson(rstate, xm)) >= n ; ) ;
            s += x;
            ss += x * x;
        }
        double m = s / l;
        fprintf(stderr, "after %d picks, mean=%.3f sdev=%.3f\n",
                l, s / l, sqrt(ss/l - m*m));
    }
}

// test_random_normal_standard(rstate);
// test_random_normal(rstate, F->mean, F->sdev);
// test_random_normal_constrained(rstate, F->mean, F->sdev, 0, ULONG_MAX);//F->ncols);
// test_random_poisson(rstate,  F->mean, F->ncols);

/* }}} */

/* {{{ random_matrix_process_data */
/* This data type gathers the internal state of the random generation */
struct random_matrix_process_data_s {
    unsigned long nrows;
    unsigned long ncols;
    int density;
    unsigned long seed;
    int maxcoeff;
    int ascii;
    FILE * out;
    struct rhs_data {
        int n;
        mpz_t p;
        FILE * f;
    } rhs[1];
    struct freq_data {
        FILE * cw;
        FILE * rw;
    } freq[1];
};
typedef struct random_matrix_process_data_s random_matrix_process_data[1];
typedef struct random_matrix_process_data_s * random_matrix_process_data_ptr;
typedef const struct random_matrix_process_data_s * random_matrix_process_data_srcptr;

void random_matrix_process_data_init(random_matrix_process_data_ptr r)
{
    memset(r, 0, sizeof(*r));
}

void random_matrix_process_data_clear(random_matrix_process_data_ptr r)
{
    if (r->rhs->n) {
        fclose(r->rhs->f);
        mpz_clear(r->rhs->p);
    }
    if (r->freq->cw) fclose(r->freq->cw);
    if (r->freq->rw) fclose(r->freq->rw);
    if (r->out) fclose(r->out);
}

/* {{{ This reads the full parameter list -- not only the param_list
 * structure --, and fills r with all argument which has been found
 * relevant. This can primarily be seen as a function dedicated to the
 * standalone program, even though the random_matrix= mechanism uses it
 * too as a back-end.
 *
 * This function does *NOT* check that all arguments in pl have been
 * consumed.
 */
int random_matrix_process_data_set_from_args(random_matrix_process_data_ptr r,
        param_list_ptr pl, int argc, char ** argv)
{
    const char * tmp;
    int binary=0;
    int freq=0;
    param_list_configure_alias(pl, "output", "-o");
    param_list_configure_switch(pl, "--binary", &binary);
    param_list_configure_switch(pl, "--freq", &freq);
    int wild_args[3] = { 0, 0, 0 }; // nrows ncols coeffs_per_row
    int wild = 0;
    for( ; argc ; ) {
        if (param_list_update_cmdline(pl, &argc, &argv)) { continue; }
        if (argv[0][0] != '-' && wild < 3) {
            char * tmp;
            wild_args[wild] = strtoul(argv[0], &tmp, 0);
            if (*tmp != '\0') {
                fprintf(stderr, "Parse error for parameter %s\n", argv[0]);
                exit(1);
            }
            argv++, argc--;
            wild++;
            continue;
        }
        fprintf(stderr, "Unhandled parameter %s\n", argv[0]);
        return 0;
    }
    /* {{{ parse r->nrows, r->ncols, density */
    if ((r->nrows = wild_args[0]) == 0) {
        fprintf(stderr, "Please specify r->nrows\n");
        exit(1);
    }
    r->ncols = wild_args[1];
    if (!(r->ncols = wild_args[1])) r->ncols = r->nrows;
    if (param_list_parse_int(pl, "density", &r->density)) {
        if (wild_args[2] > 0) {
            fprintf(stderr, "density specified twice\n");
            exit(1);
        }
    } else {
        if ((r->density = wild_args[2]) == 0)
            r->density = MIN(100, MAX(r->ncols / 10, MIN(4, r->ncols)));
    }
    ASSERT_ALWAYS(r->ncols > 10 && r->nrows > 10);
    /* }}} */

    param_list_parse_ulong(pl, "seed", &r->seed);
    if (!r->seed) r->seed = time(NULL);
    param_list_parse_int(pl, "c", &r->maxcoeff);

    r->ascii = !binary;

    /* {{{ try to parse the rhs info */
    if ((tmp = param_list_lookup_string(pl, "rhs")) != NULL) {
        ASSERT_ALWAYS(r->maxcoeff > 0);
        char * rhsname = malloc(1 + strlen(tmp));
        mpz_init(r->rhs->p);
        int rc = gmp_sscanf(tmp, "%d,%Zd,%s", &r->rhs->n, r->rhs->p, rhsname);
        ASSERT_ALWAYS(rc == 3);
        if (r->rhs->n == 0) {
            fprintf(stderr, "--rhs argument requires setting more than 0 vectors !\n");
            exit(1);
        }
        r->rhs->f = fopen(rhsname, "w");
        DIE_ERRNO_DIAG(r->rhs->f == NULL, "fopen", rhsname);
        // rhs file is now always in ascii
        // if (r->ascii)
            gmp_fprintf(r->rhs->f, "%lu %d %Zd\n", r->nrows, r->rhs->n, r->rhs->p);
    }
    /* }}} */

    r->out = stdout;

    const char * ofilename = NULL;

    if ((ofilename = param_list_lookup_string(pl, "output")) != NULL) {
        r->out = fopen(ofilename, binary ? "wb" : "w");
        DIE_ERRNO_DIAG(r->out == NULL, "fopen", ofilename);
    } else {
        if (binary) {
            fprintf(stderr, "Error: --binary requires --output\n");
            exit(1);
        }
        if (freq) {
            fprintf(stderr, "Error: --freq requires --output\n");
            exit(1);
        }
    }

    if (freq) {
        char * cwname = derived_filename(ofilename, "cw", binary ? ".bin" : ".txt");
        r->freq->cw = fopen(cwname, binary ? "wb" : "w");
        DIE_ERRNO_DIAG(r->freq->cw == NULL, "fopen", cwname);
        free(cwname);

        char * rwname = derived_filename(ofilename, "rw", binary ? ".bin" : ".txt");
        r->freq->rw = fopen(rwname, binary ? "wb" : "w");
        DIE_ERRNO_DIAG(r->freq->rw == NULL, "fopen", rwname);
        free(rwname);
    }

    return 1;
}
/* }}} */
/* {{{ This is primarily used for the random_matrix= hack. The standalone
 * program does not follow this route. Here we check that all parts of
 * the provided string are understood as legitimate arguments to
 * random_matrix=
 */
int random_matrix_process_data_set_from_string(random_matrix_process_data_ptr r, const char * str)
{
    char * rmstring;
    char ** n_argv;
    char ** n_argv0;
    int n_argc;
    param_list pl2;

    /* Create a new param_list from the random_matrix argument {{{ */
    ASSERT_ALWAYS(str);
    rmstring = strdup(str);

    n_argv0 = n_argv = malloc(strlen(rmstring) * sizeof(char*));
    n_argc = 0;
    for(char * q = rmstring, * qq; q != NULL; q = qq) {
        qq = strchr(q, ',');
        if (qq) { *qq++='\0'; }
        n_argv[n_argc++]=q;
    }
    /* }}} */
    param_list_init(pl2);
    int ok = random_matrix_process_data_set_from_args(r, pl2, n_argc, n_argv);
    if (!ok || param_list_warn_unused(pl2)) {
        fprintf(stderr, "Bad argument list for parameter random_matrix: %s\n", rmstring);
        exit(1);
    }
    if (!param_list_lookup_string(pl2, "output")) {
        /* the default is then that r->out == stdout, but for this very
         * usage we want no output at all, so r->out should be NULL. */
        r->out = NULL;
    }

    param_list_clear(pl2);
    free(rmstring);
    free(n_argv0);
    return ok;
}
/* }}} */
/* }}} */

#ifndef WANT_MAIN
void random_matrix_fill_fake_balancing_header(balancing_ptr bal, parallelizing_info_ptr pi, const char * rtmp)
{
    memset(bal->h, 0, sizeof(bal->h));
    random_matrix_process_data r;
    random_matrix_process_data_init(r);
    random_matrix_process_data_set_from_string(r, rtmp);
    bal->h->nh = pi->wr[1]->totalsize;
    bal->h->nv = pi->wr[0]->totalsize;
    bal->h->nrows = r->nrows;
    bal->h->ncols = r->ncols;
    bal->h->ncoeffs = 0; /* FIXME ; what should I do ? */
    bal->h->checksum = 0;
    bal->h->flags = FLAG_PADDING|FLAG_COLPERM|FLAG_SHUFFLED_MUL|FLAG_REPLICATE;
    bal->h->pshuf[0] = 1;
    bal->h->pshuf[1] = 0;
    bal->h->pshuf_inv[0] = 1;
    bal->h->pshuf_inv[1] = 0;
    balancing_set_row_col_count(bal);
    random_matrix_process_data_clear(r);
}

/*{{{ borrowed from balancing_workhorse.c*/
struct progress_info {
    time_t t;   /* last printed time */
    size_t z;   /* last printed data amount */
};

static int should_print_now(struct progress_info * last_printed, size_t z)
{
    if (z >= last_printed->z + (10UL<<20) || time(NULL) >= last_printed->t + 10) {
        last_printed->z = z;
        last_printed->t = time(NULL);
        return 1;
    }
    return 0;
}
/*}}}*/

int cmp_2u32(uint32_t * a, uint32_t * b)
{
    int r = (*a > *b) - (*b > *a);
    if (r) return r;
    a++;
    b++;
    return (*a > *b) - (*b > *a);
}

#if 0
/* This is totally dumb. */
uint32_t * matrix_transpose(uint32_t * p, size_t size, unsigned long nrows, unsigned long ncols)
{
    size_t ncoeffs = size - nrows;
    uint32_t * big = malloc(ncoeffs * 2 * sizeof(uint32_t));
    char buf[16];
    size_disp(ncoeffs * 2 * sizeof(uint32_t), buf);
    printf("allocating temp area of size %s\n", buf);
    uint32_t * q = p;
    uint32_t * w = big;
    uint32_t * fence = big + ncoeffs * 2;
    for(unsigned long i = 0 ; i < nrows ; i++) {
        unsigned long weight = *q++;
        for(unsigned long j = 0 ; j < weight ; j++) {
            /* we'll sort by column indices */
            *w++=*q++;
            *w++=i;
        }
    }
    ASSERT_ALWAYS(w == fence);

    free(p);
    time_t t0 = time(NULL);
    printf("now sorting\n");
    qsort(big, ncoeffs, 2 * sizeof(uint32_t), (sortfunc_t) &cmp_2u32);
    printf("sort time %d s\n", (int) (time(NULL)-t0));
    p = malloc((ncoeffs + ncols) * sizeof(uint32_t));
    q = p;
    w = big;
    for(unsigned long j = 0 ; j < ncols ; j++) {
        uint32_t * qq = q++;
        *qq = 0;
        for( ; w < fence && *w == j ; w += 2) {
            ++*qq;
            *q++ = w[1];
        }
        if (j == ncols-1) ASSERT_ALWAYS(w == fence);
    }
    free(big);
    return p;
}
#endif


void * random_matrix_get_u32(parallelizing_info_ptr pi, param_list pl, matrix_u32_ptr arg)
{
    const char * rtmp = param_list_lookup_string(pl, "random_matrix");
    ASSERT_ALWAYS(rtmp);

    unsigned long nrows;
    unsigned long ncols;
    unsigned long seed;
    int density;
    int maxcoeff;

    {
        random_matrix_process_data r;
        random_matrix_process_data_init(r);
        random_matrix_process_data_set_from_string(r, rtmp);
        nrows = r->nrows;
        ncols = r->ncols;
        density = r->density;
        maxcoeff = r->maxcoeff;
        /* This is not supported here -- mostly because we haven't been
         * extremely careful. */
        ASSERT_ALWAYS(!r->rhs->n);
        seed = r->seed + pi->m->jrank * pi->m->ncores + pi->m->trank;
        random_matrix_process_data_clear(r);
    }

    gmp_randstate_t rstate;
    gmp_randinit_default(rstate);

    nrows /= pi->wr[1]->totalsize;
    ncols /= pi->wr[0]->totalsize;
    density /= pi->wr[0]->totalsize;
    // F->offset /= pi->wr[0]->totalsize;
    gmp_randseed_ui(rstate, seed);


    random_matrix_ddata F;
    random_matrix_ddata_init(F);
    random_matrix_ddata_set_default(F);

    /* Adapt to the parallelizing_info structure : divide */
    /* note that padding has to still be padding. */

    random_matrix_ddata_adjust(F, nrows, ncols, density, pi->wr[0]->totalsize);


    /* Now we essentially have a copy of printrows above, except that
     * we're outputting binary, and not to a stream but to memory.  */
    uint64_t total_coeffs = 0;
    double tot_sq = 0;

    size_t alloc = 0;
    ASSERT_ALWAYS(arg->p == NULL);
    ASSERT_ALWAYS(arg->size == 0);

#define PUSH_P(x) do {    						\
        if (arg->size >= alloc) {					\
            alloc = arg->size + 64 + alloc / 4;			        \
            arg->p = realloc(arg->p, alloc * sizeof(uint32_t));	        \
        }								\
        arg->p[arg->size++] = (x);					\
    } while (0)

    if (pi->m->jrank == 0 && pi->m->trank == 0) {
        printf("Each of the %u jobs on %u nodes creates a matrix with %lu rows %lu cols, and %d coefficients per row on average. Seed for rank 0 is %lu.\n",
                pi->m->totalsize, pi->m->njobs, nrows, ncols, density, seed);
    }

    time_t t0 = time(NULL);
    struct progress_info last_printed[1];
    last_printed->t = t0;
    last_printed->z = 0;

    if (!arg->transpose) {
        uint32_t * ptr = malloc(ncols * sizeof(unsigned long));
        /* we'd like to avoid constant malloc()'s and free()'s */
        punched_interval_ptr pool = NULL;
        punched_interval_ptr range = punched_interval_alloc(&pool, 0, 1);
        for(unsigned long i = 0 ; i < nrows ; i++) {
            // long v = 0;
            uint32_t c = generate_row(rstate, F, ptr, range, & pool);
            PUSH_P(c);
            for(unsigned long j = 0 ; j < c ; j++) {
                PUSH_P(ptr[j]);
                if (maxcoeff) {
                    int co = gmp_urandomm_ui(rstate, 2 * maxcoeff + 1) - maxcoeff;
                    PUSH_P(co);
                    // if (r->rhs->n) v += co * (1+ptr[j]);
                }
            }
            total_coeffs += c;
            tot_sq += (double) c * (double) c;
            if (pi->m->jrank == 0 && pi->m->trank == 0 && should_print_now(last_printed, arg->size * sizeof(uint32_t))) {
                double dt = last_printed->t - t0;
                char buf[16];
                char buf2[16];
                printf("%s, %lu rows in %d s ; %s/s  \n",
                        size_disp(arg->size * sizeof(uint32_t), buf), i, (int) dt,
                        size_disp(dt > 0 ? (size_t) (arg->size * sizeof(uint32_t) / dt) : 0, buf2));
                fflush(stdout);
            }
        }
        if (pi->m->jrank == 0 && pi->m->trank == 0) {
            printf("\n");
        }
        punched_interval_free(range, &pool);
        punched_interval_free_pool(&pool);
        free(ptr);
    } else {
        size_t size0 = 0;
        uint32_t * ptr = malloc(ncols * sizeof(unsigned long));
        random_matrix_ddata G;
        random_matrix_ddata_init(G);
        /* use a special ddata, for our specially simple process (which
         * still needs the pick-and-punch thing */
        G->alpha=0;
        G->ncols = nrows; /* yes */
        /* Then in fact it's easier, as we can avoid inverse transform
         * sampling for the computation of the coefficients */
        // int heavy = 1;
        punched_interval_ptr pool = NULL;
        for(unsigned long j = 0 ; j < ncols ; j++) {
            double p = dist_p(F, j);
            G->scale = p;
            unsigned long weight;
            if (p > 0.1) {
                weight = 0;
                for(unsigned long i = 0 ; i < nrows ; i++) {
                    if (random_uniform(rstate) < p)
                        ptr[weight++]=i;
                }
            } else {
                weight = random_binomial(rstate, nrows, p);
                for(unsigned long i = 0 ; i < weight ; i++) {
                    ptr[i] = gmp_urandomm_ui(rstate, nrows);
                }
                qsort(ptr, weight, sizeof(uint32_t), (sortfunc_t) &cmp_u32);
                unsigned long nw = 0;
                for(unsigned long i = 0, j ; i < weight ; i=j) {
                    for(j = i + 1; j < weight && ptr[i] == ptr[j] ; j++) ;
                    ptr[nw++] = ptr[i];
                }
                weight = nw;
#if 0
            double wmean = nrows * p; 
            // double wsdev = sqrt(nrows * p * (1-p));
            unsigned long weight = random_binomial(rstate, nrows, p);
            } else if (heavy && weight < sqrt(0.1 * 2 * nrows)) {

            /* pick uniformly a subset of exactly [weight] row
             * indices, within [0..nrows[.  */
                if (pi->m->jrank == 0 && pi->m->trank == 0) {
                    printf("from now on, replacing pick_and_punch by accept-reject\n");
                }
                heavy = 0;
                size0 = arg->size;
                t0 = time(NULL);
            }
            if (heavy) {
                punched_interval_ptr range = punched_interval_alloc(&pool, 0, 1);
                punched_interval_set_full(range, 0, wmean);
                for(unsigned long i = 0 ; i < weight ; i++) {
                    // punched_interval_print(stdout, range);
                    double x = random_uniform(rstate) * (range->b1 - range->holes);
                    unsigned long k = pick_and_punch(G, &pool, range, x);
                    ptr[i] = k;
                }
                punched_interval_free(range, &pool);
                punched_interval_pre_free_pool(&pool, 2 * weight,
                        pi->m->jrank == 0 && pi->m->trank == 0);
                qsort(ptr, weight, sizeof(uint32_t), (sortfunc_t) &cmp_u32);
            } else {
                for(int ok = 0 ; !ok ; ) {
                    for(unsigned long i = 0 ; i < weight ; i++) {
                        ptr[i] = gmp_urandomm_ui(rstate, nrows);
                    }
                    qsort(ptr, weight, sizeof(uint32_t), (sortfunc_t) &cmp_u32);
                    ok=1;
                    for(unsigned long i = 1 ; i < weight ; i++) {
                        if (ptr[i] == ptr[i-1]) {
                            ok=0;
                            break;
                        }
                    }
                }
#endif
            }
            PUSH_P(weight);
            for(unsigned long i = 0 ; i < weight ; i++) {
                PUSH_P(ptr[i]);
            }
            total_coeffs += weight;
            tot_sq += (double) weight * (double) weight;
            if (pi->m->jrank == 0 && pi->m->trank == 0 && should_print_now(last_printed, arg->size * sizeof(uint32_t))) {
                double dt = last_printed->t - t0;
                char buf[16];
                char buf2[16];
                printf("%s, %lu cols in %d s ; %s/s (last weight: %lu) \n",
                        size_disp(arg->size * sizeof(uint32_t), buf), j, (int) dt,
                        size_disp(dt > 0 ? (size_t) ((arg->size-size0) * sizeof(uint32_t) / dt) : 0, buf2), weight);
                fflush(stdout);
            }
        }
        punched_interval_free_pool(&pool);
        random_matrix_ddata_clear(G);
        free(ptr);
    }
#undef PUSH_P
    F->total_coeffs = total_coeffs;
    double e = (double) total_coeffs / nrows;
    double s = (double) tot_sq / nrows;
    double sdev = sqrt(s - e*e);
    F->row_avg = e;
    F->row_sdev = sdev;
    if (verbose_enabled(CADO_VERBOSE_PRINT_BWC_CACHE_BUILD) && pi->m->jrank == 0 && pi->m->trank == 0) {
        fprintf(stderr, "Actual density per row avg %.2f sdev %.2f\n",
                F->row_avg, F->row_sdev);
    }


    gmp_randclear(rstate);
    random_matrix_ddata_clear(F);
    return arg;
}
#endif

#ifdef  WANT_MAIN

void random_matrix_process_print(random_matrix_process_data_ptr r, random_matrix_ddata_ptr F)
{
    int ascii = r->ascii;
    FILE * out = r->out;
    ASSERT_ALWAYS(out);
    gmp_randstate_t rstate;
    gmp_randinit_default(rstate);
    gmp_randseed_ui(rstate, r->seed);
    uint32_t * colweights = NULL;
    if (r->freq->cw) {
        colweights = malloc(r->ncols * sizeof(uint32_t));
        memset(colweights, 0, r->ncols * sizeof(uint32_t));
    }


#define WU32(out, pre, x, post) do {					\
        if (ascii) {							\
            fprintf(out, pre "%" PRIu32 post, (x));			\
        } else {							\
            fwrite(&(x), sizeof(uint32_t), 1, out);			\
        }								\
    } while (0)

#define WS32(out, pre, x, post) do {                                    \
        if (ascii) {							\
            fprintf(out, pre "%" PRId32 post, (x));			\
        } else {							\
            fwrite(&(x), sizeof(uint32_t), 1, out);			\
        }								\
    } while (0)

#define WZa(out, pre, x, post, p) do {					\
            gmp_fprintf(out, pre "%Zd" post, x);			\
    } while (0)


    if (ascii)
        fprintf(out, "%lu %lu\n", r->nrows, r->ncols);
    uint32_t * ptr = malloc(r->ncols * sizeof(uint32_t));
    uint64_t total_coeffs = 0;
    double tot_sq = 0;
    punched_interval_ptr pool = NULL;
    punched_interval_ptr range = punched_interval_alloc(&pool, 0, 1);
    for(unsigned long i = 0 ; i < r->nrows ; i++) {
        long v = 0;
        uint32_t c = generate_row(rstate, F, ptr, range, &pool);
        WU32(out, "", c, "");
        if (r->freq->rw) {
            WU32(r->freq->rw, "", c, "\n");
        }
        for(uint32_t j = 0 ; j < c ; j++) {
            WU32(out, " ", ptr[j], "");
            if (colweights) colweights[ptr[j]]++;
            if (r->maxcoeff) {
                int32_t co = gmp_urandomm_ui(rstate, 2 * r->maxcoeff + 1) - r->maxcoeff;
                WS32(out, ":", co, "");
                if (r->rhs->n) v += (long) co * (long) (1+ptr[j]);
            }
        }
        if (ascii) { fprintf(out, "\n"); }
        if (r->rhs->n) {
            mpz_t x,s;
            mpz_init(x);
            mpz_init(s);
            mpz_set_si(s, v);

            for(int j = 0 ; j < r->rhs->n - 1 ; j++) {
                mpz_urandomm(x, rstate, r->rhs->p);
                mpz_addmul_ui(s, x, r->ncols + j + 1);
                WZa(r->rhs->f, "", x, " ", r->rhs->p);
            }
            mpz_set_si(x, -(r->ncols + r->rhs->n));
            mpz_invert(x, x, r->rhs->p);
            mpz_mul(s, s, x);
            mpz_mod(s, s, r->rhs->p);
            WZa(r->rhs->f, "", s, "\n", r->rhs->p);
            mpz_clear(x);
            mpz_clear(s);
        }

        total_coeffs += c;
        tot_sq += (double) c * (double) c;
    }
    /* FIXME -- what the hell ? r->nrows is the full length anyway...
    for(unsigned long i = 0 ; i < kernel_right ; i++) {
        if (ascii) {
            fprintf(out, "0\n");
        } else {
            const uint32_t c = 0;
            fwrite(&c, sizeof(uint32_t), 1, out);
        }
    }
    */
    if (colweights) {
        if (ascii) {
            for(unsigned long j = 0 ; j < r->ncols ; j++) {
                WU32(r->freq->cw, "", colweights[j], "\n");
            }
        } else {
            fwrite(colweights, sizeof(uint32_t), r->ncols, r->freq->cw);
        }
        free(colweights);
    }
    punched_interval_free(range, &pool);
    punched_interval_free_pool(&pool);
    free(ptr);
    F->total_coeffs = total_coeffs;
    double e = (double) total_coeffs / r->nrows;
    double s = (double) tot_sq / r->nrows;
    double sdev = sqrt(s - e*e);
    F->row_avg = e;
    F->row_sdev = sdev;

    gmp_randclear(rstate);
}

void usage()
{
    fprintf(stderr, "Usage: ./random <nrows> [<ncols>] [<density>] [options]\n"
            "Options:\n"
            "\t-d <density> : desired density per row\n"
            "\t-s <seed> : seed\n"
            "\t-c <maxc> : add coefficients\n"
            "\t-v : turn verbosity on\n"
            "\t-o <matfile> : output file name\n"
            "\t--binary : output in binary\n"
            "\t--kleft <d>: ensure at least a left kernel of dimension d\n"
            "\t--kright <d>: ditto for right kernel\n"
            "\t--rhs <nrhs>,<prime>,<filename>: rhs output\n"
           );
    exit(1);
}

int main(int argc, char * argv[])
{
    param_list pl;
    int verbose = 0;
    unsigned long kernel_left = 0;
    unsigned long kernel_right = 0;
    random_matrix_process_data r;


    argv++, argc--;
    param_list_init(pl);
    param_list_configure_alias(pl, "density", "-d");
    param_list_configure_alias(pl, "seed", "-s");
    param_list_configure_switch(pl, "-v", &verbose);

    random_matrix_process_data_init(r);
    if (!random_matrix_process_data_set_from_args(r, pl, argc, argv))
        usage();

    /* {{{ parse kernel size. default is to make the matrix invertible */
    param_list_parse_ulong(pl, "kleft", &kernel_left);
    param_list_parse_ulong(pl, "kright", &kernel_right);

    /* we've been given dimensions for the target matrix, together with a
     * constraint on the kernel size (to be understood as "at least that
     * large").
     *
     * given the row/col unbalance, we may or may not have to generate
     * fewer rows/cols.
     */
    if (r->ncols > r->nrows) {
        if (kernel_right >= r->ncols - r->nrows) {
            kernel_right -= r->ncols - r->nrows;
        } else {
            kernel_right = 0;
        }
    }
    if (r->nrows > r->ncols) {
        if (kernel_left >= r->nrows - r->ncols) {
            kernel_left -= r->nrows - r->ncols;
        } else {
            kernel_left = 0;
        }
    }
    if (kernel_right > r->nrows / 4) {
        fprintf(stderr, "Warning, right kernel is large."
                " Could trigger misbehaviours\n");
    }
    if (kernel_left > r->ncols / 4) {
        fprintf(stderr, "Warning, left kernel is large."
                " Could trigger misbehaviours\n");
    }

    if (r->rhs->n) {
        ASSERT_ALWAYS(kernel_left == 0);
        ASSERT_ALWAYS(kernel_right == 0);
        // kernel_right = 0;
    }
    /* }}} */


    if (param_list_warn_unused(pl)) usage();

    param_list_clear(pl);

    random_matrix_ddata F;
    random_matrix_ddata_init(F);
    random_matrix_ddata_set_default(F);
    random_matrix_ddata_adjust(F, r->nrows - kernel_right, r->ncols - kernel_left, r->density, 1);
    if (verbose) random_matrix_ddata_info(stderr, F);

    random_matrix_process_print(r, F);

    if (verbose)
        fprintf(stderr, "Actual density per row avg %.2f sdev %.2f\n",
                F->row_avg, F->row_sdev);
    random_matrix_ddata_clear(F);

    random_matrix_process_data_clear(r);



    return 0;
}
#endif
