#ifndef MATMUL_SUB_VSC_COMBINE_H_
#define MATMUL_SUB_VSC_COMBINE_H_

#include "abase.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void matmul_sub_vsc_combine(abdst_field x, abt * dst, const abt * * mptrs, const uint8_t * q, unsigned long count, unsigned int defer);

#ifdef __cplusplus
}
#endif

#endif	/* MATMUL_SUB_VSC_COMBINE_H_ */
