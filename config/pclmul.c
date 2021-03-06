#include <stdint.h>
#include <wmmintrin.h>
#include <assert.h>

int main() {
    assert(sizeof(unsigned long) == 8); /* assume 64-bit */
#if defined(__GNUC__) && __GNUC__ == 4 &&__GNUC_MINOR__ == 1
#define _gf2x_mm_cvtsi64_m64(u) _mm_cvtsi64x_m64((u))
#else
#define _gf2x_mm_cvtsi64_m64(u) _mm_cvtsi64_m64((u))
#endif
    /* _m128i from 2 int64_t's */
#define _gf2x_mm_setr_epi64(lo, hi)                                     \
    _mm_setr_epi64(                                                     \
            _gf2x_mm_cvtsi64_m64((int64_t) (lo)),                       \
            _gf2x_mm_cvtsi64_m64((int64_t) (hi))                        \
            )
    /* _m128i from 1 int64_t's */
#define _gf2x_mm_set1_epi64(u) _mm_set1_epi64( _gf2x_mm_cvtsi64_m64((int64_t) (u)))
    __m128i a = _gf2x_mm_set1_epi64(17);
    __m128i b = _gf2x_mm_set1_epi64(42);
    union { __m128i s; unsigned long x[2]; } proxy;
    proxy.s = _mm_clmulepi64_si128(a, b, 0);
    return proxy.x[0] - 650;
}
