#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fppol_io.h"
#include "fppol_internal.h"



/* Fixed-size polynomials.
 *****************************************************************************/

// Masks for interleaving bit vectors.
static const uint64_t __mask[][4] = {
  {         0x55555555ul,         0x33333333ul,
            0x0f0f0f0ful,         0x00ff00fful },
  {     0x249249249249ul,     0x0c30c30c30c3ul,
        0x00f00f00f00ful,     0x0000ff0000fful },
  { 0x1111111111111111ul, 0x0303030303030303ul,
    0x000f000f000f000ful, 0x000000ff000000fful },
};


// Value of hexadecimal digits.
#define X 0xff
static const unsigned char __digit_val[] = {
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, X, X, X, X, X, X,
  X,10,11,12,13,14,15, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X,10,11,12,13,14,15, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
};
#undef X


// Conversion to string.
#define __DEF_FPPOLxx_GET_STR(sz)                                     \
  char *fppol##sz##_get_str(char *str, fppol##sz##_srcptr p)          \
  {                                                                   \
    int d = fppol##sz##_deg(p);                                       \
    if (str == NULL) {                                                \
      str = (char *)malloc((fppol##sz##_strlen(p)+1) * sizeof(char)); \
      ASSERT_ALWAYS(str != NULL);                                     \
    }                                                                 \
    char *ptr = str;                                                  \
    if (d < 0) { sprintf(ptr, "0"); return str; }                     \
    for (unsigned i = (d>>4)+1, n = 0; i--; n = 1) {                  \
      uint64_t w = 0, t;                                              \
      for (unsigned k = 0; k < __FP_BITS; ++k) {                      \
        t = (p[k] >> (i<<4)) & 0xfffful;                              \
        for (unsigned i = 4, j = __FP_BITS-1; j && i--; )             \
          t = (t | (t << (j<<i))) & __mask[j-1][i];                   \
        w |= t << k;                                                  \
      }                                                               \
      ptr += sprintf(ptr, "%0*lx", n ? __FP_BITS<<2 : 0, w);          \
    }                                                                 \
    return str;                                                       \
  }


// Conversion from string.
// Return 1 if successful.
#define __DEF_FPPOLxx_SET_STR(sz)                                \
  int fppol##sz##_set_str(fppol##sz##_ptr r, const char *str)    \
  {                                                              \
    fppol##sz##_set_zero(r);                                     \
    for (unsigned i = strlen(str), n = 0; i; n += 16) {          \
      uint64_t w = 0, t;                                         \
      for (unsigned m = 0; i && m < __FP_BITS<<4; ) {            \
        if (isspace(str[--i])) continue;                         \
        unsigned char c = __digit_val[(unsigned char)str[i]];    \
        if (c > 0xf) return 0;                                   \
        w |= (uint64_t)c << m;                                   \
        m += 4;                                                  \
      }                                                          \
      if (!w) continue;                                          \
      if (n >= sz || (sz-n <= 16 && w >> (__FP_BITS*(sz-n))))    \
        return 0;                                                \
      for (unsigned k = 0; k < __FP_BITS; ++k) {                 \
        t = w >> k;                                              \
        for (unsigned i = 0, j = __FP_BITS-1; j && i < 4; ++i) { \
          t &= __mask[j-1][i];                                   \
          t |= t >> (j<<i);                                      \
        }                                                        \
        r[k] |= (t & 0xfffful) << n;                             \
      }                                                          \
    }                                                            \
    return fppol##sz##_is_valid(r);                              \
  }


// Output to stream.
#define __DEF_FPPOLxx_OUT(sz)                         \
  void fppol##sz##_out(FILE *f, fppol##sz##_srcptr p) \
  {                                                   \
    static __thread char buf[__FP_BITS*(sz>>2)+1];    \
    fppol##sz##_get_str(buf, p);                      \
    fprintf(f == NULL ? stdout : f, "%s", buf);       \
  }


// Input from stream.
// Return 1 if successful.
#define __DEF_FPPOLxx_INP(sz)                                   \
  int fppol##sz##_inp(fppol##sz##_ptr r, FILE *f)               \
  {                                                             \
    static __thread char buf[__FP_BITS*(sz>>2)+1];              \
    int c;                                                      \
    unsigned n;                                                 \
    if (f == NULL) f = stdin;                                   \
    for (; isspace(c = getc(f)); );                             \
    if (c == EOF || __digit_val[c] > 0xf) return 0;             \
    for (; c == '0'; c = getc(f));                              \
    for (n = 0; c != EOF && __digit_val[c] <= 0xf; c = getc(f)) \
      if (n < sizeof(buf)) buf[n++] = (char)c;                  \
    ungetc(c, f);                                               \
    if (n >= sizeof(buf)) return 0;                             \
    buf[n] = '\0';                                              \
    return fppol##sz##_set_str(r, buf);                         \
  }


// All definitions bundled up into a single macro.
#define __DEF_FPPOLxx_IO_ALL(sz)  \
        __DEF_FPPOLxx_GET_STR(sz) \
        __DEF_FPPOLxx_SET_STR(sz) \
        __DEF_FPPOLxx_OUT    (sz) \
        __DEF_FPPOLxx_INP    (sz)

__DEF_FPPOLxx_IO_ALL( 8)
__DEF_FPPOLxx_IO_ALL(16)
__DEF_FPPOLxx_IO_ALL(32)
__DEF_FPPOLxx_IO_ALL(64)

#undef __DEF_FPPOLxx_GET_STR
#undef __DEF_FPPOLxx_SET_STR
#undef __DEF_FPPOLxx_OUT
#undef __DEF_FPPOLxx_INP
#undef __DEF_FPPOLxx_IO_ALL



/* Multiprecision polynomials.
 *****************************************************************************/

// Conversion to string.
char *fppol_get_str(char *str, fppol_srcptr p)
{
  static __thread char buf[__FP_BITS*16+1];
  int      d = fppol_deg(p);
  unsigned l = fppol_strlen(p), ll;
  if (str == NULL) {
    str = (char *)malloc((l+1) * sizeof(char));
    ASSERT_ALWAYS(str != NULL);
  }
  memset(str, '0', l); str[l] = '\0';
  for (int k = 0; k <= d>>6; ++k, l -= __FP_BITS*16) {
    fppol64_get_str(buf, p->limbs[k]);
    ll = strlen(buf);
    memcpy(str+l-ll, buf, ll);
  }
  return str;
}


// Conversion from string.
// Return 1 if successful.
int fppol_set_str(fppol_ptr r, const char *str)
{
  if (strlen(str) == 0)
    return 0;
  static __thread char buf[__FP_BITS*16+1];
  unsigned n        = 0;
  buf[__FP_BITS*16] = '\0';
  __fppol_realloc_lazy(r, (strlen(str)*4+__FP_BITS-1) / __FP_BITS);
  for (unsigned i = strlen(str), j; i; ) {
    for (j = __FP_BITS*16; i && j; ) {
      if (isspace(str[--i])) continue;
      buf[--j] = str[i];
    }
    if (j < __FP_BITS*16) {
      if (!fppol64_set_str(r->limbs[n++], buf+j))
        return 0;
    }
  }
  r->deg = (n<<6) - 1;
  __fppol_update_degree(r);
  return 1;
}


// Output to stream.
void fppol_out(FILE *f, fppol_srcptr p)
{
  char *buf = fppol_get_str(NULL, p);
  fprintf(f == NULL ? stdout : f, "%s", buf);
  free(buf);
}


// FIXME: does this function works when parsing "0" ???
// Input from stream.
// Return 1 if successful.
int fppol_inp(fppol_ptr r, FILE *f)
{
  unsigned  alloc = __FP_SIZE*16, n;
  char     *buf;
  int       c;
  if (f == NULL) f = stdin;
  for (; isspace(c = getc(f)); );
  if (c == EOF || __digit_val[c] > 0xf) return 0;
  buf = (char *)malloc((alloc+1) * sizeof(char));
  ASSERT_ALWAYS(buf != NULL);
  for (; c == '0'; c = getc(f));
  for (n = 0; c != EOF && __digit_val[c] <= 0xf; c = getc(f)) {
    if (n >= alloc) {
      alloc *= 2;
      buf    = (char *)realloc(buf, (alloc+1) * sizeof(char));
    }
    buf[n++] = (char)c;
  }
  ungetc(c, f);
  buf[n] = '\0';
  int rc = fppol_set_str(r, buf);
  free(buf);
  return rc;
}
