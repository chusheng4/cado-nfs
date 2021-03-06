/**
 * @file ropt_param.c
 * Holds extern vars. The customizable parameters 
 * are in ropt_param.h.
 */


#include "cado.h"
#include "ropt_param.h"


/**
 * Default values for L1 cache size and tune sieve length. The code
 * will also try to auto-detect them.
 */
unsigned int L1_cachesize = 12288;
unsigned int size_tune_sievearray = 6144;


/* ------------------
   possible to change
   ------------------ */


/**
 * Total number of sublattices in the tuning and sieving steps.
 * If with the default parametes (SIZE_SIEVEARRAY_V_MAX 4194304),
 * each root sieve takes about 2-4 seconds. The first column is 
 * ranked by the digits of integers to be factored and the right
 * column is the number (actually number+1).
 *
 * Usually, there is one or more tuning steps before the final root
 * sieve. In that case, more sublattices are checked (e.g. double/quad
 * the following valeus).
 * 
 * The number is linearly scaled by param->effort, but a larger value
 * is not always necessary since the sieving is done in order with
 * the best sublattices first.
 */
const unsigned int size_total_sublattices[8][2] = {
  /* {digits, num_of_sublattices} */
  {80,  4},  /* for up to 79 digits */
  {100, 8},  /* up to 99 digits */
  {140, 16},  /* up to 139 digits */
  {170, 32}, /* up to 169 digits */
  {180, 64}, /* up to 179 digits */
  {220, 128}, /* up to 219 digits */
  {260, 256}, /* up to 259 digits */
  {300, 512} /* up to 299 digits */
};


/**
 * Number of top sublattice for individual primes[i] in stage 1,
 * where i < NUM_SUBLATTICE_PRIMES. The constrcution should depends
 * on the total num of primes in s1param->e_sl[]. 
 * The main purpose is to prevent too much crt computations in stage 1.
 * They will be passed to s1param->individual_nbest_sl[] later.
 */
const unsigned int
s1_size_each_sublattice[NUM_SUBLATTICE_PRIMES][NUM_SUBLATTICE_PRIMES] = {
  { 64,  0,  0,  0,  0,  0,  0,  0,  0 }, // tlen_e_sl = 1
  { 64, 64,  0,  0,  0,  0,  0,  0,  0 }, // tlen_e_sl = 2
  { 64, 64, 64,  0,  0,  0,  0,  0,  0 }, // tlen_e_sl = 3
  { 64, 32, 32, 32,  0,  0,  0,  0,  0 }, // tlen_e_sl = 4
  { 32, 32, 16,  8,  8,  0,  0,  0,  0 }, // tlen_e_sl = 5
  { 32, 16, 16,  8,  8,  4,  0,  0,  0 }, // tlen_e_sl = 6 (current bound)
  { 32, 16,  8,  8,  4,  4,  4,  0,  0 }, // tlen_e_sl = 7
  { 32, 16,  8,  8,  4,  4,  4,  2,  0 }, // tlen_e_sl = 8
  { 32, 16,  8,  8,  4,  4,  4,  2,  2 }, // tlen_e_sl = 9
};


/**
 * As above, but it's only used for tuning good w in 'ropt_quadratic.c'.
 * Therefore, the values are much smaller than above.
 */
const unsigned int
s1_size_each_sublattice_tune[NUM_SUBLATTICE_PRIMES] = {
  8,  8,  4,  4,  4,  2,  2,  2,  2
};


/* -------------------------
   perhaps no need to change
   ------------------------- */


/**
 * Default parameters for sublattice p_i^e^i.
 * Non-decreasing due to ropt_s1param_setup() in ropt_str.c
 */
const unsigned int 
default_sublattice_pe[NUM_DEFAULT_SUBLATTICE][NUM_SUBLATTICE_PRIMES] = {
  /* 2, 3, 5, 7, 11, 13, 17, 19, 23 */
  { 1, 0, 0, 0, 0, 0, 0, 0, 0 }, // 2
  { 2, 0, 0, 0, 0, 0, 0, 0, 0 }, // 4
  { 2, 1, 0, 0, 0, 0, 0, 0, 0 }, // 12
  { 3, 1, 0, 0, 0, 0, 0, 0, 0 }, // 24
  { 4, 1, 0, 0, 0, 0, 0, 0, 0 }, // 48
  { 3, 2, 0, 0, 0, 0, 0, 0, 0 }, // 72
  { 3, 1, 1, 0, 0, 0, 0, 0, 0 }, // 120
  { 4, 2, 0, 0, 0, 0, 0, 0, 0 }, // 144
  { 4, 1, 1, 0, 0, 0, 0, 0, 0 }, // 240
  { 5, 2, 0, 0, 0, 0, 0, 0, 0 }, // 288
  { 3, 2, 1, 0, 0, 0, 0, 0, 0 }, // 360
  { 5, 1, 1, 0, 0, 0, 0, 0, 0 }, // 480
  { 4, 2, 1, 0, 0, 0, 0, 0, 0 }, // 720
  { 5, 2, 1, 0, 0, 0, 0, 0, 0 }, // 1440
  { 5, 3, 1, 0, 0, 0, 0, 0, 0 }, // 4320
  { 5, 2, 1, 1, 0, 0, 0, 0, 0 }, // 10080
  { 6, 2, 1, 1, 0, 0, 0, 0, 0 }, // 20160
  { 5, 3, 2, 0, 0, 0, 0, 0, 0 }, // 21600
  { 5, 2, 2, 1, 0, 0, 0, 0, 0 }, // 50400
  { 6, 2, 2, 1, 0, 0, 0, 0, 0 }, // 100800
  { 6, 3, 2, 1, 0, 0, 0, 0, 0 }, // 302400
  { 6, 3, 2, 1, 1, 0, 0, 0, 0 }, // 3326400
  { 7, 3, 2, 1, 1, 0, 0, 0, 0 }, // 6652800
  { 8, 3, 2, 1, 1, 0, 0, 0, 0 }, // 13305600
  { 7, 4, 2, 1, 1, 0, 0, 0, 0 }, // 19958400
  { 6, 3, 2, 1, 1, 1, 0, 0, 0 }, // 43243200
};


/**
 * Hard-coded product for above default_sublattice_pe.
 */
const unsigned long
default_sublattice_prod[NUM_DEFAULT_SUBLATTICE] = {
  2, 4, 12, 24, 48, 72, 120, 144, 240, 288, 360, 480,
  720, 1440, 4320, 10080, 20160, 21600, 50400, 100800,
  302400, 3326400, 6652800, 13305600, 19958400, 43243200,
};


/**
 * Primes.
 */
const unsigned int primes[NP] = {
  2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
  31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
  73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
  127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
  179, 181, 191, 193, 197, 199
};


/**
 * next_prime_idx[3] = ind(5) = 2 in above table.
 */
const unsigned char next_prime_idx[] = {
  0, // 0
  0, 1, 2, 2, 3, 3, 4, 4, 4, 4, // 1 - 10
  5, 5, 6, 6, 6, 6, 7, 7, 8, 8,
  8, 8, 9, 9, 9, 9, 9, 9, 10, 10,
  11, 11, 11, 11, 11, 11, 12, 12, 12, 12,
  13, 13, 14, 14, 14, 14, 15, 15, 15, 15,
  15, 15, 16, 16, 16, 16, 16, 16, 17, 17,
  18, 18, 18, 18, 18, 18, 19, 19, 19, 19,
  20, 20, 21, 21, 21, 21, 21, 21, 22, 22,
  22, 22, 23, 23, 23, 23, 23, 23, 24, 24,
  24, 24, 24, 24, 24, 24, 25, 25, 25, 25,
  26, 26, 27, 27, 27, 27, 28, 28, 29, 29,
  29, 29, 30, 30, 30, 30, 30, 30, 30, 30,
  30, 30, 30, 30, 30, 30, 31, 31, 31, 31,
  32, 32, 32, 32, 32, 32, 33, 33, 34, 34,
  34, 34, 34, 34, 34, 34, 34, 34, 35, 35,
  36, 36, 36, 36, 36, 36, 37, 37, 37, 37,
  37, 37, 38, 38, 38, 38, 39, 39, 39, 39,
  39, 39, 40, 40, 40, 40, 40, 40, 41, 41,
  42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
  43, 43, 44, 44, 44, 44, 45, 45 // 191 - 198
};


/**
 * Asymptotic estimate of minimum order statistics
 * for 2^K many rotations where 0 <= K <= 149.
 * function expected_alpha_est() in alpha.sage
 */
double exp_alpha (double logK) 
{
  if (logK < 0.999)
    return 0.0;
  return MU - SIGMA * (sqrt(2*logK)-(log(logK)+1.3766)/(2*sqrt(2*logK)));
}


