#include "cado.h"
#include <stdio.h>
#include "bwc_config.h"
#include "parallelizing_info.h"
#include "matmul_top.h"
#include "select_mpi.h"
#include "params.h"
#include "xvectors.h"
#include "bw-common.h"
#include "filenames.h"
#include "mpfq/mpfq.h"
#include "mpfq/mpfq_vbase.h"
#include "async.h"
#include "portability.h"

/*
 * Relatively common manipulation in fact. Move to matmul_top ?
 */
static void xvec_to_vec(matmul_top_data_ptr mmt, uint32_t * gxvecs, int m, unsigned int nx, int d)
{
    mmt_comm_ptr mcol = mmt->wr[d];
    pi_comm_ptr picol = mmt->pi->wr[d];
    int shared = mcol->v->flags & THREAD_SHARED_VECTOR;
    mpfq_vbase_ptr A = mcol->v->abase;
    if (!shared || picol->trank == 0) {
        A->vec_set_zero(A, mcol->v->v, mcol->i1 - mcol->i0);
        for(int j = 0 ; j < m ; j++) {
            for(unsigned int k = 0 ; k < nx ; k++) {
                uint32_t i = gxvecs[j*nx+k];
                // set bit j of entry i to 1.
                if (i < mcol->i0 || i >= mcol->i1)
                    continue;
                void * where;
                where = SUBVEC(mcol->v, v, i-mcol->i0);
                A->set_ui_at(A, where, j, 1);
            }
        }
    }
    if (shared)
        serialize_threads(picol);
}

/* We merely have to build up a check vector. We've got a fairly easy
 * candidate for that: the x vector.  */

#if 0
void save_untwisted_transposed_vector(matmul_top_data_ptr mmt, const char * name, int d, int iter)
{
    mmt_comm_ptr mcol = mmt->wr[!d];
    mmt_comm_ptr mrow = mmt->wr[d];
    pi_comm_ptr picol = mmt->pi->wr[!d];
    pi_comm_ptr pirow = mmt->pi->wr[d];

    // v ---> Pr^-1*Sr^-1*v
    matmul_top_twist_vector(mmt, d);

    serialize_threads(mmt->pi->m);

    // v ---> Pr*v          [[ Sr^-1 * v ]]
    if (pirow->trank == 0 || !(mrow->v->flags & THREAD_SHARED_VECTOR)) {
        if (pirow->jrank == 0 && pirow->trank == 0) {
            /* ok, we keep the data */
        } else {
            abzero(mmt->abase, mrow->v->v, mrow->i1 - mrow->i0);
        }
    }
    matmul_top_mul_comm(mmt, !d);
    serialize_threads(mmt->pi->m);

    matmul_top_save_vector(mmt, name, !d, iter);
    // v --> Pr^-1 * v      [[ Pr^-1*Sr^-1*v ]]
    if (picol->trank == 0 || !(mcol->v->flags & THREAD_SHARED_VECTOR)) {
        if (picol->jrank == 0 && picol->trank == 0) {
            /* ok, we keep the data */
        } else {
            abzero(mmt->abase, mcol->v->v, mcol->i1 - mcol->i0);
        }
    }
    matmul_top_mul_comm(mmt, d);
    serialize_threads(mmt->pi->m);

    // v --> Sr*Pr*v        [[ v ]]
    matmul_top_untwist_vector(mmt, d);
    serialize_threads(mmt->pi->m);
}
#endif

void * sec_prog(parallelizing_info_ptr pi, param_list pl, void * arg MAYBE_UNUSED)
{
    int fake = param_list_lookup_string(pl, "random_matrix") != NULL;

    /* Interleaving does not make sense for this program. So the second
     * block of threads just leave immediately */
    if (pi->interleaved && pi->interleaved->idx)
        return NULL;

    int tcan_print = bw->can_print && pi->m->trank == 0;
    matmul_top_data mmt;


    /* XXX Here we're working in the opposite direction compared to
     * prep/krylov/mksol ! */
    int flags[2];
    flags[!bw->dir] = THREAD_SHARED_VECTOR;
    flags[bw->dir] = 0;


    int withcoeffs = mpz_cmp_ui(bw->p, 2) > 0;
    int nchecks = withcoeffs ? NCHECKS_CHECK_VECTOR_GFp : NCHECKS_CHECK_VECTOR_GF2;
    mpfq_vbase A;
    mpfq_vbase_oo_field_init_byfeatures(A, 
            MPFQ_PRIME_MPZ, bw->p,
            MPFQ_GROUPSIZE, nchecks,
            MPFQ_DONE);

    pi_datatype_ptr A_pi = pi_alloc_mpfq_datatype(pi, A);

    matmul_top_init(mmt, A, A_pi, pi, flags, pl, bw->dir);
    unsigned int unpadded = MAX(mmt->n0[0], mmt->n0[1]);

    /* Because we're a special case, we _expect_ to work opposite to
     * optimized direction. So we pass bw->dir even though _we_ are going
     * to call mmt_mul with !bw->dir.
     */

    mmt_comm_ptr mrow = mmt->wr[!bw->dir];
    // mmt_comm_ptr mcol = mmt->wr[bw->dir];
    
    serialize(pi->m);

    A->vec_set_zero(A, mrow->v->v, mrow->i1 - mrow->i0);

    uint32_t * gxvecs = NULL;
    unsigned int nx = 0;

    if (!fake) {
        load_x(&gxvecs, bw->m, &nx, pi);
    } else {
        set_x_fake(&gxvecs, bw->m, &nx, pi);
    }


    xvec_to_vec(mmt, gxvecs, MIN(nchecks, bw->m), nx, !bw->dir);

    /*
    for(int j = 0 ; j < nchecks ; j++) {
        for(unsigned int k = 0 ; k < nx ; k++) {
            uint32_t i = gxvecs[j*nx+k];
            // set bit j of entry i to 1.
            if (i < mrow->i0 || i >= mrow->i1)
                continue;
            abt * where;
            where = mrow->v->v + aboffset(abase, i-mrow->i0);
            abset_ui(abase, where, j, 1);
        }
    }
    */
    // allreduce_across(mmt, !bw->dir);

    // matmul_top_twist_vector(mmt, !bw->dir);
    // matmul_top_save_vector(mmt, "ux", !bw->dir, 0);
    // save_untwisted_transposed_vector(mmt, "tx", !bw->dir, 0);
    matmul_top_save_vector(mmt, CHECK_FILE_BASE, !bw->dir, 0, unpadded);


    if (tcan_print) {
        printf("Computing trsp(x)*M^k for check stops k=");
        for(int s = 0 ; s < bw->number_of_check_stops ; s++) {
            int next = bw->check_stops[s];
            if (s) printf(",");
            printf("%d", next);
        }
        printf("\n");
    }

    serialize(pi->m);

#if 0
    /* FIXME -- that's temporary ! only for debugging */
    pi_log_init(pi->m);
    pi_log_init(pi->wr[0]);
    pi_log_init(pi->wr[1]);
#endif

    // kill the warning.
    mmt->mm->iteration[!bw->dir] = INT_MIN;

    int k = 0;
    for(int s = 0 ; s < bw->number_of_check_stops ; s++) {
        int next = bw->check_stops[s];
        matmul_top_twist_vector(mmt, !bw->dir);
        for( ; k < next ; k++) {
            pi_log_op(mmt->pi->m, "iteration %d", k);
            matmul_top_mul(mmt, !bw->dir);

            if (tcan_print) {
                putchar('.');
                fflush(stdout);
            }
        }
        serialize(pi->m);
        serialize_threads(mmt->pi->m);
        matmul_top_untwist_vector(mmt, !bw->dir);
        matmul_top_save_vector(mmt, CHECK_FILE_BASE, !bw->dir, k, unpadded);
        if (tcan_print) {
            printf("saved %s.%d\n", CHECK_FILE_BASE, next);
        }
    }

    matmul_top_clear(mmt);
    pi_free_mpfq_datatype(pi, A_pi);

    A->oo_field_clear(A);

    free(gxvecs);

#if 0
    pi_log_clear(pi->m);
    pi_log_clear(pi->wr[0]);
    pi_log_clear(pi->wr[1]);
#endif

    return NULL;
}


int main(int argc, char * argv[])
{
    param_list pl;

    bw_common_init_new(bw, &argc, &argv);
    param_list_init(pl);
    parallelizing_info_init();

    bw_common_decl_usage(pl);
    parallelizing_info_decl_usage(pl);
    matmul_top_decl_usage(pl);
    /* declare local parameters and switches: none here (so far). */

    bw_common_parse_cmdline(bw, pl, &argc, &argv);

    bw_common_interpret_parameters(bw, pl);
    parallelizing_info_lookup_parameters(pl);
    matmul_top_lookup_parameters(pl);
    /* interpret our parameters */
    catch_control_signals();

    if (param_list_warn_unused(pl)) {
        param_list_print_usage(pl, bw->original_argv[0], stderr);
        exit(EXIT_FAILURE);
    }

    pi_go(sec_prog, pl, 0);

    parallelizing_info_finish();
    param_list_clear(pl);
    bw_common_clear_new(bw);
    return 0;
}

