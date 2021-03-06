/* This source file is our test case for sse-3 support. */
#include <stdint.h>
#include <string.h>
#include <pmmintrin.h>

int main()
{
    __m128d x = _mm_setr_pd(12.34, 34.12);
    __m128d y = _mm_setr_pd(78.56, 56.78);
    double a[2], b[2] = { 78.56 + 56.78, 12.34 + 34.12 };

    y = _mm_hadd_pd(y, x);
    memcpy(a, &y, 16);
    return (a[0] != b[0] || a[1] != b[1]);
}
