#include "cado.h"
#include <stdio.h>
#include "batch.h"
#include "utils.h"

static void
mpz_list_init (mpz_list L)
{
  L->l = NULL;
  L->alloc = L->size = 0;
}

/* at any point, all values up to L->alloc should be mpz_init()'ed,
   even beyond L->size */
static void
mpz_list_add (mpz_list L, unsigned long n)
{
  if (L->size == L->alloc)
    {
      unsigned long old = L->alloc, i;
      L->alloc = 3 * (L->alloc / 2) + 2;
      L->l = realloc (L->l, L->alloc * sizeof (mpz_t));
      for (i = old; i < L->alloc; i++)
        mpz_init (L->l[i]);
    }
  ASSERT(L->size < L->alloc);
  mpz_set_ui (L->l[L->size], n);
  L->size ++;
}

static void
mpz_list_clear (mpz_list L)
{
  size_t i;

  for (i = 0; i < L->alloc; i++)
    mpz_clear (L->l[i]);
  free (L->l);
  L->alloc = L->size = 0;
}

void
cofac_list_init (cofac_list l)
{
  l->a = NULL;
  l->b = NULL;
  l->R = NULL;
  l->A = NULL;
  l->R0 = NULL;
  l->A0 = NULL;
  l->sq = NULL;
  l->perm = NULL;
  l->alloc = 0;
  l->size = 0;
}

/* add in the list L all primes pmin <= p < pmax.
   Assume pmin is the current prime in 'pi'
   (pmin=2 when 'pi' was just initialized).
   Return the current prime in 'pi' at the end, i.e.,
   the smallest prime >= pmax. */
static unsigned long
prime_list (mpz_list L, prime_info pi, unsigned long pmin,
            unsigned long pmax)
{
  unsigned long p;

  for (p = pmin; p < pmax; p = getprime_mt (pi))
    mpz_list_add (L, p);
  return p;
}

/* same as prime_list, but only adds primes p for which f has at least one
   root modulo p, or the leading coefficient of f vanishes modulo p */
static unsigned long
prime_list_poly (mpz_list L, prime_info pi, unsigned long pmin,
                 unsigned long pmax, mpz_poly_t f)
{
  unsigned long p;

  if (f->deg == 1)
    return prime_list (L, pi, pmin, pmax);

  for (p = pmin; p < pmax; p = getprime_mt (pi))
    if (mpz_divisible_ui_p (f->coeff[f->deg], p) ||
        mpz_poly_roots_ulong (NULL, f, p) > 0)
      mpz_list_add (L, p);
  return p;
}

void
cofac_list_realloc (cofac_list l, size_t newsize)
{
  unsigned long i;

  /* if we shrink the list, clear the mpz_t's */
  for (i = newsize; i < l->size; i++)
    {
      mpz_clear (l->R[i]);
      mpz_clear (l->A[i]);
      mpz_clear (l->R0[i]);
      mpz_clear (l->A0[i]);
      mpz_clear (l->sq[i]);
    }
  l->a = realloc (l->a, newsize * sizeof (int64_t));
  l->b = realloc (l->b, newsize * sizeof (uint64_t));
  l->R = realloc (l->R, newsize * sizeof (mpz_t));
  l->A = realloc (l->A, newsize * sizeof (mpz_t));
  l->R0 = realloc (l->R0, newsize * sizeof (mpz_t));
  l->A0 = realloc (l->A0, newsize * sizeof (mpz_t));
  l->sq = realloc (l->sq, newsize * sizeof (mpz_t));
  l->perm = realloc (l->perm, newsize * sizeof (uint32_t));
  l->alloc = newsize;
  if (newsize < l->size)
    l->size = newsize;
}

void
cofac_list_add (cofac_list l, long a, unsigned long b, mpz_t R, mpz_t A,
                mpz_t sq)
{
  if (l->size == l->alloc)
    cofac_list_realloc (l, 2 * l->alloc + 1);
  l->a[l->size] = a;
  l->b[l->size] = b;
  mpz_init_set (l->R[l->size], R);
  mpz_init_set (l->A[l->size], A);
  mpz_init_set (l->R0[l->size], R);
  mpz_init_set (l->A0[l->size], A);
  mpz_init_set (l->sq[l->size], sq);
  l->perm[l->size] = l->size;
  (l->size)++;
}

void
cofac_list_clear (cofac_list l)
{
  unsigned long i;
  for (i = 0; i < l->size; i++)
    {
      mpz_clear (l->R[i]);
      mpz_clear (l->A[i]);
      mpz_clear (l->R0[i]);
      mpz_clear (l->A0[i]);
      mpz_clear (l->sq[i]);
    }
  free (l->a);
  free (l->b);
  free (l->R);
  free (l->A);
  free (l->R0);
  free (l->A0);
  free (l->sq);
  free (l->perm);
}

/* FIXME: since we don't need to keep the indidivual primes here,
   instead of allocating spaces for n mpz_t data structures, we need
   only to allocate O(log n) [see function compute_biproduct below] */
unsigned long
prime_product (mpz_t P, prime_info pi, unsigned long p_max,
               unsigned long p_last)
{
  unsigned long i;
  mpz_list L;

  mpz_list_init (L);
  p_last = prime_list (L, pi, p_last, p_max);

  /* FIXME: equilibrate the product */
  mpz_t *l = L->l;
  unsigned long n = L->size;
  while (n > 1)
  {
    for (i = 0; i+1 < n; i+=2)
      mpz_mul (l[i/2], l[i], l[i+1]);
    if (n & 1)
      mpz_swap (l[n/2], l[n-1]);
    n = (n + 1) / 2;
  }
  mpz_set (P, l[0]);

  mpz_list_clear (L);
  return p_last;
}

/* same as prime_product, but keeps only primes for which the given polynomial
   has factors modulo p */
static void
prime_product_poly (mpz_t P, prime_info pi, unsigned long p_max,
                    unsigned long p_last, mpz_poly_t f)
{
  unsigned long i;
  mpz_list L;

  mpz_list_init (L);
  p_last = prime_list_poly (L, pi, p_last, p_max, f);

  /* FIXME: equilibrate the product */
  mpz_t *l = L->l;
  unsigned long n = L->size;
  while (n > 1)
  {
    for (i = 0; i+1 < n; i+=2)
      mpz_mul (l[i/2], l[i], l[i+1]);
    if (n & 1)
      mpz_swap (l[n/2], l[n-1]);
    n = (n + 1) / 2;
  }
  mpz_set (P, l[0]);

  mpz_list_clear (L);
}

static unsigned long
tree_height (unsigned long n)
{
  unsigned long h = 0;

  while (n > 1)
    {
      h ++;
      n = (n + 1) / 2;
    }
  return h;
}

/* return the product tree formed from R[0..n-1].
   Put in w[i] the number of elements of level i:
   w[0] = n, w[1] = ceil(n/2), ... */
static mpz_t**
product_tree (mpz_t *R, uint32_t *perm, unsigned long n, unsigned long *w)
{
  unsigned long h = tree_height (n), i, j;
  mpz_t **T;

  ASSERT_ALWAYS(n >= 1);

  T = (mpz_t**) malloc ((h + 1) * sizeof (mpz_t*));

  /* initialize tree */
  w[0] = n;
  for (i = 0; i <= h; i++)
    {
      w[i] = 1 + ((n - 1) >> i);
      T[i] = (mpz_t*) malloc (w[i] * sizeof (mpz_t));
      for (j = 0; j < w[i]; j++)
        mpz_init (T[i][j]);
    }

  /* initialize T[0] to R */
  for (j = 0; j < n; j++)
    mpz_set (T[0][j], R[perm[j]]);

  /* compute product tree */
  for (i = 1; i <= h; i++)
    {
      for (j = 0; j < w[i-1] / 2; j++)
        mpz_mul (T[i][j], T[i-1][2*j], T[i-1][2*j+1]);
      if (w[i-1] & 1)
        mpz_set (T[i][w[i]-1], T[i-1][w[i-1]-1]);
    }

  return T;
}

/* Compute the remainder tree using the "scaled" variant
   (http://cr.yp.to/arith/scaledmod-20040820.pdf).
   At the root, we compute a floating-point
   approximation of P/T[h][0] with m+guard bits, where m = nbits(T[h][0]). */
static void
remainder_tree (mpz_t **T, unsigned long n, unsigned long *w, mpz_t P,
                mpz_t *R, uint32_t *perm)
{
  unsigned long h = tree_height (n), i, j, guard;
  unsigned long **nbits;
  mpz_t Q;

  guard = h;
  nbits = malloc ((h + 1) * sizeof (unsigned long*));
  for (i = 0; i <= h; i++)
    {
      nbits[i] = malloc (w[i] * sizeof (unsigned long));
      for (j = 0; j < w[i]; j++)
        nbits[i][j] = mpz_sizeinbase (T[i][j], 2);
    }

  mpz_init (Q);
  mpz_mod (Q, P, T[h][0]); /* first reduce modulo T[h][0] in case P is huge */
  mpz_mul_2exp (Q, Q, nbits[h][0] + guard);
  mpz_tdiv_q (T[h][0], Q, T[h][0]);
  /* P/T[h][0] ~ Q/2^(m+guard) */
  for (i = h; i > 0; i--)
    {
      for (j = 0; j < w[i-1] / 2; j++)
        {
          /* T[i][j]/2^(nbits[i][j] + guard) ~ P/T[i][j] */
          mpz_mul (T[i-1][2*j], T[i][j], T[i-1][2*j]);
          /* same for the right part */
          mpz_mul (T[i-1][2*j+1], T[i][j], T[i-1][2*j+1]);
          /* swap */
          mpz_swap (T[i-1][2*j], T[i-1][2*j+1]);

          /* get the fractional part, i.e., the low nbits[i][j] + guard bits */
          mpz_tdiv_r_2exp (T[i-1][2*j], T[i-1][2*j], nbits[i][j] + guard);
          /* now keep only nbits[i-1][2*j] + guard significant bits */
          mpz_div_2exp (T[i-1][2*j], T[i-1][2*j], nbits[i][j] - nbits[i-1][2*j]);

          mpz_tdiv_r_2exp (T[i-1][2*j+1], T[i-1][2*j+1], nbits[i][j] + guard);
          mpz_div_2exp (T[i-1][2*j+1], T[i-1][2*j+1], nbits[i][j] - nbits[i-1][2*j+1]);

        }
      if (w[i-1] & 1)
        mpz_swap (T[i-1][w[i-1]-1], T[i][w[i]-1]);
    }

  /* from T[0][j] ~ P/R[j]*2^(nbits[0][j] + guard) mod 2^(nbits[0][j] + guard),
     get T[0][j]*R[j]/2^(nbits[0][j] + guard) ~ P mod R[j] */
  for (j = 0; j < n; j++)
    {
      mpz_mul (T[0][j], T[0][j], R[perm[j]]);
      /* T[0][j] ~ P*2^(nbits[0][j] + guard) mod R[j]*2^(nbits[0][j]+guard) */
      mpz_div_2exp (T[0][j], T[0][j], nbits[0][j]);
      /* T[0][j] ~ P*2^guard mod R[j]*2^guard */
      mpz_add_ui (T[0][j], T[0][j], (1UL << guard) - 1UL);
      mpz_div_2exp (T[0][j], T[0][j], guard);
    }

  mpz_clear (Q);
  for (i = 0; i <= h; i++)
    free (nbits[i]);
  free (nbits);
}

/* Clear the product tree. */
static void
clear_product_tree (mpz_t **T, unsigned long n, unsigned long *w)
{
  unsigned long i, j, h = tree_height (n);

  for (i = 0; i <= h; i++)
    {
      for (j = 0; j < w[i]; j++)
        mpz_clear (T[i][j]);
      free (T[i]);
    }
  free (T);
}

#define MAX_DEPTH 32

/* Input:
   R[0], ..., R[n-1] are cofactors
   P is the product of primes
   Output:
   Each R[j] has been divided by its P-smooth part.
*/
static void
smoothness_test (mpz_t *R, uint32_t *perm, unsigned long n, mpz_t P)
{
  unsigned long j, w[MAX_DEPTH];
  mpz_t **T;

  if (n == 0)
    return;

  T = product_tree (R, perm, n, w);

  /* compute remainder tree */
  remainder_tree (T, n, w, P, R, perm);

  /* now T[0][j] = P mod R[j] for 0 <= j < n */
  for (j = 0; j < n; j++)
    {
      mpz_gcd (T[0][j], T[0][j], R[perm[j]]);
      mpz_divexact (R[perm[j]], R[perm[j]], T[0][j]);
    }

  clear_product_tree (T, n, w);
}

/* invariant:
   relations 0 to *nb_smooth-1 are smooth
   relations *nb_smooth to *nb_unknown-1 are unknown
   relations >= *nb_unknown are non-smooth (useless).
*/
static void
update_status (mpz_t *R, uint32_t *perm,
               unsigned char *b_status_r, unsigned char *b_status_a,
               unsigned long *nb_smooth, unsigned long *nb_unknown)
{
  unsigned long i, j;

  for (j = *nb_smooth; j < *nb_unknown; j++)
    {
      i = perm[j];
      if (b_status_r[i] == STATUS_UNKNOWN)
      {
        /* if R[i] < L, then R[i] is smooth (we assume L <= B^2) */
        if (mpz_cmp_ui (R[i], 1) == 0)
          {
            b_status_r[i] = STATUS_SMOOTH;
            if (b_status_a[i] == STATUS_SMOOTH)
              {
                /* relation j is smooth, swap it with relation *nb_smooth */
                perm[j] = perm[*nb_smooth];
                perm[*nb_smooth] = i;
                (*nb_smooth)++;
              }
          }
        else /* not smooth */
          {
            /* relation j is useless, swap it with relation *nb_unknown - 1 */
            (*nb_unknown)--;
            perm[j] = perm[*nb_unknown];
            perm[*nb_unknown] = i;
            j--;
          }
      }
    }
}

/* return the number n of smooth relations in l,
   which should be at the end in locations perm[0], perm[1], ..., perm[n-1] */
unsigned long
find_smooth (cofac_list l, int lpb[2], unsigned long lim[2], mpz_t batchP[2],
             FILE *out)
{
  unsigned long nb_rel_read = l->size;
  unsigned long nb_smooth;
  unsigned long nb_unknown;
  unsigned char *b_status_r;
  unsigned char *b_status_a;
  double start = seconds ();

  b_status_r = (unsigned char *) malloc (nb_rel_read * sizeof(unsigned char));
  b_status_a = (unsigned char *) malloc (nb_rel_read * sizeof(unsigned char));
  memset (b_status_r, STATUS_UNKNOWN, nb_rel_read);
  memset (b_status_a, STATUS_UNKNOWN, nb_rel_read);

  nb_smooth = 0;
  nb_unknown = nb_rel_read;

  /* the code below assumes lim0 <= 2^lpb0 and lim1 <= 2^lpb1 */
  ASSERT_ALWAYS(lim[0] <= (1UL << lpb[0]));
  ASSERT_ALWAYS(lim[1] <= (1UL << lpb[1]));

  /* invariant: the smooth relations are in 0..nb_smooth-1,
     the unknown ones in nb_smooth..nb_unknown-1,
     the remaining ones are not smooth */

  /* it seems faster to start from the algebraic side */
  for (int z = 1; z >= 0; z--)
    {
      if (z == 0)
        smoothness_test (l->R, l->perm + nb_smooth, nb_unknown - nb_smooth,
                         batchP[0]);
      else
        smoothness_test (l->A, l->perm + nb_smooth, nb_unknown - nb_smooth,
                         batchP[1]);

      /* we only need to update relations in [nb_smooth, nb_unknown-1] */
      if (z == 0)
        update_status (l->R, l->perm, b_status_r, b_status_a,
                       &nb_smooth, &nb_unknown);
      else
        update_status (l->A, l->perm, b_status_a, b_status_r,
                       &nb_smooth, &nb_unknown);
    }

  free (b_status_r);
  free (b_status_a);

  fprintf (out, "# batch: took %.1fs to detect %lu smooth relations out of %lu\n", seconds () - start, nb_smooth, nb_rel_read);

  return nb_smooth;
}

/* print all factors of n, and return the new value of m */
static int
print_smooth_aux (mpz_t *factors, mpz_t n, facul_method_t *methods,
                  struct modset_t *fm, struct modset_t *cfm,
                  int lpb, double BB, double BBB, int m, FILE *out)
{
  unsigned long i;
  int j, res_fac;

  /* any factor < B^2 is necessarily prime */
  for (i = 0; methods[i].method != 0 && mpz_cmp_d (n, BB) >= 0; i++)
    {
      res_fac = facul_doit_onefm_mpz (factors, n, methods[i], fm, cfm,
                                      lpb, BB, BBB);

      ASSERT_ALWAYS(res_fac != FACUL_NOT_SMOOTH);

      /* factors[0..res_fac-1] are prime factors of n, 0 <= res_fac <= 2 */

      for (j = 0; j < res_fac; j++)
        {
          if (m++ > 0)
            fprintf (out, ",");
          gmp_fprintf (out, "%Zx", factors[j]);
          mpz_divexact (n, n, factors[j]);
        }

      if (fm->arith != CHOOSE_NONE) /* fm is a non-trivial composite factor */
        {
          mpz_t t;
          struct modset_t cfm2;
          mpz_init (t);
          modset_get_z (t, fm);
          /* t should be composite, i.e., t >= BB */
          ASSERT(mpz_cmp_d (t, BB) >= 0);
          mpz_divexact (n, n, t);
          m = print_smooth_aux (factors, t, methods + i + 1, fm, &cfm2,
                                lpb, BB, BBB, m, out);
          modset_clear (fm);
          fm->arith = CHOOSE_NONE;
          mpz_clear (t);
        }

      if (cfm->arith != CHOOSE_NONE)
        {
          mpz_t t;
          struct modset_t fm2;
          mpz_init (t);
          modset_get_z (t, cfm);
          /* t should be composite, i.e., t >= BB */
          ASSERT(mpz_cmp_d (t, BB) >= 0);
          mpz_divexact (n, n, t);
          m = print_smooth_aux (factors, t, methods + i + 1, &fm2, cfm,
                                lpb, BB, BBB, m, out);
          modset_clear (cfm);
          cfm->arith = CHOOSE_NONE;
          mpz_clear (t);
        }
    }

  if (mpz_cmp_ui (n, 1) > 0)
    {
      ASSERT_ALWAYS (mpz_cmp_d (n, BB) < 0);
      if (m++ > 0)
        fprintf (out, ",");
      gmp_fprintf (out, "%Zx", n);
    }

  return m;
}

static int
trial_divide (mpz_t n, unsigned long *sp, unsigned long spsize, FILE *out)
{
  int m = 0; /* number of already printed factors */
  unsigned long i;

  for (i = 0; i < spsize; i++)
    {
      while (mpz_divisible_ui_p (n, sp[i]))
        {
          if (m++ > 0)
            fprintf (out, ",");
          fprintf (out, "%lx", sp[i]);
          mpz_divexact_ui (n, n, sp[i]);
        }
    }

  return m;
}

/* Print the prime factor of the input 'n' separated by spaces.
   The list SP (small primes) contains all primes < B.
   BB is the prime bound: any factor < BB is necessarily prime.
   'hint' is either 1 or a product of large primes.
   'cofac' is the initial cofactor (without special-q).
*/
static void
print_smooth (mpz_t *factors, mpz_t n, facul_method_t *methods,
              struct modset_t *fm, struct modset_t *cfm,
              unsigned int lpb, double BB, double BBB, unsigned long *sp,
              unsigned long spsize, mpz_t hint, mpz_t cofac, mpz_ptr sq,
              FILE *out)
{
  int m; /* number of already printed factors */

  ASSERT_ALWAYS(mpz_divisible_p (n, cofac));
  mpz_divexact (n, n, cofac);

  ASSERT_ALWAYS(mpz_divisible_p (cofac, hint));
  mpz_divexact (cofac, cofac, hint);

  if (sq != NULL)
    {
      ASSERT_ALWAYS(mpz_divisible_p (n, sq));
      mpz_divexact (n, n, sq);
    }

  /* remove small primes */
  m = trial_divide (n, sp, spsize, out);

  /* factor hint */
  m = print_smooth_aux (factors, hint, methods, fm, cfm, lpb, BB, BBB, m, out);

  /* factor rest of cofactor */
  m = print_smooth_aux (factors, cofac, methods, fm, cfm, lpb, BB, BBB, m, out);

  /* factor rest of factor base primes */
  print_smooth_aux (factors, n, methods, fm, cfm, lpb, BB, BBB, m, out);

  if (sq != NULL)
    {
      if (m)
        fprintf (out, ",");
      gmp_fprintf (out, "%Zx", sq);
    }
}

/* strip integers in l[0..n-1] which do not divide P */
unsigned long
strip (unsigned long *l, unsigned long n, mpz_t P)
{
  unsigned long i, j;

  for (i = j = 0; i < n; i++)
    if (mpz_divisible_ui_p (P, l[i]))
      l[j++] = l[i];
  return j;
}

/* Given a list L of bi-smooth cofactors, print the corresponding relations
   on "out".
   n is the number of bi-smooth cofactors in L.
*/
void
factor (cofac_list L, unsigned long n, cado_poly pol, int lpb0,
        int lpb1, FILE *out)
{
  unsigned long i, pmax, *sp0, *sp1, spsize[2];
  int nb_methods;
  facul_method_t *methods;
  mpz_t Q[2];
  struct modset_t fm, cfm;
  double BB, BBB;
  mpz_list SP;
  prime_info pi;
  double start = seconds ();
  mpz_t *norm0, *norm1;
  uint32_t *perm = L->perm;

  /* we trial divide by all primes < L^(1/2), so that any factor < L
     is necessarily prime */
  pmax = (lpb1 > lpb0) ? lpb1 : lpb0;
  pmax = (unsigned long) ceil (pow (2.0, (double) pmax / 2.0));
  pmax = 16 * pmax;

  mpz_init (Q[0]);
  mpz_init (Q[1]);

  mpz_list_init (SP);
  prime_info_init (pi);
  prime_list_poly (SP, pi, 2, pmax, pol->pols[0]);
  spsize[0] = SP->size;
  sp0 = malloc (spsize[0] * sizeof (unsigned long));
  for (i = 0; i < spsize[0]; i++)
    sp0[i] = mpz_get_ui (SP->l[i]);
  prime_info_clear (pi);
  mpz_list_clear (SP);

  mpz_list_init (SP);
  prime_info_init (pi);
  prime_list_poly (SP, pi, 2, pmax + pmax / 2, pol->pols[1]);
  spsize[1] = SP->size;
  sp1 = malloc (spsize[1] * sizeof (unsigned long));
  for (i = 0; i < spsize[1]; i++)
    sp1[i] = mpz_get_ui (SP->l[i]);
  prime_info_clear (pi);
  mpz_list_clear (SP);

  nb_methods = 30;
  if (nb_methods >= NB_MAX_METHODS)
    nb_methods = NB_MAX_METHODS - 1;
  methods = facul_make_default_strategy (nb_methods - 3, 0);

  BB = (double) pmax * (double) pmax;
  BBB = BB * (double) pmax;

  /* compute all norms F(a,b) and G(a,b) */
  norm0 = malloc (n * sizeof (mpz_t));
  norm1 = malloc (n * sizeof (mpz_t));
  for (i = 0; i < n; i++)
    {
      mpz_init (norm0[i]);
      mpz_init (norm1[i]);
      mpz_poly_homogeneous_eval_siui (norm0[i], pol->pols[0],
                                      L->a[perm[i]], L->b[perm[i]]);
      mpz_poly_homogeneous_eval_siui (norm1[i], pol->pols[1],
                                      L->a[perm[i]], L->b[perm[i]]);
    }

  for (i = 0; i < n; i++)
    {
      fprintf (out, "%" PRId64 ",%" PRIu64 ":", L->a[perm[i]], L->b[perm[i]]);

      print_smooth (Q, norm0[i], methods, &fm, &cfm, lpb0, BB, BBB, sp0,
                    spsize[0], L->R[perm[i]], L->R0[perm[i]], NULL, out);
      fprintf (out, ":");

      print_smooth (Q, norm1[i], methods, &fm, &cfm, lpb1, BB, BBB, sp1,
                    spsize[1], L->A[perm[i]], L->A0[perm[i]], L->sq[perm[i]],
                    out);
      fprintf (out, "\n");
      fflush (out);
      mpz_clear (norm0[i]);
      mpz_clear (norm1[i]);
    }
  free (norm0);
  free (norm1);

  mpz_clear (Q[0]);
  mpz_clear (Q[1]);

  free (sp0);
  free (sp1);
  facul_clear_aux_methods (methods);

  fprintf (out, "# batch: took %.1fs to factor %lu smooth relations\n",
           seconds () - start, n);
}

static void
create_batch_product (mpz_t P, unsigned long B, unsigned long L,
mpz_poly_t pol)
{
  prime_info pi;
  unsigned long p;

  ASSERT_ALWAYS (L > B);

  prime_info_init (pi);

  for (p = 2; p < B; p = getprime_mt (pi));

  prime_product_poly (P, pi, L, p, pol);

  prime_info_clear (pi);
}

/* We have 3 cases:
   1) if f == NULL: P is computed but not stored
   2) if f != NULL but file is non-existing: P is computed and saved in f
   3) if f != NULL and file is existing: P is read from file
*/
void
create_batch_file (const char *f, mpz_t P, unsigned long B, unsigned long L,
                   mpz_poly_t pol, FILE *out)
{
  FILE *fp;
  double s = seconds ();

  fprintf (out, "# batch: creating or reading large prime product");
  fflush (out);

  if (f == NULL) /* case 1 */
    {
      create_batch_product (P, B, L, pol);
      goto end;
    }

  fp = fopen (f, "r");
  if (fp != NULL) /* case 3 */
    {
      int ret = gmp_fscanf (fp, "%Zx\n", P);
      ASSERT_ALWAYS(ret == 1);
      goto end;
    }

  /* case 2 */
  create_batch_product (P, B, L, pol);

  fp = fopen (f, "w");
  ASSERT_ALWAYS(fp != NULL);

  /* gmp_fprintf is buggy in GMP <= 6.1.0 for numbers of more than 2^33-8 bits:
     https://gmplib.org/list-archives/gmp-bugs/2015-November/003794.html */
  ASSERT_ALWAYS((mpz_sizeinbase (P, 2) + 1) / 2 <= 4294967292UL);
  gmp_fprintf (fp, "%Zx\n", P);

  fclose (fp);

 end:
  fprintf (out, " took %.0fs\n", seconds () - s);
  fflush (out);
}

