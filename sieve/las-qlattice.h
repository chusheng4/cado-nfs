#ifndef LAS_QLATTICE_H_
#define LAS_QLATTICE_H_

#include <stdint.h>
#include <gmp.h>
#include "fb-types.h"         /* fbprime_t */
#include "las-base.hpp"
#include "portability.h"

/* implementations for inlines */
#include "las-arith.h"

struct qlattice_basis : private NonCopyable {
    int64_t a0, b0, a1, b1;
    mpz_t q;
    qlattice_basis(){mpz_init(q);}
    ~qlattice_basis(){mpz_clear(q);}
    void set_q(const mpz_t special_q) {
      /* Currently requires prime special-q values.
         For powers, the base prime would have to be determined and stored in
         a variable, so that powers of that prime in the factor base can be
         skipped over. For composite special-q, a list of primes would have to
         be stored and skipped. */
      ASSERT_ALWAYS(!mpz_perfect_power_p(special_q));
      mpz_set(q, special_q);
    };
};

int SkewGauss (qlattice_basis &, const mpz_t, const mpz_t, double);

static inline fbprime_t
fb_root_in_qlattice_31bits (const fbprime_t p, const fbprime_t R,
        const uint32_t invp, const qlattice_basis &basis);
#ifdef  HAVE_redc_64
static inline fbprime_t
fb_root_in_qlattice_63bits (const fbprime_t p, const fbprime_t R,
        const uint64_t invp, const qlattice_basis &basis);
#endif


/* fb_root_in_qlattice returns (R*b1-a1)/(a0-R*b0) mod p */
#if defined(SUPPORT_LARGE_Q)
#ifndef  HAVE_redc_64
#error  "Please implement redc_64"
#else
/* The reason why the special-q is constrained to some limit is quite
 * clearly linked to the fb_root_in_qlattice variant being used. However,
 * it does not seem to be exactly 31 or 63 bits. This should be
 * investigated */
#define MAX_SPECIALQ_BITSIZE    60
static inline fbprime_t
fb_root_in_qlattice(const fbprime_t p, const fbprime_t R,
        const redc_invp_t invp, const qlattice_basis &basis);
static inline fbprime_t
fb_root_in_qlattice(const fbprime_t p, const fbprime_t R,
        const redc_invp_t invp, const qlattice_basis &basis)
{
    return fb_root_in_qlattice_63bits(p, R, invp, basis);
}
#endif

#else

#define MAX_SPECIALQ_BITSIZE    30
static inline fbprime_t
fb_root_in_qlattice(const fbprime_t p, const fbprime_t R,
        const redc_invp_t invp, const qlattice_basis &basis);
static inline fbprime_t
fb_root_in_qlattice(const fbprime_t p, const fbprime_t R,
        const redc_invp_t invp, const qlattice_basis &basis)
{
    return fb_root_in_qlattice_31bits(p, R, invp, basis);
}
#endif

/* The version fb_root_in_qlattice_31bits mandates that the coordinates
 * of the q-lattice are at most 31 bits, so that combinations such as
 * Rb1-a1 always fit within the interval ]-2^32p, +2^32p[
 */

/* This helper function is used for powers of 2. See below */
static inline fbprime_t
fb_root_in_qlattice_po2 (const fbprime_t p, const fbprime_t R,
        const qlattice_basis &basis);

static inline fbprime_t
fb_root_in_qlattice_31bits (const fbprime_t p, const fbprime_t R,
        const uint32_t invp, const qlattice_basis &basis)
{
  int64_t aux1, aux2;
  uint32_t u, v;

    /* Handle powers of 2 separately, REDC doesn't like them */
  if (UNLIKELY(!(p & 1)))
    return fb_root_in_qlattice_po2(p, R, basis);

    // Use Signed Redc for the computation:
    // Numerator and denominator will get divided by 2^32, but this does
    // not matter, since we take their quotient.

  if (LIKELY(R < p)) /* Root in a,b-plane is affine */
    {
      aux1 = (int64_t)R * basis.b1 - basis.a1;
      aux2 = basis.a0 - (int64_t)R *basis.b0;
    }
  else /* Root in a,b-plane is projective */
    {
      aux1 = basis.b1 - (int64_t)(R - p) * basis.a1;
      aux2 = (int64_t)(R - p) * basis.a0 - basis.b0;
    }
  u = redc_32(aux1, p, invp); /* 0 <= u < p */
  v = redc_32(aux2, p, invp); /* 0 <= v < p */
  
  aux2 = invmod_redc_32(v, p);
  if (LIKELY(aux2)) {
    aux1 = 0;
    aux2 *= u;
  }
  else 
    {
      /* root in i,j-plane is projective */
      aux2 = invmod_redc_32(u, p);
      if (UNLIKELY(!aux2))
	{
	  fprintf (stderr, "Error, root in (i,j)-plane is projective\n");
	  exit (EXIT_FAILURE); /* Should never happen! */
	}
      aux1 = p;
      aux2 *= v;
    }
  return (fbprime_t) (redc_u32 (aux2, p, invp) + aux1);
}

#ifdef  HAVE_redc_64
/* This one is dog slow, but should be correct under the relaxed
 * condition that p be at most 63 bits or so */
static inline fbprime_t
fb_root_in_qlattice_63bits (const fbprime_t p, const fbprime_t R,
        const uint64_t invp, const qlattice_basis &basis)
{
  int64_t aux1, aux2;
  uint64_t u, v;
  
    /* Handle powers of 2 separately, REDC doesn't like them */
  if (UNLIKELY(!(p & 1 )))
    return fb_root_in_qlattice_po2(p, R, basis);
  
  if (LIKELY(R < p)) /* Root in a,b-plane is affine */
    {
      aux1 = ((int64_t)R)*basis.b1 - basis.a1;
      aux2 = basis.a0 - ((int64_t)R)*basis.b0;
    }
  else /* Root in a,b-plane is projective */
    {
      aux1 = basis.b1 - ((int64_t)(R - p))*basis.a1;
      aux2 = ((int64_t)(R - p))*basis.a0 - basis.b0;
    }
  
  /* The root in the (i,j) plane is (aux1:aux2). Now let's put it
   * in of the two forms:
   * (x:1) -> we return x=aux1/aux2.
   * (1:x), with p|x -> we return p+x = p+aux2/aux1
   *
   * (note that p is not necessarily a prime, it may be a prime power
   */
  /* Do a full 64-bit redc */
  u = redc_64(aux1, p, invp); /* 0 <= u < p */
  v = redc_64(aux2, p, invp); /* 0 <= v < p */
  
  aux2 = invmod_redc_64(v, p);
  if (LIKELY(aux2)) {
    aux1 = 0;
    aux2 *= u;
  }
  else 
    {
      /* root in i,j-plane is projective */
      aux2 = invmod_redc_64(u, p);
      if (UNLIKELY(!aux2))
	{
	  fprintf (stderr, "Error, root in (i,j)-plane is projective\n");
	  exit (EXIT_FAILURE); /* Should never happen! */
	}
      aux1 = p;
      aux2 *= v;
    }
  return (fbprime_t) (redc_64 (aux2, p, invp) + aux1);
}

#endif

/* This is just for powers of 2, and is used by both versions above */

static inline fbprime_t fb_root_in_qlattice_po2 (const fbprime_t p, const fbprime_t R, const qlattice_basis &basis)
{
    fbprime_t u, v;
    ASSERT(p == (p & -p)); /* Test that p is power of 2 */
    if (R < p) /* Root in a,b-plane is non-projective */
      {
	u = (int64_t)R * basis.b1 - basis.a1;
	v = basis.a0 - (int64_t)R * basis.b0;
      }
    else /* Root in a,b-plane is projective */
      {
        u = basis.b1 - (int64_t)(R - p) * basis.a1;
        v = (int64_t)(R - p) * basis.a0 - basis.b0;
      }
    
    if (v & 1)
      {
        /* root in i,j-plane is non-projective */
        v = invmod_po2 (v);
        return (u * v) & (p - 1);
      }
    else
      {
        /* root in i,j-plane is projective */
        u = invmod_po2 (u);
        return p + ((u * v) & (p - 1));
      }
}

#endif	/* LAS_QLATTICE_H_ */
