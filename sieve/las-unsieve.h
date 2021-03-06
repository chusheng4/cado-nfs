#ifndef LAS_UNSIEVE_H_
#define LAS_UNSIEVE_H_

#include <stdint.h>

typedef struct {
    unsigned int lpf, cof, start;
} unsieve_entry_t;

#ifdef HAVE_SSE2
#include <xmmintrin.h>
typedef __m128 unsieve_pattern_t;
#define UNSIEVE_OR(a,b) do{(a) = _mm_or_ps((a),(b));}while(0)
#else
typedef unsigned long unsieve_pattern_t;
#define UNSIEVE_OR(a,b) do{(a) |= (b);}while(0)
#endif

struct unsieve_aux_data_s {
    /* entry[i].lpf is largest prime factor of i, for i < I,
       cof is the cofactor i/lpf^k s.t. lfp^k || i,
       start is (I/2) % lpf */
    unsieve_entry_t *entries;
    unsieve_pattern_t pattern3[3];
    unsieve_pattern_t pattern5[5];
    unsieve_pattern_t pattern7[7];
};
typedef struct unsieve_aux_data_s unsieve_aux_data[1];
typedef struct unsieve_aux_data_s * unsieve_aux_data_ptr;
typedef const struct unsieve_aux_data_s * unsieve_aux_data_srcptr;

/* {{{ j_div
 * A structure for factoring the line-coordinate j into distinct odd primes
 * and performing fast divisibility tests of i by those primes.
 */
struct j_div_s {
  unsigned int p,     /* The odd prime that divides this entry */
               cof,   /* The cofactor after dividing out p as often as possible */
               inv,   /* p^(-1) (mod 2^32) */
               bound; /* (2^32-1) / p */
};
typedef struct j_div_s * j_div_ptr;
typedef const struct j_div_s * j_div_srcptr;


unsieve_aux_data_srcptr init_unsieve_data(uint32_t);
void clear_unsieve_data(unsieve_aux_data_srcptr);

j_div_srcptr init_j_div(uint32_t);
void clear_j_div(j_div_srcptr);
int  search_survivors_in_line(unsigned char * const restrict[2], const unsigned char[2], 
        unsigned int, unsigned int, int, j_div_srcptr, unsigned int,
        unsieve_aux_data_srcptr);

#endif	/* LAS_UNSIEVE_H_ */
