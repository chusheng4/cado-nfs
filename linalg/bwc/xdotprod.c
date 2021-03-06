#include "cado.h"
#include "bwc_config.h"
#include "xdotprod.h"
#include "bw-common.h"

void x_dotprod(matmul_top_data_ptr mmt, uint32_t * xv, unsigned int nx, mmt_vec_ptr v, unsigned int z0, unsigned int m, int sign)
{
    /* We're reading from the shared right vector data -- this area is
     * written to by the other threads in the column. Some of them might
     * be lingering in reduce operations, so we have to wait for them
     */
    if (mmt->wr[bw->dir]->v->flags & THREAD_SHARED_VECTOR) {
        serialize_threads(mmt->pi->wr[bw->dir]);
    } else {
        /* I presume that no locking is needed here. But it's unchecked
        */
        // ASSERT_ALWAYS(0);
    }

    for(unsigned int j = 0 ; j < m ; j++) {
        void * where = SUBVEC(v, v, z0 + j);
        for(unsigned int t = 0 ; t < nx ; t++) {
            uint32_t i = xv[j*nx+t];
            unsigned int vi0 = mmt->wr[bw->dir]->i0;
            unsigned int vi1 = mmt->wr[bw->dir]->i1;
            unsigned int hi0 = mmt->wr[!bw->dir]->i0;
            unsigned int hi1 = mmt->wr[!bw->dir]->i1;
            if (i < vi0 || i >= vi1)
                continue;
            /* We want the data to match our interval on both
             * directions, because otherwise we'll end up
             * computing rubbish -- recall that no broadcast_down
             * has occurred yet.
             */
            if (i < hi0 || i >= hi1)
                continue;
            if (sign > 0) {
                v->abase->add(v->abase,
                        where, where,
                        SUBVEC(mmt->wr[bw->dir]->v, v, i - vi0));
            } else {
                v->abase->sub(v->abase,
                        where, where,
                        SUBVEC(mmt->wr[bw->dir]->v, v, i - vi0));
            }
        }
    }
}

