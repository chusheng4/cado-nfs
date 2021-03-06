#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "macros.h"

#include "types.h"
#include "fppol.h"
#include "ffspol.h"
#include "qlat.h"
#include "fb.h"
#include "latsieve.h"
#include "norm.h"
#include "timing.h"
#include "ijvec.h"
#include "params.h"
#include "sublat.h"
#include "smoothness.h"
#include "polyfactor.h"
#include "buckets.h"
#include "fq.h"
#include "fqpol.h"

int factor_survivor(fppol_t a, fppol_t b,
        MAYBE_UNUSED ijpos_t pos, 
        MAYBE_UNUSED replayable_bucket_t *buckets,
        MAYBE_UNUSED large_factor_base_t *FB,
        ffspol_t* F, int *B, qlat_t qlat) 
{
    fppol_t Nab;
    fppol_init(Nab);
    for (int twice = 0; twice < 2; twice++) {
        ffspol_norm(Nab, F[twice], a, b);
        if (qlat->side == twice) {
            fppol_t qq;
            fppol_init(qq);
            if (!qlat->want_longq)
                fppol_set_sq(qq, qlat->q);
            else
                fppol_set(qq, qlat->longq);
            fppol_div(Nab, Nab, qq);
            fppol_clear(qq);
        }
#ifdef BUCKET_RESIEVE
        bucket_apply_at_pos(Nab, pos, buckets[twice], FB[twice]);
#endif
        if (!fppol_is_smooth(Nab, B[twice])) {
            fppol_clear(Nab);
            return 0;
        }
    }

    // There are rare cases where q divides both a and b.
    // We did check that i and j are coprime, but the transformation to a
    // and b has determinant q, so it can produce a common cofactor q.
    // For degree reasons, we expect this to be rare.
    {
        fppol_t g;
        fppol_init(g);
        fppol_gcd(g, a, b);
        int dg = fppol_deg(g);
        fppol_clear(g);
        if (dg > 0) {
            fppol_clear(Nab);
            return 0;
        } 
    }

    // OK. Now we know that we have a relation.
    // Let's factor it completely and print it.
    // But first, ensure that b is monic (destructively, but who
    // cares...)

    if (!fppol_is_monic(b)) {
        fp_t lc;
        fppol_get_coeff(lc, b, fppol_deg(b));
        fppol_sdiv(b, b, lc);
        fppol_sdiv(a, a, lc);
    }
    fppol_fact_t factors[2];
    fppol_fact_init(factors[0]);
    fppol_fact_init(factors[1]);
    for (int twice = 0; twice < 2; twice++) {
        ffspol_norm(Nab, F[twice], a, b);
        fppol_factor(factors[twice], Nab);
        // check again the smoothness for the rare cases where 
        // a multiple factor was larger than the lpb.
        for (int i = 0; i < factors[twice]->n; ++i) {
            if (fppol_deg(factors[twice]->factors[i]) > B[twice]) {
                if (qlat->side == twice) {
                    // might be q ?
                    fppol_t qq;
                    fppol_init(qq);
                    if (!qlat->want_longq)
                        fppol_set_sq(qq, qlat->q);
                    else
                        fppol_set(qq, qlat->longq);
                    if (fppol_eq(factors[twice]->factors[i], qq)) {
                        fppol_clear(qq);
                        continue;
                    }
                    fppol_clear(qq);
                }
                fppol_fact_clear(factors[0]);
                fppol_fact_clear(factors[1]);
                fppol_clear(Nab);
                return 0;
            }
        }
    }

    fppol_out(stdout, a); printf(",");
    fppol_out(stdout, b); printf(":");
    for (int twice = 0; twice < 2; twice++) {
        fppol_fact_out(stdout, factors[twice]);
        if (!twice)
            printf(":");
    }
    printf("\n");
    fppol_fact_clear(factors[0]);
    fppol_fact_clear(factors[1]);
    fppol_clear(Nab);
    return 1;
}


int sq_roots(sq_t * roots, sq_srcptr q, ffspol_srcptr F) {
    fq_info_t Fq;
    fq_info_init(Fq, q);

    fqpol_t f;
    fqpol_init(f);
    fqpol_set_ffspol(f, F, Fq);

    int nr = fqpol_roots(roots, f, Fq);
    fqpol_clear(f);
    fq_info_clear(Fq);
    return nr;
}

int sq_is_irreducible(sq_srcptr p) {
    fppol_t P;
    fppol_init(P);
    fppol_set_sq(P, p);
    int ret = fppol_is_irreducible(P);
    fppol_clear(P);
    return ret;
}

typedef struct {
    int nrels;
    double time;
    qlat_t qlat;
} stats_sq_struct;

typedef struct {
    stats_sq_struct * data;
    int n;
    int alloc;
} stats_yield_struct;

typedef stats_yield_struct stats_yield_t[1];
typedef stats_yield_struct *stats_yield_ptr;
typedef const stats_yield_struct *stats_yield_srcptr;

void stats_yield_init(stats_yield_ptr stats_yield)
{
    stats_yield->n = 0;
    stats_yield->alloc = 100;
    stats_yield->data = (stats_sq_struct *)malloc(100*sizeof(stats_sq_struct));
    ASSERT_ALWAYS(stats_yield->data != NULL);
}

void stats_yield_clear(stats_yield_ptr stats_yield)
{
    free(stats_yield->data);
}

void stats_yield_push(stats_yield_ptr stats_yield, int nrels, double time,
        qlat_srcptr qlat)
{
    if (stats_yield->n == stats_yield->alloc) {
        stats_yield->alloc += 100;
        stats_yield->data = (stats_sq_struct *)realloc(
                stats_yield->data, 
                stats_yield->alloc*sizeof(stats_sq_struct));
        ASSERT_ALWAYS(stats_yield->data != NULL);
    }
    stats_yield->data[stats_yield->n].nrels = nrels;
    stats_yield->data[stats_yield->n].time = time;
    memcpy(&stats_yield->data[stats_yield->n].qlat[0],
            qlat, sizeof(qlat_t));
    stats_yield->n++;
}


// Attempt to compute a confidence interval for the average nrels.
void nrels_confidence_interval(double *av_nrels, double * ci68, double * ci95,
        double * ci99, stats_yield_srcptr stats_yield)
{
    long tot_rels = 0;
    int n = stats_yield->n;
    double nn = (double) n;
    for (int i = 0; i < n; ++i) {
        tot_rels += stats_yield->data[i].nrels;
    }
    *av_nrels = (double)tot_rels / nn;

    double sigma = 0;
    for (int i = 0; i < n; ++i) {
        double diff = stats_yield->data[i].nrels - *av_nrels;
        sigma += diff*diff;
    }
    sigma /= (nn-1);  // the "-1" is supposed to give a better estimator.
    sigma = sqrt(sigma);
    if (ci68 != NULL) 
        *ci68 = sigma/sqrt(nn);
    if (ci95 != NULL) 
        *ci95 = 2*sigma/sqrt(nn);
    if (ci99 != NULL) 
        *ci99 = 3*sigma/sqrt(nn);
}


// Attempt to compute a confidence interval for the average yield.
void yield_confidence_interval(double *av_yield, double * ci68, double * ci95,
        double * ci99, stats_yield_srcptr stats_yield)
{
    long tot_rels = 0;
    *av_yield = 0;
    int nn = stats_yield->n;
    int n = 0;
    for (int i = 0; i < nn; ++i) {
        if (stats_yield->data[i].nrels == 0)
            continue;
        n++;
        tot_rels += stats_yield->data[i].nrels;
        *av_yield += stats_yield->data[i].time;
    }
    *av_yield /= tot_rels;

    double sigma = 0;
    for (int i = 0; i < nn; ++i) {
        if (stats_yield->data[i].nrels == 0)
            continue;
        double diff = stats_yield->data[i].time/stats_yield->data[i].nrels
            - *av_yield;
        sigma += stats_yield->data[i].nrels*diff*diff;
    }
    sigma /= (tot_rels-1);  // the "-1" is supposed to give a better estimator.
    sigma = sqrt(sigma);
    // We divide by n and not by tot_rels, because the number of samples
    // is more the number of special-q than the number of relations.
    // Indeed, the yield starts to make sense only at the special-q
    // level.
    // On the other hand, we used individual relations before, because we
    // want a notion of average yield that is global, and the variation
    // of the number of relations is correlated to the yield for a given
    // q.
    // TODO: ask someone who knows about statistics if this is correct.
    if (ci68 != NULL) 
        *ci68 = sigma/sqrt(n);
    if (ci95 != NULL) 
        *ci95 = 2*sigma/sqrt(n);
    if (ci99 != NULL) 
        *ci99 = 3*sigma/sqrt(n);
}

void stats_nrels_print_ci(stats_yield_srcptr stats_yield)
{
    double ci68, ci95, ci99, av_nrels;
    nrels_confidence_interval(&av_nrels, &ci68, &ci95, &ci99, stats_yield);
    printf("#   Confidence interval for rels/sq at 68.2%%: [%1.4f - %1.4f]\n",
            av_nrels - ci68, av_nrels + ci68);
    printf("#                                   at 95.4%%: [%1.4f - %1.4f]\n",
            av_nrels - ci95, av_nrels + ci95);
    printf("#                                   at 99.7%%: [%1.4f - %1.4f]\n",
            av_nrels - ci99, av_nrels + ci99);
}

void stats_yield_print_ci(stats_yield_srcptr stats_yield)
{
    double ci68, ci95, ci99, av_yield;
    yield_confidence_interval(&av_yield, &ci68, &ci95, &ci99, stats_yield);
    printf("#   Confidence interval for yield at 68.2%%: [%1.4f - %1.4f]\n",
            av_yield - ci68, av_yield + ci68);
    printf("#                                 at 95.4%%: [%1.4f - %1.4f]\n",
            av_yield - ci95, av_yield + ci95);
    printf("#                                 at 99.7%%: [%1.4f - %1.4f]\n",
            av_yield - ci99, av_yield + ci99);
}

#define SQSIDE_DEFAULT 0
#define FIRSTSIEVE_DEFAULT 0

void usage(const char *argv0, const char * missing)
{
    fprintf(stderr, "Usage: %s [options | optionfile] \n", argv0);
    fprintf(stderr, "  Command line options have the form '-optname optval'\n");
    fprintf(stderr, "  and take the form 'optname=optval' in the optionfile\n");
    fprintf(stderr, "List of options (a * means mandatory, default value in []):\n");
    fprintf(stderr, "  pol0 *           function field polynomial on side 0\n");
    fprintf(stderr, "  pol1 *           function field polynomial on side 1\n");
    fprintf(stderr, "  fb0  *           factor base file on side 0\n");
    fprintf(stderr, "  fb1  *           factor base file on side 1\n");
    fprintf(stderr, "  fbb0 *           factor base bound on side 0\n");
    fprintf(stderr, "  fbb1 *           factor base bound on side 1\n");
    fprintf(stderr, "  I    *           degree bound for i\n");
    fprintf(stderr, "  J    *           degree bound for j\n");
    fprintf(stderr, "  lpb0 *           large prime bound on side 0\n");
    fprintf(stderr, "  lpb1 *           large prime bound on side 1\n");
    fprintf(stderr, "  thresh0 [2*lpb0] survivor threshold on side 0\n");
    fprintf(stderr, "  thresh1 [2*lpb1] survivor threshold on side 1\n");
    fprintf(stderr, "  q    *           q-poly of the special-q\n");
    fprintf(stderr, "  rho  *           rho-poly of the special-q\n");
    fprintf(stderr, "  longq    *       variant of -q to use if q is large\n");
    fprintf(stderr, "  longrho  *       variant of -rho to use if rho is large\n");
    fprintf(stderr, "  q0   *           lower bound for special-q range\n");
    fprintf(stderr, "  q1   *           upper bound for special-q range\n");
    fprintf(stderr, "  sqt  [3]         skip special-q whose defect is sqt or more\n");
    fprintf(stderr, "  bench            bench-mode. Takes parameter of the form deg0-deg1\n");
    fprintf(stderr, "                   and estimates the time and number of rels for corresping sq.\n");
    fprintf(stderr, "                   q0 and q1 are ignored.\n");
    fprintf(stderr, "  reliableyield    ignore q1, run until estimated yield is reliable\n");
    fprintf(stderr, "                   that is in a +/-3%% interval with 95%% confidence level.\n");
    fprintf(stderr, "  reliablenrels    the same, but criterion is rels per sq.\n");
    fprintf(stderr, "  reliablerange    set the 3%% to another percentage in reliable estimates.\n");
    fprintf(stderr, "Note: giving (q0,q1) is exclusive to giving (q,rho). In the latter case,\n" "    rho is optional.\n");
    fprintf(stderr, "  sqside [%d]       side (0 or 1) of the special-q\n", SQSIDE_DEFAULT);
    fprintf(stderr, "  firstsieve [%d]   side (0 or 1) to sieve first\n", FIRSTSIEVE_DEFAULT);
    fprintf(stderr, "  S                skewness, i.e. deg(a)-deg(b)\n");
    fprintf(stderr, "  sublat           toggle the sublattice sieving\n");
    fprintf(stderr, "  gf               indicate the base field for sanity check\n");
 
    if (missing != NULL)
        fprintf(stderr, "Missing parameter: %s\n", missing);
    exit(1);
}

int main(int argc, char **argv)
{
    ffspol_t ffspol[2]; 
    qlat_t qlat;
    large_factor_base_t LFB[2];
    small_factor_base_t SFB[2];
    int fbb[2] = {0, 0};
    int I=0, J=0;  
    int lpb[2] = {0, 0};  
    unsigned int threshold[2] = {0, 0};  
    char *argv0 = argv[0];
    int want_sublat = 0;
    int sqside = SQSIDE_DEFAULT;
    int firstsieve = FIRSTSIEVE_DEFAULT;
    sq_t q0, q1;
    int rho_given = 0;
    int skewness = 0;
    int gf = 0;
    int want_reliable_yield = 0;
    int want_reliable_nrels = 0;
    double reliablerange = 0.03;
    int sqt = 3;
    int bench = 0;
    int bench_end = 0;
    double bench_tot_rels = 0;
    double bench_tot_time = 0;
    int want_longq = 0;

    param_list pl;
    param_list_init(pl);
    param_list_configure_knob(pl, "-sublat", &want_sublat);
    param_list_configure_knob(pl, "-reliableyield", &want_reliable_yield);
    param_list_configure_knob(pl, "-reliablenrels", &want_reliable_nrels);
    argv++, argc--;
    for (; argc;) {
        if (param_list_update_cmdline(pl, &argc, &argv)) {
            continue;
        }
        /* Could also be a parameter file */
        FILE *f;
        if ((f = fopen(argv[0], "r")) != NULL) {
            param_list_read_stream(pl, f);
            fclose(f);
            argv++, argc--;
            continue;
        }
        fprintf(stderr, "Unhandled parameter %s\n", argv[0]);
        usage(argv0, NULL);
    }
    // Update parameter list at least once to register argc/argv pointers.
    param_list_update_cmdline(pl, &argc, &argv);
    param_list_print_command_line(stdout, pl);

    param_list_parse_int(pl, "gf", &gf);
    if (gf) {
        if (gf != FP_SIZE) {
            fprintf(stderr, "Error: base field mismatch.\n");
            fprintf(stderr, "  The binary is compiled for GF(%d)\n", FP_SIZE);
            fprintf(stderr, "  The parameters are for GF(%d)\n", gf);
            exit(EXIT_FAILURE);
        }
    }

    // read function field polynomials
    {
        const char * polstr;
        ffspol_init(ffspol[0]);
        ffspol_init(ffspol[1]);
        polstr = param_list_lookup_string(pl, "pol0");
        if (polstr == NULL) usage(argv0, "pol0");
        ffspol_set_str(ffspol[0], polstr);
        polstr = param_list_lookup_string(pl, "pol1");
        if (polstr == NULL) usage(argv0, "pol1");
        ffspol_set_str(ffspol[1], polstr);
    }
    // read various bounds
    param_list_parse_int(pl, "sqt", &sqt); 
    param_list_parse_int(pl, "S", &skewness); 
    param_list_parse_int(pl, "I", &I); 
    param_list_parse_int(pl, "J", &J); 
    param_list_parse_int(pl, "lpb0", &lpb[0]);
    param_list_parse_int(pl, "lpb1", &lpb[1]);
    param_list_parse_int(pl, "fbb0", &fbb[0]);
    param_list_parse_int(pl, "fbb1", &fbb[1]);
    param_list_parse_uint(pl, "thresh0", &threshold[0]);
    param_list_parse_uint(pl, "thresh1", &threshold[1]);
    if (I == 0) usage(argv0, "I");
    if (J == 0) usage(argv0, "J");
    if (lpb[0] == 0) usage(argv0, "lpb0");
    if (lpb[1] == 0) usage(argv0, "lpb1");
    if (threshold[0] == 0) 
        threshold[0] = 2*lpb[0];
    if (threshold[1] == 0) 
        threshold[1] = 2*lpb[1];

    // Check if we want to be in "longq" mode
    {
        const char *sqstr;
        int noerr;
        sqstr = param_list_lookup_string(pl, "longq");
        if (sqstr != NULL) {
            fppol_init(qlat->longq);
            fppol_init(qlat->longrho);
            fppol_init(qlat->longa0);
            fppol_init(qlat->longa1);
            fppol_init(qlat->longb0);
            fppol_init(qlat->longb1);
            want_longq = 1;
            qlat->want_longq = 1;
            noerr = fppol_set_str(qlat->longq, sqstr);
            if (!noerr) {
                fprintf(stderr, "Could not parse longq: %s\n", sqstr);
                exit(EXIT_FAILURE);
            }
            if (!fppol_is_monic(qlat->longq)) {
                fprintf(stderr, "Error: given longq is not monic: %s\n", sqstr);
                exit(EXIT_FAILURE);
            }
            if (!fppol_is_irreducible(qlat->longq)) {
                fprintf(stderr, "Error, longq is not irreducible: %s\n", sqstr);
                exit(EXIT_FAILURE);
            }
            sqstr = param_list_lookup_string(pl, "longrho");
            if (sqstr == NULL) {
                fprintf(stderr, "Error, longrho is required with longq\n");
                exit(EXIT_FAILURE);
            }
            noerr = fppol_set_str(qlat->longrho, sqstr);
            if (!noerr) {
                fprintf(stderr, "Could not parse longrho: %s\n", sqstr);
                exit(EXIT_FAILURE);
            }
        } else {
            qlat->want_longq = 0;
        }
    }

    // read q0, q1
    if (!want_longq) {
        const char *sqstr;
        int noerr;
        sqstr = param_list_lookup_string(pl, "q0");
        if (sqstr != NULL) {
            noerr = sq_set_str(q0, sqstr);
            if (!noerr) {
                fprintf(stderr, "Could not parse q0: %s\n", sqstr);
                exit(EXIT_FAILURE);
            }
            if (!sq_is_monic(q0)) {
                fprintf(stderr, "Error: given q0 is not monic: %s\n", sqstr);
                exit(EXIT_FAILURE);
            }
            sqstr = param_list_lookup_string(pl, "q1");
            if (sqstr == NULL) usage(argv0, "q1");
            noerr = sq_set_str(q1, sqstr);
            if (!noerr) {
                fprintf(stderr, "Could not parse q1: %s\n", sqstr);
                exit(EXIT_FAILURE);
            }
            if (sq_deg(q1) > lpb[0]) {
                fprintf(stderr, "WARNING: not a good idea to have special-q beyond the large prime bound!\n");
            }
        } else {
            sq_set_zero(q0);
            sq_set_zero(q1);
        }
    }

    // want to bench ?
#if FP_SIZE == 2
#define BENCH_RANGE 10
#else
#define BENCH_RANGE 5
#endif
    {
        int res[2];
        int ret = param_list_parse_int_and_int(pl, "bench", res, "-");
        if (ret) {
            bench = 1;
            bench_end = res[1];
            sq_set_ti(q0, res[0]);
            sq_t range;
            sq_set_ti(range, BENCH_RANGE);
            sq_add(q1, q0, range);
        }
    }

    // read q, rho 
    // We store them in qlat for convenience, but these will be moved
    // away before the main loop.
    if (!want_longq) {
        const char *sqstr;
        int noerr;
        sqstr = param_list_lookup_string(pl, "q");
        if (sq_is_zero(q0) && (sqstr == NULL)) usage(argv0, "q");
        if (sqstr != NULL) {
            if (!sq_is_zero(q0)) {
                fprintf(stderr, "You can not provide both (q0,q1) and q\n");
                exit(EXIT_FAILURE);
            }
            noerr = sq_set_str(qlat->q, sqstr);
            if (!noerr) {
                fprintf(stderr, "Could not parse q: %s\n", sqstr);
                exit(EXIT_FAILURE);
            }
            if (!sq_is_irreducible(qlat->q)) {
                fprintf(stderr, "Error, q is not irreducible: %s\n", sqstr);
                exit(EXIT_FAILURE);
            }
            if (!sq_is_monic(qlat->q)) {
                fprintf(stderr, "Error, q is not monic: %s\n", sqstr);
                exit(EXIT_FAILURE);
            }
            sqstr = param_list_lookup_string(pl, "rho");
            if (sqstr != NULL) {
                rho_given = 1;
                noerr = sq_set_str(qlat->rho, sqstr);
                if (!noerr) {
                    fprintf(stderr, "Could not parse rho: %s\n", sqstr);
                    exit(EXIT_FAILURE);
                }
                if (sq_deg(qlat->q) > lpb[0]) {
                    fprintf(stderr, "WARNING: not a good idea to have a special-q beyond the large prime bound!\n");
                }
            }
        }
    }

    param_list_parse_int(pl, "sqside", &sqside);
    if (sqside != 0 && sqside != 1) {
        fprintf(stderr, "sqside must be 0 or 1\n");
        exit(EXIT_FAILURE);
    }
    param_list_parse_int(pl, "firstsieve", &firstsieve);
    if (firstsieve != 0 && firstsieve != 1) {
        fprintf(stderr, "firstsieve must be 0 or 1\n");
        exit(EXIT_FAILURE);
    }
 
#ifdef USE_F2
    sublat_ptr sublat;
    if (want_sublat) {
#ifdef DISABLE_SUBLAT
        fprintf(stderr,
                "Error: your binary was compiled with DISABLE_SUBLAT\n");
        exit(EXIT_FAILURE);
#endif
        sublat = &nine_sublat[0];
    } else
        sublat = &no_sublat[0];
#else
    if (want_sublat)
        fprintf(stderr, "# Sorry, no sublattices in characteristic > 2. Ignoring the 'sublat' option\n");
    sublat_ptr sublat = &no_sublat[0];
#endif

    param_list_parse_double(pl, "reliablerange", &reliablerange);
    if (want_reliable_yield && want_reliable_nrels) {
        fprintf(stderr, "Error: -reliableyield and -reliablenrels are incompatible options.\n");
        exit(EXIT_FAILURE);
    }
  
    // Most of what we do is at the sublattice level. 
    // So we fix I and J accordingly.
    I -= sublat->deg;
    J -= sublat->deg;

    // Read the factor bases
    {
        const char *filename;
        int noerr;
        for (int i = 0; i < 2; ++i) {
            char param[4] = {'f', 'b', '0', '\0'};
            if (i == 1) 
                param[2] = '1';
            filename = param_list_lookup_string(pl, param);
            if (filename == NULL) usage(argv0, param);
            double tm = seconds();
            noerr = factor_base_init(LFB[i], SFB[i], filename, I, fbb[i],
                    I, J, sublat);
            fprintf(stdout, "# Reading factor base %d took %1.1f s\n", 
                    i, seconds()-tm);
            if (!noerr) {
                fprintf(stderr, "Could not read %s: %s\n", param, filename);
                exit(EXIT_FAILURE);
            }

        }
    }

    param_list_clear(pl);

    // Allocate storage space for the buckets.
    buckets_t buckets[2];
    buckets_init(buckets[0], I, J, expected_hit_number(LFB[0], I, J),
            I, 1+factor_base_max_degp(LFB[0]));
    buckets_init(buckets[1], I, J, expected_hit_number(LFB[1], I, J),
            I, 1+factor_base_max_degp(LFB[1]));
    ASSERT_ALWAYS(buckets[0]->n == buckets[1]->n);
    print_bucket_info(buckets[0], buckets[1]);
    fflush(stdout);

#ifdef BUCKET_RESIEVE
    replayable_bucket_t replayable_bucket[2];
    replayable_bucket[0]->b = (__replayable_update_struct *)
        malloc((buckets[0]->max_size) * sizeof(__replayable_update_struct));
    replayable_bucket[1]->b = (__replayable_update_struct *)
        malloc((buckets[1]->max_size) * sizeof(__replayable_update_struct));
    ASSERT_ALWAYS(replayable_bucket[0]->b != NULL);
    ASSERT_ALWAYS(replayable_bucket[1]->b != NULL);
#else
    void * replayable_bucket = NULL;
#endif

    // Size of a bucket region.
    unsigned size = bucket_region_size();

    // Allocate space for a bucket region.
    uint8_t *S;
    S = (uint8_t *) malloc(size*sizeof(uint8_t));
    ASSERT_ALWAYS(S != NULL);

    qlat->side = sqside; 

    double tot_time = seconds();
    double tot_norms = 0;
    double tot_sieve = 0;
    double tot_buck_fill = 0;
    double tot_buck_apply = 0;
    double tot_cofact = 0;

    int tot_nrels = 0;
    int tot_sq = 0;
    int no_rels_sq = 0;
    
    sq_t * roots;
    roots = (sq_t *) malloc (ffspol[sqside]->deg * sizeof(sq_t));
    ASSERT_ALWAYS(roots != NULL);
    int nroots = 0; // number of roots still to work on for current q.

    if (!want_longq) {
        if (sq_is_zero(q0)) {
            sq_set(q0, qlat->q);
            sq_set(q1, qlat->q);
        }
        if (rho_given) {
            nroots = 1;
            sq_set(roots[0], qlat->rho);
        } else {
            if (sq_is_irreducible(q0)) {
                nroots = sq_roots(roots, q0, ffspol[sqside]);
                sq_set(qlat->q, q0);
                printf("############################################\n");
                printf("# Roots for q = "); 
                sq_out(stdout, q0);
                printf(":");
                for (int i = 0; i < nroots; ++i) {
                    printf(" ");
                    sq_out(stdout, roots[i]);
                }
                printf("\n");
            }
        }
    }

    stats_yield_t stats_yield;
    stats_yield_init(stats_yield);

    //
    // Begin of loop over special-q's
    //
    do {
      if (!want_longq) {
        // Select next special-q
        if (nroots == 0) { // find next q
            // exit early if rho was given
            if (rho_given)
                break;
            // otherwise, compute next valid q.
            do {
                do {
                    sq_monic_set_next(q0, q0, 64);
                } while (!sq_is_irreducible(q0));
                nroots = sq_roots(roots, q0, ffspol[sqside]);
            } while (nroots == 0);

            if (want_reliable_nrels) {
                if (stats_yield->n > 10) {
                    double av_nrels, ci95;
                    nrels_confidence_interval(&av_nrels, NULL, &ci95, NULL,
                            stats_yield);
                    printf("############################################\n");
                    printf("#   Current average nrels: %1.2f\n", av_nrels);
                    stats_nrels_print_ci(stats_yield);
                    if (ci95/av_nrels < reliablerange)
                        break;
                }
            } else if (want_reliable_yield) {
                if (stats_yield->n > 10) {
                    double av_yield, ci95;
                    yield_confidence_interval(&av_yield, NULL, &ci95, NULL,
                            stats_yield);
                    printf("############################################\n");
                    printf("#   Current average yield: %1.2f\n", av_yield);
                    stats_yield_print_ci(stats_yield);
                    if (ci95/av_yield < reliablerange)
                        break;
                }
            } else if (sq_cmp(q0, q1) >= 0) {
                if (!bench)
                    break; 
                int degq0 = sq_deg(q0);
                double rpq = (double)tot_nrels / (double)tot_sq;
                double nsq = (double)(1L<<degq0) / (double)degq0;
                double nr = rpq*nsq;
                printf("#BENCH ###########################################\n");
                printf("#BENCH Estimations for special-q's of degree %d:\n",
                        degq0);
                printf("#BENCH   rels per sq: %1.2f rel/sq\n", rpq);
                printf("#BENCH   rels for all sq: %1.0f\n", nr);
                tot_time = seconds()-tot_time;
                double yield = (double)tot_time / (double)tot_nrels;
                double tm = yield*nr;
                printf("#BENCH   yield: %1.2f s/rel\n", yield);
                printf("#BENCH   time: %1.0f s", tm);
                printf(" = %1.1f d\n", tm/86400);
                bench_tot_rels += nr;
                bench_tot_time += tm;
                printf("#BENCH Accumulated data for deg up to %d:\n", degq0);
                printf("#BENCH   total rels: %1.0f\n", bench_tot_rels);
                printf("#BENCH   total time: %1.0f s", bench_tot_time);
                printf(" = %1.1f d\n", bench_tot_time/86400);
                printf("#BENCH ###########################################\n");
                tot_nrels = 0;
                tot_time = seconds();
                tot_sq = 0;
                
                if (degq0 >= bench_end)
                    break;
                else {
                    // select next degree to bench
                    sq_set_ti(q0, degq0+1);
                    sq_t range;
                    sq_set_ti(range, BENCH_RANGE);
                    sq_add(q1, q0, range);
                    nroots = 0;
                    continue;
                }
            }

            printf("############################################\n");
            printf("# Roots for q = "); 
            sq_out(stdout, q0);
            printf(":");
            for (int i = 0; i < nroots; ++i) {
                printf(" ");
                sq_out(stdout, roots[i]);
            }
            printf("\n");

            sq_set(qlat->q, q0);
            sq_set(qlat->rho, roots[nroots-1]);
            nroots--;
        } else {
            sq_set(qlat->rho, roots[nroots-1]);
            nroots--;
        }
      } // end of selection of next sq.

        tot_sq++;

        double t_tot = seconds();

        double t_norms = 0;
        double t_sieve = 0;
        double t_buck_fill = 0;
        double t_buck_apply = 0;
        double t_cofact = 0;
        int nrels = 0;

        // Check the given special-q
        if (!is_valid_sq(qlat, ffspol[sqside])) {
            if (want_longq) {
                fprintf(stderr, "Error: the longrho is not a root mod longq\n");
                exit(EXIT_FAILURE);
            }
            fprintf(stderr, "Error: the rho = ");
            sq_out(stderr, qlat->rho);
            fprintf(stderr, " is not a root modulo ");
            sq_out(stderr, qlat->q);
            fprintf(stderr, " of the polynomial %d\n", sqside);
            exit(EXIT_FAILURE);
        }

        // Reduce the q-lattice
        int noerr = skewGauss(qlat, skewness);
        ASSERT_ALWAYS(noerr);
        printf("############################################\n");
        print_qlat_info(qlat);
        fflush(stdout);

        // If the reduced q-lattice is still too unbalanced, then skip it.
        if (!want_longq) {
            // the optimal degree is ceiling( (s + deg(q))/2 ).
            int opt_deg = (skewness + sq_deg(qlat->q) + 1) / 2;
            int sqsize = MAX(
                    MAX(ai_deg(qlat->a0), ai_deg(qlat->a1)),
                    MAX(skewness+ai_deg(qlat->b0), skewness+ai_deg(qlat->b1))
                    );
            printf("#   qlat vector degree: %d\n", sqsize);
            if (sqsize > opt_deg + sqt) {
                printf("# Special-q lattice basis is too unbalanced, let's skip it!\n");
                tot_sq--;
                continue;
            }
        }

        // Precompute all the data for small factor base elements.
        for (int i = 0; i < 2; ++i)
            small_factor_base_precomp(SFB[i], I, J, qlat);

        // Loop on all sublattices
        // In the no_sublat case, this loops degenerates into one pass, since
        // nb = 1.
        for (sublat->n = 0; sublat->n < sublat->nb; sublat->n++) {
#if 0
            if (use_sublat(sublat)) {
                fprintf(stdout, "# Sublattice (");
                fppol16_out(stdout, sublat->lat[sublat->n][0]);
                fprintf(stdout, ", ");
                fppol16_out(stdout, sublat->lat[sublat->n][1]);
                fprintf(stdout, ") :\n");
            }
#endif
            // Fill the buckets.
            t_buck_fill -= seconds();
            buckets_fill(buckets[0], LFB[0], sublat, I, J, qlat);
            buckets_fill(buckets[1], LFB[1], sublat, I, J, qlat);
            t_buck_fill += seconds();

            // j0 is the first valid line in the current bucket region.
            ij_t j0;
            ij_set_zero(j0);
	    ijpos_t pos0 = 0;
            for (unsigned k = 0; k < buckets[0]->n;
                 ++k, pos0 += size) {
              // Skip empty bucket regions.
              if (ijvec_get_start_pos(j0, I, J) >= pos0+size)
                continue;

              // Init the bucket region.
              memset(S, 0, size*sizeof(uint8_t));

              // Kill trivial positions.
              // When there are no sublattices:
              //   (i,0) for i != 1
              //   (0,j) for j != 1
              // When using sublattices, just the position (0,0)
              {
                if (UNLIKELY(!k)) {
                  S[0] = 255;  // that's (0,0)
                  if (!use_sublat(sublat)) {
                    ij_t i;
                    for (ij_set_one(i); ij_set_next(i, i, I); )
                      S[ijvec_get_offset(i, I)] = 255;
                  }
                }
                if (!use_sublat(sublat)) {
                  ij_t j;
                  ij_set(j, j0);
                  for (int rc = 1; rc; rc = ij_monic_set_next(j, j, J)) {
                    if (UNLIKELY(ij_in_fp(j)))
                      continue;
                    ijpos_t pos = ijvec_get_start_pos(j, I, J) - pos0;
                    if (pos >= size)
                      break;
                    S[pos] = 255;
                  }
                }
              }

              for (int twice = 0; twice < 2; twice++) {
                // Select the side to be sieved
                int side = (firstsieve)?(1-twice):twice;

                // Norm initialization.
                // convention: if a position contains 255, it must stay like
                // this. It means that the other side is hopeless.
                t_norms -= seconds();
                init_norms(S, ffspol[side], I, J, j0, pos0, size,
                           qlat, qlat->side == side, sublat, side);
                t_norms += seconds();

                // Line sieve.
                unsigned int sublat_thr;
                t_sieve -= seconds();
                sieveSFB(S, &sublat_thr, SFB[side], I, J,
                        j0, pos0, size, sublat);
                t_sieve += seconds();

                // Apply the updates from the corresponding bucket.
                t_buck_apply -= seconds();
                bucket_apply(S, buckets[side], k);
                t_buck_apply += seconds();

                // since (0,0) is divisible by everyone, its position might
                // have been clobbered.
                if (!k && !use_sublat(sublat)) S[0] = 255;

                // mark survivors
                // no need to check if this is a valid position
                for (unsigned i = 0; i < size; ++i) {
                  if (S[i] > (threshold[side] + sublat_thr)>>SCALE) {
                    S[i] = 255; 
#ifdef TRACE_POS
                    if (i + pos0 == TRACE_POS) {
                        fprintf(stderr, "TRACE_POS(%" PRIu64 "): ", i + pos0);
                        fprintf(stderr, "above threshold.\n");
                    }
#endif
                  } else {
                    S[i] = 0;
#ifdef TRACE_POS
                    if (i + pos0 == TRACE_POS) {
                        fprintf(stderr, "TRACE_POS(%" PRIu64 "): ", i + pos0);
                        fprintf(stderr, "below threshold.\n");
                    }
#endif
                  }
                }
              }

#ifdef BUCKET_RESIEVE
              // prepare replayable buckets
              bucket_prepare_replay(replayable_bucket[0], buckets[0], S, k);
              bucket_prepare_replay(replayable_bucket[1], buckets[1], S, k);
#endif

              t_cofact -= seconds();
              // survivors cofactorization
              {
                fppol_t a, b;
                ij_t i, j, g;
                ij_t hati, hatj;
                fppol_init(a);
                fppol_init(b);

                int rci, rcj = 1;
                for (ij_set(j, j0); rcj; rcj = ij_monic_set_next(j, j, J)) {
                  ijpos_t start = ijvec_get_start_pos(j, I, J) - pos0;
                  if (start >= size)
                    break;
                  rci = 1;
                  for (ij_set_zero(i); rci; rci = ij_set_next(i, i, I)) {
                    ijpos_t pos = start + ijvec_get_offset(i, I);

                    if (S[pos] != 255) {
#ifdef TRACE_POS
                      if (pos + pos0 == TRACE_POS) {
                        fprintf(stderr, "TRACE_POS(%" PRIu64 "): ", pos+pos0);
                        fprintf(stderr,
                           "entering cofactorization, S[pos] = %d\n",
                           S[pos]);
                        }
#endif 
                      ij_convert_sublat(hati, hatj, i, j, sublat);
                      ij_gcd(g, hati, hatj);
                      if (ij_deg(g) != 0 && ij_deg(hati)>0  && ij_deg(hatj)>0)
                        continue;
                      ij2ab(a, b, hati, hatj, qlat);
                      nrels += factor_survivor(a, b, pos, replayable_bucket,
                              LFB, ffspol, lpb, qlat);
                    }
                  }
                }
                ij_set(j0, j);
                fppol_clear(a);
                fppol_clear(b);
              }
              t_cofact += seconds();
            }

        }  // End of loop on sublattices.

        t_tot = seconds()-t_tot;
        fprintf(stdout, "# Total for this special-q: %d relations found "
                "in %1.1f s\n", nrels, t_tot);
        fprintf(stdout,
                "# Time of main steps: "
                "%1.2f s      (norms);                "
                "%1.2f s (sieve);\n"
                "#                     "
                "%1.2f+%1.2f s (buckets: fill+apply);  "
                "%1.2f s (cofact).\n",
                t_norms, t_sieve, t_buck_fill, t_buck_apply, t_cofact);
        fprintf(stdout, "# Yield: %1.5f s/rel\n", t_tot/nrels);
        fflush(stdout);
        tot_nrels += nrels;
        tot_norms   += t_norms;
        tot_sieve   += t_sieve;
        tot_buck_apply += t_buck_apply;
        tot_buck_fill += t_buck_fill;
        tot_cofact  += t_cofact;

        if (nrels == 0) {
            no_rels_sq++;
        } 
        if (want_longq)
            break;
        
        stats_yield_push(stats_yield, nrels, t_tot, qlat);
    } while (1); // End of loop over special-q's

    free(S);
    factor_base_clear(LFB[0], SFB[0]);
    factor_base_clear(LFB[1], SFB[1]);
    buckets_clear(buckets[0]);
    buckets_clear(buckets[1]);
    free(roots);
#ifdef BUCKET_RESIEVE
    free(replayable_bucket[0]->b);
    free(replayable_bucket[1]->b);
#endif

    if (!want_longq && !bench) {
    tot_time = seconds()-tot_time;
    fprintf(stdout, "###### General statistics ######\n");
    fprintf(stdout, "#   Total time: %1.1f s\n", tot_time);
    fprintf(stdout,
                "# Time of main steps: "
                "%1.2f s      (norms);                "
                "%1.2f s (sieve);\n"
                "#                     "
                "%1.2f+%1.2f s (buckets: fill+apply);  "
                "%1.2f s (cofact).\n",
        tot_norms, tot_sieve,
        tot_buck_fill, tot_buck_apply, tot_cofact);
 
    fprintf(stdout, "#   Computed %d special-q\n", tot_sq);
    fprintf(stdout, "#   %d relations found (%1.1f rel/sq)\n",
            tot_nrels, (double)tot_nrels / (double)tot_sq);
    fprintf(stdout, "#   Yield: %1.5f s/rel\n", tot_time/tot_nrels);
    stats_nrels_print_ci(stats_yield);
    stats_yield_print_ci(stats_yield);
    if (no_rels_sq > 0) {
        fprintf(stdout, "#   Warning: statistics on yield are biased. There were %d special-q with no relations.\n", no_rels_sq);
    }
#ifdef WANT_NORM_STATS
    norm_stats_print();
#endif
    }

    ffspol_clear(ffspol[0]);
    ffspol_clear(ffspol[1]);
    stats_yield_clear(stats_yield);

    return EXIT_SUCCESS;
}
