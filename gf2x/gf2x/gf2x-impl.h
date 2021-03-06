/* This file is part of the gf2x library.

   Copyright 2007, 2008, 2009, 2010, 2013, 2015
   Richard Brent, Pierrick Gaudry, Emmanuel Thome', Paul Zimmermann

   This program is free software; you can redistribute it and/or modify it
   under the terms of either:
    - If the archive contains a file named toom-gpl.c (not a trivial
    placeholder), the GNU General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.
    - If the archive contains a file named toom-gpl.c which is a trivial
    placeholder, the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.
   
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the license text for more details.
   
   You should have received a copy of the GNU General Public License as
   well as the GNU Lesser General Public License along with this program;
   see the files COPYING and COPYING.LIB.  If not, write to the Free
   Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
   02110-1301, USA.
*/

#ifndef GF2X_IMPL_H_
#define GF2X_IMPL_H_

#include "gf2x.h"
/* first include the stuff that even users of the library have access to */
#include "gf2x/gf2x-impl-export.h"

/* then proceed to the really internal stuff */
#include "gf2x/gf2x-config.h"
#include "gf2x/gf2x-thresholds.h"

/* These flags are for internal use */
#define	GF2X_SELECT_KARA	0	/* do not change ! */
#define	GF2X_SELECT_TC3		1
#define	GF2X_SELECT_TC3W	2
#define	GF2X_SELECT_TC4		3
#define	GF2X_SELECT_UNB_DFLT	0
#define	GF2X_SELECT_UNB_TC3U	1	/* do not change ! */

#include <assert.h>
#ifndef ASSERT
#define ASSERT(x)	assert(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern long gf2x_toomspace(long n);
extern long gf2x_toomuspace(long n);
extern void gf2x_mul_basecase(unsigned long * c, const unsigned long * a,
			 long na, const unsigned long * b, long nb);


extern void gf2x_mul_toom(unsigned long *c, const unsigned long *a,
			const unsigned long *b, long n, unsigned long *stk);
extern void gf2x_mul_kara(unsigned long *c, const unsigned long *a, const unsigned long *b,
			long n, unsigned long *stk);
#if GPL_CODE_PRESENT
extern void gf2x_mul_tc3(unsigned long *c, const unsigned long *a, const unsigned long *b,
		 	long n, unsigned long *stk);
extern void gf2x_mul_tc3w(unsigned long *c, const unsigned long *a, const unsigned long *b,
		        long n, unsigned long *stk);
extern void gf2x_mul_tc4(unsigned long *c, const unsigned long *a, const unsigned long *b,
			long n, unsigned long *stk);
extern void gf2x_mul_tc3u(unsigned long * c, const unsigned long * a, long sa,
	      const unsigned long * b, unsigned long * stk);
#endif /* GPL_CODE_PRESENT */

extern short gf2x_best_toom(unsigned long);
extern long gf2x_toomspace(long);

extern short gf2x_best_utoom(unsigned long);
extern long gf2x_toomuspace(long);


extern void gf2x_mul_fft(unsigned long *c, const unsigned long *a, size_t an,
		            const unsigned long *b, size_t bn, long K);


/* tunetoom.c need to poke into toom.c's tables ; that's very ugly. So
 * please don't use for anything else.
 * */
extern short best_tab[GF2X_TOOM_TUNING_LIMIT];
extern short best_utab[GF2X_TOOM_TUNING_LIMIT];

#ifdef __cplusplus
}
#endif

#endif	/* GF2X_IMPL_H_ */
