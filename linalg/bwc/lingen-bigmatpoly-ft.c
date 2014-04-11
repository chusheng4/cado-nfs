#include "cado.h"
#include "abase.h"
#include <stdlib.h>
#include "portability.h"
#include "macros.h"
#include "lingen-matpoly-ft.h"
#include "lingen-bigmatpoly-ft.h"

/* This is the interface for doing products of polynomial matrices by
 * caching transforms, and transferring them over MPI. There are
 * currently two pitfalls.
 *
 * 1/ we have no streaming of computations and communications. This means
 * two things. First, communication/computation overlap can be
 * beneficial: while transforms of rows on the left term of a product are
 * computed (assuming transforms of the right term are ready), it is
 * possible to communicate the already computed row transforms. A second
 * point is that if we manage to get these computed soon, then there is
 * no need to keep them in memory. Such an approach would make it
 * possible to reduce the overall memory footprint. The place to fix this
 * is to modify the control flow in bigmatpoly_mul_caching_adj
 *
 * 2/ we are transferring full-length transforms, while it would 
 * be sufficient to transfer the truncated transforms of course.
 */



/* {{{  init/zero/clear interface for bigmatpoly_ft */
matpoly_ft_ptr bigmatpoly_ft_my_cell(bigmatpoly_ft_ptr p)
{
    int irank;
    int jrank;
    MPI_Comm_rank(p->col, &irank);
    MPI_Comm_rank(p->row, &jrank);
    return bigmatpoly_ft_cell(p, irank, jrank);
}

void bigmatpoly_ft_init_model(bigmatpoly_ft_ptr model, MPI_Comm comm, unsigned int m, unsigned int n)
{
    int size;
    int rank;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    ASSERT_ALWAYS(size == (int) (m * n));
    int irank = rank / n;
    int jrank = rank % n;

    memset(model, 0, sizeof(bigmatpoly_ft));
    model->m1 = m;
    model->n1 = n;
    MPI_Comm_dup(comm, &(model->comm));
    MPI_Comm_split(comm, irank, jrank, &(model->row));
    MPI_Comm_split(comm, jrank, irank, &(model->col));
}

/* This completes the initialization process. This is _not_ a collective
 * operation */
void bigmatpoly_ft_finish_init(abdst_field ab, bigmatpoly_ft_ptr p, unsigned int m, unsigned int n, struct fft_transform_info * fti)
{
    ASSERT_ALWAYS(bigmatpoly_ft_check_pre_init(p));
    p->m = m;
    p->n = n;
    p->m0 = m / p->m1;
    p->n0 = n / p->n1;
    matpoly_ft_ptr me = bigmatpoly_ft_my_cell(p);
    /* For matpoly_ft, init or finish_init are identical */
    matpoly_ft_init(ab, me, p->m0, p->n0, fti);
}
/* If m,n,len are zero, then this is exactly equivalent to duplicating
 * the mpi_model. In which case the initialization may be completed later
 * on with bigmatpoly_ft_finish_init
 */
void bigmatpoly_ft_init(abdst_field ab, bigmatpoly_ft_ptr p, bigmatpoly_ft_srcptr model, unsigned int m, unsigned int n, struct fft_transform_info * fti)
{
    ASSERT_ALWAYS(model);
    ASSERT_ALWAYS(m % model->m1 == 0);
    ASSERT_ALWAYS(n % model->n1 == 0);
    int irank;
    int jrank;
    MPI_Comm_rank(model->col, &irank);
    MPI_Comm_rank(model->row, &jrank);
    memset(p, 0, sizeof(bigmatpoly_ft));
    p->m1 = model->m1;
    p->n1 = model->n1;
    MPI_Comm_dup(model->comm, &(p->comm));
    MPI_Comm_dup(model->row, &(p->row));
    MPI_Comm_dup(model->col, &(p->col));

    p->cells = malloc(p->m1*p->n1*sizeof(matpoly_ft));
    memset(p->cells, 0, p->m1*p->n1*sizeof(matpoly_ft));
    /* Either none or all must be non-zero */
    ASSERT_ALWAYS((!m||!n) ^ (m&&n));
    if (!m)
        return;

    /* Allocate only our own cell. If we happen to appear as a left
     * multiplicand someday, we'll need recipient cells for our whole
     * row. If we happen to appear as a right multiplicand, we'll need
     * recipient cells for our whole columns. But there is no reason for
     * both situations (let alone any) to happen. Thus this allocation of
     * recipient cells is dynamic */
    bigmatpoly_ft_finish_init(ab, p, m, n, fti);
}

/* This checks the validity of the m,n,size fields. If the structure
 * corresponds to a lazy allocation state, this function returns 1. If
 * the structure corresponds to something fully initializaed already,
 * this function returns 0. If the fields are filled in an inconsistent
 * manner, abort() is called.
 */
int bigmatpoly_ft_check_pre_init(bigmatpoly_ft_srcptr p)
{
    if (p->m && p->n)
        return 0;
    if (!p->m && !p->n)
        return 1;
    abort();
    return 0;
}


void bigmatpoly_ft_clear_model(bigmatpoly_ft_ptr p)
{
    MPI_Comm_free(&(p->comm));
    MPI_Comm_free(&(p->row));
    MPI_Comm_free(&(p->col));
    memset(p, 0, sizeof(bigmatpoly_ft));
}

void bigmatpoly_ft_clear(abdst_field ab, bigmatpoly_ft_ptr p, struct fft_transform_info * fti)
{
    for(unsigned int t = 0 ; t < p->m1 * p->n1 ; t++) {
        if (p->cells[t]->data)
            matpoly_ft_clear(ab, p->cells[t], fti);
    }
    free(p->cells);
    bigmatpoly_ft_clear_model(p);
}


/* We are a left multiplicand. This is a no-op if space for our row has
 * already been allocated */
static void bigmatpoly_ft_provision_row(abdst_field ab, bigmatpoly_ft_ptr p, struct fft_transform_info * fti)
{
    int irank;
    int jrank;
    MPI_Comm_rank(p->col, &irank);
    MPI_Comm_rank(p->row, &jrank);
    for(unsigned int j = 0 ; j < p->n1 ; j++) {
        if (j == (unsigned int) jrank) continue;
        matpoly_ft_ptr them = bigmatpoly_ft_cell(p, irank, j);
        if (them->data) continue;
        matpoly_ft_init(ab, them, p->m0, p->n0, fti);
    }
}

/* We are a right multiplicand. This is a no-op if space for our col has
 * already been allocated */
void bigmatpoly_ft_provision_col(abdst_field ab, bigmatpoly_ft_ptr p, struct fft_transform_info * fti)
{
    int irank;
    int jrank;
    MPI_Comm_rank(p->col, &irank);
    MPI_Comm_rank(p->row, &jrank);
    for(unsigned int i = 0 ; i < p->m1 ; i++) {
        if (i == (unsigned int) irank) continue;
        matpoly_ft_ptr them = bigmatpoly_ft_cell(p, i, jrank);
        if (them->data) continue;
        matpoly_ft_init(ab, them, p->m0, p->n0, fti);
    }
}

/* This zeroes out _our_ cell */
void bigmatpoly_ft_zero(abdst_field ab, bigmatpoly_ft_ptr p, struct fft_transform_info * fti)
{
    matpoly_ft_ptr me = bigmatpoly_ft_my_cell(p);
    matpoly_ft_zero(ab, me, fti);
}

/* okay, it's ugly */
void bigmatpoly_ft_swap(bigmatpoly_ft_ptr a, bigmatpoly_ft_ptr b)
{
    bigmatpoly_ft x;
    memcpy(x, a, sizeof(bigmatpoly_ft));
    memcpy(a, b, sizeof(bigmatpoly_ft));
    memcpy(b, x, sizeof(bigmatpoly_ft));
}

/* }}} */


/* {{{ allgather operations */
static void bigmatpoly_ft_allgather_row(abdst_field ab, bigmatpoly_ft_ptr a, struct fft_transform_info * fti)
{
    int irank;
    int jrank;
    MPI_Comm_rank(a->col, &irank);
    MPI_Comm_rank(a->row, &jrank);
    bigmatpoly_ft_provision_row(ab, a, fti);
    
    /* Each node makes his cell exportable */
    matpoly_ft_export(ab, bigmatpoly_ft_my_cell(a), fti);

    size_t fft_alloc_sizes[3];
    fft_get_transform_allocs(fft_alloc_sizes, fti);
    size_t tsize = fft_alloc_sizes[0];

    /* TODO: only transfer up to the truncated length ? */
    for(unsigned int k = 0 ; k < a->n1 ; k++) {
        matpoly_ft_ptr data = bigmatpoly_ft_cell(a, irank, k);
        MPI_Bcast(data->data, data->m * data->n * tsize, MPI_BYTE, k, a->row);
    }

    /* and now all nodes on the row import the cells from their friends */
    for(unsigned int j = 0 ; j < a->n1 ; j++) {
        matpoly_ft_import(ab, bigmatpoly_ft_cell(a, irank, j), fti);
    }
}

static void bigmatpoly_ft_allgather_col(abdst_field ab, bigmatpoly_ft_ptr a, struct fft_transform_info * fti)
{
    int irank;
    int jrank;
    MPI_Comm_rank(a->col, &irank);
    MPI_Comm_rank(a->row, &jrank);
    bigmatpoly_ft_provision_col(ab, a, fti);

    /* Each node makes his cell exportable */
    matpoly_ft_export(ab, bigmatpoly_ft_my_cell(a), fti);

    size_t fft_alloc_sizes[3];
    fft_get_transform_allocs(fft_alloc_sizes, fti);
    size_t tsize = fft_alloc_sizes[0];

    /* TODO: only transfer up to the truncated length ? */
    for(unsigned int k = 0 ; k < a->m1 ; k++) {
        matpoly_ft_ptr data = bigmatpoly_ft_cell(a, k, jrank);
        MPI_Bcast(data->data, data->m * data->n * tsize, MPI_BYTE, k, a->col);
    }

    /* and now all nodes on the row import the cells from their friends */
    for(unsigned int i = 0 ; i < a->m1 ; i++) {
        matpoly_ft_import(ab, bigmatpoly_ft_cell(a, i, jrank), fti);
    }
}
/* }}} */

void bigmatpoly_ft_mul(abdst_field ab, bigmatpoly_ft_ptr c, bigmatpoly_ft_ptr a, bigmatpoly_ft_ptr b, struct fft_transform_info * fti)/*{{{*/
{
    ASSERT_ALWAYS(a->n == b->m);
    ASSERT_ALWAYS(a->n1 == b->m1);
    if (bigmatpoly_ft_check_pre_init(c)) {
        bigmatpoly_ft_finish_init(ab, c, a->m, b->n, fti);
    }
    ASSERT_ALWAYS(c->m);
    ASSERT_ALWAYS(c->m == a->m);
    ASSERT_ALWAYS(c->n == b->n);
    int irank;
    int jrank;
    MPI_Comm_rank(c->col, &irank);
    MPI_Comm_rank(c->row, &jrank);
    bigmatpoly_ft_allgather_row(ab, a, fti);
    bigmatpoly_ft_allgather_col(ab, b, fti);
    // ASSERT_ALWAYS(c->alloc >= c->size);

    matpoly_ft_ptr lc = bigmatpoly_ft_my_cell(c);
    bigmatpoly_ft_zero(ab, c, fti);
    ASSERT_ALWAYS(a->n == b->m);
    for(unsigned int k = 0 ; k < a->n1 ; k++) {
        matpoly_ft_addmul(ab, lc, 
                bigmatpoly_ft_cell(a, irank, k),
                bigmatpoly_ft_cell(b, k, jrank), fti);
    }
}/*}}}*/


void bigmatpoly_ft_dft(abdst_field ab, bigmatpoly_ft_ptr ta, bigmatpoly_ptr a, struct fft_transform_info * fti)
{
    matpoly_ft_dft(ab, bigmatpoly_ft_my_cell(ta), bigmatpoly_my_cell(a), fti);
}

void bigmatpoly_ft_ift(abdst_field ab, bigmatpoly_ptr a, bigmatpoly_ft_ptr ta, struct fft_transform_info * fti)
{
    bigmatpoly_set_size(a, a->size);
    matpoly_ft_ift(ab, bigmatpoly_my_cell(a), bigmatpoly_ft_my_cell(ta), fti);
}

void bigmatpoly_ft_ift_mp(abdst_field ab, bigmatpoly_ptr a, bigmatpoly_ft_ptr ta, unsigned int shift, struct fft_transform_info * fti)
{
    bigmatpoly_set_size(a, a->size);
    matpoly_ft_ift_mp(ab, bigmatpoly_my_cell(a), bigmatpoly_ft_my_cell(ta), shift, fti);
}

void bigmatpoly_mul_caching_adj(abdst_field ab, bigmatpoly c, bigmatpoly a, bigmatpoly b, unsigned int adj)/*{{{*/
{
    bigmatpoly_ft tc, ta, tb;
    mpz_t p;
    mpz_init(p);
    abfield_characteristic(ab, p);
    struct fft_transform_info fti[1];
    fft_get_transform_info_fppol(fti, p, a->size, b->size, a->n);
    if (adj != UINT_MAX) {
        fft_transform_info_adjust_depth(fti, adj);
    }
    bigmatpoly_clear(ab, c);
    bigmatpoly_ptr model = a;
    bigmatpoly_ft_ptr ftmodel = (bigmatpoly_ft_ptr) a;
    bigmatpoly_init(ab, c, model, a->m, b->n, a->size + b->size - 1);
    bigmatpoly_ft_init(ab, ta, ftmodel, a->m, a->n, fti);
    bigmatpoly_ft_init(ab, tb, ftmodel, b->m, b->n, fti);
    bigmatpoly_ft_init(ab, tc, ftmodel, a->m, b->n, fti);
    bigmatpoly_ft_dft(ab, ta, a, fti);
    bigmatpoly_ft_dft(ab, tb, b, fti);
    bigmatpoly_ft_mul(ab, tc, ta, tb, fti);
    c->size = a->size + b->size - 1;
    bigmatpoly_ft_ift(ab, c, tc, fti);
    bigmatpoly_ft_clear(ab, ta, fti);
    bigmatpoly_ft_clear(ab, tb, fti);
    bigmatpoly_ft_clear(ab, tc,  fti);
    mpz_clear(p);
}/*}}}*/

void bigmatpoly_mp_caching_adj(abdst_field ab, bigmatpoly c, bigmatpoly a, bigmatpoly b, unsigned int adj)/*{{{*/
{
    bigmatpoly_ft tc, ta, tb;
    mpz_t p;
    mpz_init(p);
    abfield_characteristic(ab, p);
    struct fft_transform_info fti[1];
    fft_get_transform_info_fppol_mp(fti, p, MIN(a->size, b->size), MAX(a->size, b->size), a->n);
    if (adj != UINT_MAX) {
        fft_transform_info_adjust_depth(fti, adj);
    }
    bigmatpoly_clear(ab, c);
    bigmatpoly_ptr model = a;
    bigmatpoly_ft_ptr ftmodel = (bigmatpoly_ft_ptr) a;
    bigmatpoly_init(ab, c, model, a->m, b->n, MAX(a->size, b->size) - MIN(a->size, b->size) + 1);
    bigmatpoly_ft_init(ab, ta, ftmodel, a->m, a->n, fti);
    bigmatpoly_ft_init(ab, tb, ftmodel, b->m, b->n, fti);
    bigmatpoly_ft_init(ab, tc, ftmodel, a->m, b->n, fti);
    bigmatpoly_ft_dft(ab, ta, a, fti);
    bigmatpoly_ft_dft(ab, tb, b, fti);
    bigmatpoly_ft_mul(ab, tc, ta, tb, fti);
    c->size = MAX(a->size, b->size) - MIN(a->size, b->size) + 1;
    bigmatpoly_ft_ift_mp(ab, c, tc, MIN(a->size, b->size) - 1, fti);
    bigmatpoly_ft_clear(ab, ta, fti);
    bigmatpoly_ft_clear(ab, tb, fti);
    bigmatpoly_ft_clear(ab, tc,  fti);
    mpz_clear(p);
}/*}}}*/
