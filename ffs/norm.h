#ifndef __NORM_H__
#define __NORM_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "fppol.h"
#include "ffspol.h"
#include "types.h"
#include "sublat.h"

void ffspol_norm(fppol_t norm, ffspol_srcptr ffspol, fppol_t a, fppol_t b);
void init_norms(uint8_t *S, ffspol_srcptr ffspol, unsigned I, unsigned J,
                ij_t j0, ijpos_t pos0, ijpos_t size, qlat_t qlat,
                int sqside, sublat_ptr sublat, MAYBE_UNUSED int side);

int deg_norm_ij(ffspol_ptr ffspol_ij, ij_t i, ij_t j, int *gap);



#ifdef WANT_NORM_STATS
void norm_stats_print();
#endif

#ifdef __cplusplus
}
#endif

#endif   /* __NORM_H__ */
