#ifndef MOD_MPZ_H
#define MOD_MPZ_H

#include <assert.h>
#include <limits.h>
#include <gmp.h>
#include "macros.h"

#define MODUL_MAXBITS LONG_MAX

#define ASSERT_INPUT(x) ASSERT_EXPENSIVE (mpz_cmp(m, x) > 0);
#define ASSERT_INPUT_UL(x) ASSERT_EXPENSIVE (mpz_cmp_ui(m, x) > 0);

typedef mpz_t residuempz_t;
typedef mpz_t modintmpz_t;
typedef mpz_t modulusmpz_t;

MAYBE_UNUSED
static inline void
modmpz_intset (modintmpz_t r, const modintmpz_t s)
{
  mpz_set (r, s);
}


MAYBE_UNUSED
static inline void
modmpz_intset_ul (modintmpz_t r, const unsigned long s)
{
  mpz_set_ui (r, s);
}


MAYBE_UNUSED
static inline unsigned long 
modmpz_intget_ul (const modintmpz_t s)
{
  return mpz_get_ui(s);
}


MAYBE_UNUSED
static inline int
modmpz_intequal (const modintmpz_t a, const modintmpz_t b)
{
  return (mpz_cmp(a,b) == 0);
}


MAYBE_UNUSED
static inline int
modmpz_intequal_ul (const modintmpz_t a, const unsigned long b)
{
  return (mpz_cmp_ui (a,b) == 0);
}


MAYBE_UNUSED
static inline int
modmpz_intcmp (const modintmpz_t a, const modintmpz_t b)
{
  return (mpz_cmp(a,b));
}


MAYBE_UNUSED
static inline int
modmpz_intcmp_ul (const modintmpz_t a, const unsigned long b)
{
  return (mpz_cmp_ui(a, b));
}


MAYBE_UNUSED
static inline int
modmpz_intfits_ul (const modintmpz_t a)
{
  return (mpz_fits_ulong_p(a));
}


MAYBE_UNUSED
static inline void
modmpz_intadd (modintmpz_t r, const modintmpz_t a, const modintmpz_t b)
{
  mpz_add (r, a, b);
}


MAYBE_UNUSED
static inline void
modmpz_intsub (modintmpz_t r, const modintmpz_t a, const modintmpz_t b)
{
  mpz_sub (r, a, b);
}


/* Returns the number of bits in a, that is, floor(log_2(n))+1. 
   For n==0 returns 0. */
MAYBE_UNUSED
static inline int
modmpz_intbits (const modintmpz_t a)
{
  if (mpz_sgn(a) == 0)
    return 0;
  return (mpz_sizeinbase(a, 2));
}


/* r = n/d. We require d|n */
MAYBE_UNUSED
static inline void
modmpz_intdivexact (modintmpz_t r, const modintmpz_t n, const modintmpz_t d)
{
  ASSERT_EXPENSIVE(mpz_divisible_p(n, d));
  mpz_divexact (r, n, d);
}


/* r = n%d */
MAYBE_UNUSED
static inline void
modmpz_intmod (modintmpz_t r, const modintmpz_t n, 
              const modintmpz_t d)
{
  mpz_mod (r, n, d);
}


/* Functions for the modulus */

MAYBE_UNUSED
static inline void
modmpz_initmod_ul (modulusmpz_t m, const unsigned long s)
{
  mpz_set_ui (m, s);
}


MAYBE_UNUSED
static inline void
modmpz_initmod_uls (modulusmpz_t m, const modintmpz_t s)
{
  FIXME
}


MAYBE_UNUSED
static inline unsigned long
modmpz_getmod_ul (const modulusmpz_t m)
{
  FIXME
}


MAYBE_UNUSED
static inline void
modmpz_getmod_uls (modintmpz_t r, const modulusmpz_t m)
{
  FIXME
}



MAYBE_UNUSED
static inline void
modmpz_clearmod (modulusmpz_t m MAYBE_UNUSED)
{
  mpz_clear(m);
}


/* Functions for residues */

/* Initialises a residue_t type and sets it to zero */
MAYBE_UNUSED
static inline void
modmpz_init (residuempz_t r, const modulusmpz_t m MAYBE_UNUSED)
{
  mpz_init(r);
}


/* Initialises a residue_t type, but does not set it to zero. For fixed length
   residue_t types, that leaves nothing to do at all. */
MAYBE_UNUSED
static inline void
modmpz_init_noset0 (residuempz_t r MAYBE_UNUSED, 
		   const modulusmpz_t m MAYBE_UNUSED)
{
  mpz_init(r);
}


MAYBE_UNUSED
static inline void
modmpz_clear (residuempz_t r MAYBE_UNUSED, 
             const modulusmpz_t m MAYBE_UNUSED)
{
  mpz_clear (r);
}


MAYBE_UNUSED
static inline void
modmpz_set (residuempz_t r, const residuempz_t s, 
           const modulusmpz_t m MAYBE_UNUSED)
{
  ASSERT_INPUT (s);
  mpz_set (r, s);
}


MAYBE_UNUSED
static inline void
modmpz_set_ul (residuempz_t r, const unsigned long s, const modulusmpz_t m)
{
  mpz_set_ui (r, s);
  mpz_mod (r, r, m);
}


/* Sets the residue_t to the class represented by the integer s. Assumes that
   s is reduced (mod m), i.e. 0 <= s < m */

MAYBE_UNUSED
static inline void
modmpz_set_ul_reduced (residuempz_t r, const unsigned long s, 
                      const modulusmpz_t m MAYBE_UNUSED)
{
  ASSERT_INPUT_UL (s);
  mpz_set_ui (r, s);
}


MAYBE_UNUSED
static inline void
modmpz_set_uls (residuempz_t r, const modintmpz_t s, 
		   const modulusmpz_t m)
{
  FIXME
}


MAYBE_UNUSED
static inline void
modmpz_set_uls_reduced (residuempz_t r, const modintmpz_t s, 
		       const modulusmpz_t m MAYBE_UNUSED)
{
  FIXME
}


MAYBE_UNUSED 
static inline void 
modmpz_set0 (residuempz_t r, const modulusmpz_t m MAYBE_UNUSED) 
{ 
  mpz_set_ui (r, 0);
}


MAYBE_UNUSED 
static inline void 
modmpz_set1 (residuempz_t r, const modulusmpz_t m MAYBE_UNUSED) 
{ 
  if (mpz_cmp_ui (m, 1) == 0)
    mpz_set_ui (r, 0);
  else
    mpz_set_ui (r, 1);
}


/* Exchanges the values of the two arguments */

MAYBE_UNUSED
static inline void
modmpz_swap (residuempz_t a, residuempz_t b, 
            const modulusmpz_t m MAYBE_UNUSED)
{
  mpz_swap(a, b);
}


MAYBE_UNUSED
static inline unsigned long
modmpz_get_ul (const residuempz_t s, const modulusmpz_t m MAYBE_UNUSED)
{
  ASSERT_INPUT (s);
  ASSERT_EXPENSIVE (mpz_fits_ulong(s));
  return mpz_get_ui (s);
}


MAYBE_UNUSED
static inline void
modmpz_get_uls (modintmpz_t r, const residuempz_t s, 
		   const modulusmpz_t m MAYBE_UNUSED)
{
  FIXME
}


MAYBE_UNUSED
static inline int
modmpz_equal (const residuempz_t a, const residuempz_t b, 
             const modulusmpz_t m MAYBE_UNUSED)
{
  ASSERT_INPUT (a);
  ASSERT_INPUT (b);
  return (mpz_cmp(a, b) == 0);
}


MAYBE_UNUSED
static inline int
modmpz_is0 (const residuempz_t a, const modulusmpz_t m MAYBE_UNUSED)
{
  ASSERT_INPUT (a);
  return (mpz_sgn(a) == 0);
}


MAYBE_UNUSED
static inline int
modmpz_is1 (const residuempz_t a, const modulusmpz_t m MAYBE_UNUSED)
{
  ASSERT_INPUT (a);
  return (mpz_cmp_ui(a, 1) == 0);
}


MAYBE_UNUSED
static inline void
modmpz_add (residuempz_t r, const residuempz_t a, const residuempz_t b, 
	   const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  ASSERT_INPUT (b);
  mpz_add (r, a, b);
  if (mpz_cmp(r, m) >= 0)
    mpz_sub (r, r, m);
}


MAYBE_UNUSED
static inline void
modmpz_add1 (residuempz_t r, const residuempz_t a, const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  mpz_add_ui (r, a, 1);
  if (mpz_cmp(r, m) >= 0)
    mpz_sub (r, r, m);
}


MAYBE_UNUSED
static inline void
modmpz_add_ul (residuempz_t r, const residuempz_t a, const unsigned long b, 
	      const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  mpz_add (r, a, b);
  mpz_mod (r, r, m);
}


MAYBE_UNUSED
static inline void
modmpz_sub (residuempz_t r, const residuempz_t a, const residuempz_t b, 
	   const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  ASSERT_INPUT (b);
  mpz_sub (r, a, b);
  if (mpz_sgn(r) < 0)
    mpz_add (r, r, m);
}


MAYBE_UNUSED
static inline void
modmpz_sub_ul (residuempz_t r, const residuempz_t a, const unsigned long b, 
	      const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  mpz_sub (r, a, b);
  mpz_mod (r, r, m);
}


MAYBE_UNUSED
static inline void
modmpz_neg (residuempz_t r, const residuempz_t a, const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  if (mpz_sgn(a) == 0)
    mpz_set_ui (r, 0);
  else
    mpz_sub (r, m, a);
}


MAYBE_UNUSED
static inline void
modmpz_mul (residuempz_t r, const residuempz_t a, const residuempz_t b, 
           const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  ASSERT_INPUT (b);
  mpz_mul (r, a, b);
  mpz_mod (r, r, m);
}


MAYBE_UNUSED
static inline void
modmpz_sqr (residuempz_t r, const residuempz_t a, const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  mpz_mul (r, a, b);
  mpz_mod (r, r, m);
}


MAYBE_UNUSED
static inline void
modmpz_div2 (residuempz_t r, const residuempz_t a, const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  ASSERT_EXPENSIVE (mpz_odd_p (m));
  if (mpz_even_p(a)) {
    mpz_tdiv_2exp (r, a, 1);
  } else {
    mpz_add (r, a, m);
    mpz_tdiv_2exp (r, r, 1);
  }
}


MAYBE_UNUSED
static inline int
modmpz_next (residuempz_t a, const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  mpz_add_ui (r, r, 1);
  return (mpz_cmp(r, m) == 0);
}


MAYBE_UNUSED
static inline int
modmpz_finished (const residuempz_t a, const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  return (mpz_cmp(r, m) == 0);
}


static inline int 
modmpz_div_ul (residuempz_t r, const residuempz_t a, const unsigned long b, 
              const modulusmpz_t m)
{
  mpz_t t;
  ASSERT_INPUT (a);
  mpz_init (t);
  mpz_set_ui (t, b);
  if (mpz_invert (t, t, m) == 0) {
    mpz_clear (t);
    return 0;
  }
  modmpz_mul (r, a, t, m);
  mpz_clear (t);
  return 1;
}

static inline int 
modmpz_div3 (residuempz_t r, const residuempz_t a, const modulusmpz_t m)
{
  return modmpz_div_ul(r, a, 3, m);
}

static inline int 
modmpz_div5 (residuempz_t r, const residuempz_t a, const modulusmpz_t m)
{
  return modmpz_div_ul(r, a, 5, m);
}

static inline int 
modmpz_div7 (residuempz_t r, const residuempz_t a, const modulusmpz_t m)
{
  return modmpz_div_ul(r, a, 7, m);
}

static inline int 
modmpz_div11 (residuempz_t r, const residuempz_t a, const modulusmpz_t m)
{
  return modmpz_div_ul(r, a, 11, m);
}

static inline int 
modmpz_div13 (residuempz_t r, const residuempz_t a, const modulusmpz_t m)
{
  return modmpz_div_ul(r, a, 13, m);
}

void modmpz_gcd (modintmpz_t r, const residuempz_t a, const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  mpz_gcd (r, a, m);
}

static inline void 
modmpz_pow_ul (residuempz_t r, const residuempz_t a, const unsigned long e, 
              const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  mpz_powm_ui (r, a, e, m);
}

static inline void 
modmpz_2pow_ul (residuempz_t r, const unsigned long e, const modulusmpz_t m)
{
  mpz_set_ui (r, 2);
  mpz_powm_ui (r, r, e, m);
}

static inline void 
modmpz_pow_mp (residuempz_t r, const residuempz_t a, const unsigned long *e,
              const int l, const modulusmpz_t m)
{
  mpz_t t;
  ASSERT_INPUT (a);
  mpz_init (t);
  mpz_import (t, l, -1, sizeof(unsigned long), 0, 0, e);
  mpz_powm (r, a, t, m);
  mpz_clear (t);
}

static inline void 
modmpz_2pow_mp (residuempz_t r, const unsigned long *e, const int l, 
               const modulusmpz_t m)
{
  mpz_set_ui (r, 2);
  modmpz_pow_mp (r, r, e, l, m);
}

static inlien void 
modmpz_V_mp (residuempz_t r, const residuempz_t a, const unsigned long *e,
            const int l, const modulusmpz_t m)
{
  FIXME
}

static inline void 
modmpz_V_ul (residuempz_t r, const residuempz_t a, const unsigned long e, 
                 const modulusmpz_t m)
{
  ASSERT_INPUT (a);
  modmpz_V_mp (r, a, e, l, m);
}

static inline int 
modmpz_sprp (const residuempz_t a, const modulusmpz_t m)
{
  /* FIXME */
  return mpz_probab_prime_p(m);
}

static inline int 
modmpz_sprp2 (const modulusmpz_t)
{
  return mpz_probab_prime_p(m);
}

static inline int 
modmpz_isprime (const modulusmpz_t)
{
  return mpz_probab_prime_p(m);
}

int modmpz_inv (residuempz_t r, const residuempz_t a, const modulusmpz_t m)
{
  mpz_invert (r, a, m);
}

static inline int 
modmpz_jacobi (const residuempz_t a, const modulusmpz_t m)
{
  return mpz_jacobi(a, m);
}

#endif  /* MOD_MPZ_H */