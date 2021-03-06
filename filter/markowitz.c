#include "cado.h"

#include "portability.h"
#include "filter_config.h"
#include "utils_with_io.h"
#include "merge_replay_matrix.h"
#include "sparse.h"
#include "mst.h"
#include "report.h"
#include "markowitz.h"

#define MKZ_DEBUG 0
// #define MKZ_TIMINGS 1

#if MKZ_TIMINGS
double tmkzup, tmkzdown, tmkzupdown, tmkzcount;
#endif

/* mat->MKZA[j] becomes MKZ_INF when column j is deleted */
#define MKZ_INF UMAX(index_t) 

// Again, a priority queue as a heap...!
// Q[0] contains the number of items in Q[], so that useful part of Q
// is Q[1..Q[0]]

// Q[2*i] contains j-jmin = dj
// Q[2*i+1] contains the Markowitz count for j

// A[j] gives u s.t. Q[2*u] = j

// get r-th component of Q[i]
#define MkzGet(Q, i, r) (Q[((i)<<1)+(r)])
#define MkzSet(Q, i, r, val) (Q[((i)<<1)+(r)] = (val))

#if 0
static int
MkzGetCount(int32_t *Q, int32_t *A, int32_t dj)
{
    return MkzGet(Q, A[dj], 1);
}
#endif

inline int
MkzIsAlive(index_t *A, int32_t dj)
{
  return A[dj] != MKZ_INF;
}

// (Q, A)[k1] <- (Q, A)[k2]
static void
MkzAssign(int32_t *Q, index_t *A, int32_t k1, int32_t k2)
{
    int32_t dj = MkzGet(Q, k2, 0);

    MkzSet(Q, k1, 0, dj);
    MkzSet(Q, k1, 1, MkzGet(Q, k2, 1)); // could be simplified...!
    A[dj] = k1;
}

#if 0
static void MAYBE_UNUSED
MkzPrintQueue(int32_t *Q)
{
    int level = 0, imax = 1, i;

    fprintf(stderr, "L0:");
    for(i = 1; i <= Q[0]; i++){
	fprintf(stderr, " [%d, %d]", MkzGet(Q, i, 1), MkzGet(Q, i, 0));
	if(i == imax){
	    imax = (imax<<1)+1;
	    fprintf(stderr, "\nL%d:", ++level);
	}
    }
    fprintf(stderr, "\n");
}
#endif

static void
MkzUpQueue(int32_t *Q, index_t *A, int32_t k)
{
    int32_t dj = MkzGet(Q, k, 0), count = MkzGet(Q, k, 1);
#if MKZ_TIMINGS
    double tt = seconds();
#endif

    while((k > 1) && (MkzGet(Q, k/2, 1) >= count)){
	// we are at level > 0 and the father is >= son
	// the father replaces the son
      MkzAssign(Q, A, k, k/2);
	k /= 2;
    }
    // we found the place of (dj, count)
    MkzSet(Q, k, 0, dj);
    MkzSet(Q, k, 1, count);
    A[dj] = k;
#if MKZ_TIMINGS
    tmkzup += (seconds()-tt);
#endif
}

static void
MkzInsert(int32_t *Q, index_t *A, int32_t dj, int32_t count)
{
    Q[0]++;
    MkzSet(Q, Q[0], 0, dj);
    MkzSet(Q, Q[0], 1, count);
    A[dj] = Q[0];
    MkzUpQueue(Q, A, Q[0]);
}

// Move Q[k] down, by keeping the structure of Q as a heap, i.e.,
// each node has a smaller cost than its two left and right nodes
static void
MkzDownQueue(int32_t *Q, index_t *A, int32_t k)
{
    int32_t dj = MkzGet(Q, k, 0), count = MkzGet(Q, k, 1), j;
#if MKZ_TIMINGS
    double tt = seconds();
#endif

    while ((j = 2*k) <= Q[0]){ /* node k has at least one left son 2k */
	if(j < Q[0])
	    // node k has also a right son
	    if(MkzGet(Q, j, 1) > MkzGet(Q, j+1, 1))
		j++;
	// at this point, Q[j] is the son with the smallest "count"
	if(count <= MkzGet(Q, j, 1)) /* Q[k] has smaller cost than both sons */
	    break;
	else{
	    // the father takes the place of the "smaller" son
          MkzAssign(Q, A, k, j);
	    k = j;
	}
    }
    // we found the place of (dj, count)
    MkzSet(Q, k, 0, dj);
    MkzSet(Q, k, 1, count);
    A[dj] = k;
#if MKZ_TIMINGS
    tmkzdown += (seconds()-tt);
#endif
}

// (Q, A)[k] has just arrived, but we have to move it in the heap, so that
// it finds its place.
static void
MkzMoveUpOrDown(int32_t *Q, index_t *A, int32_t k)
{
#if MKZ_TIMINGS
    double tt = seconds();
#endif

    // move new node up or down
    if(k == 1)
	// rare event!
	MkzDownQueue(Q, A, 1);
    else{
	// k has a father
	if(MkzGet(Q, k/2, 1) > MkzGet(Q, k, 1))
	    // we have to move up
	    MkzUpQueue(Q, A, k);
	else
	    MkzDownQueue(Q, A, k);
    }
#if MKZ_TIMINGS
    tmkzupdown += (seconds()-tt);
#endif
}

// Remove (Q, A)[k].
static void
MkzDelete(int32_t *Q, index_t *A, int32_t k)
{
#if MKZ_DEBUG >= 1
    fprintf(stderr, "MKZ: deleting (Q, A)[%d]=[%d, %d]\n", k,
	    MkzGet(Q, k, 0), MkzGet(Q, k, 1));
#endif
    // we put Q[Q[0]] in Q[k]
    MkzAssign (Q, A, k, Q[0]);
    Q[0]--;
    MkzMoveUpOrDown(Q, A, k);
}

#if 0
// Remove (Q, A)[k].
void
MkzRemove(int32_t *dj, int32_t *mkz, int32_t *Q, index_t *A, int32_t k)
{
    *dj = MkzGet(Q, k, 0);
    *mkz = MkzGet(Q, k, 1);
    A[*dj] = MKZ_INF;
    MkzAssign(Q, A, k, Q[0]);
    Q[0]--;
    MkzMoveUpOrDown(Q, A, k);
}
#endif

#if MKZ_DEBUG >= 1
static int MAYBE_UNUSED
MkzIsHeap(int32_t *Q)
{
    int k;

    for(k = 1; k <= Q[0]/2; k++){
	// k has a left son
	if(MkzGet(Q, k, 1) > MkzGet(Q, 2*k, 1)){
	    fprintf(stderr, "Pb: father=%d > lson=%d\n",
		    MkzGet(Q, k, 1), MkzGet(Q, 2*k, 1));
	    return 0;
	}
	if(k < Q[0]/2){
	    // k has a right son
	    if(MkzGet(Q, k, 1) > MkzGet(Q, 2*k+1, 1)){
		fprintf(stderr, "Pb: father=%d > rson=%d\n",
			MkzGet(Q, k, 1), MkzGet(Q, 2*k+1, 1));
		return 0;
	    }
	}
    }
    return 1;
}
#endif

static void
MkzCheck(filter_matrix_t *mat)
{
  uint64_t dj;
  int maxlevel = mat->mergelevelmax;
  for(dj = 0; dj < mat->ncols; dj++)
  {
    if (0 < mat->wt[dj] && mat->wt[dj] <= maxlevel)
	  {
      if(MkzGet(mat->MKZQ, mat->MKZA[dj], 0) != (int32_t) dj)
      {
        fprintf(stderr, "GASP: %" PRId32 " <> %" PRIu64 " in MkzCheck\n",
                        MkzGet(mat->MKZQ, mat->MKZA[dj], 0), dj);
        exit (1);
      }
    }
  }
}

static int
Cavallar(filter_matrix_t *mat, int32_t j)
{
    return abs(mat->wt[j]);
}

/* Note: we could also take into account the "cancelled ideals", i.e., the
   ideals apart the pivot j that got cancelled "by chance". However some
   experiments show that this does not improve (if any) the result of merge.
   A possible explanation is that the contribution of cancelled ideals follows
   a normal distribution, and that on the long run we mainly see the average.
*/
static int
pureMkz(filter_matrix_t *mat, int32_t j)
{
    int i;
    unsigned int k;
    int w = mat->wt[j];

    if (w <= 1)
      return -4; /* ensures that empty columns and singletons are removed earlier */
    else if (w == 2)
      return -2;
    else
      {
        index_t w0;

        /* approximate traditional Markowitz count: we assume we add the
	   lightest row to all other rows */
        w0 = mat->ncols;
        for(k = 1; k <= mat->R[j][0]; k++)
          if((i = mat->R[j][k]) != -1){
	    // this should be the weight of row i
            if(matLengthRow(mat, i) < w0)
              w0 = matLengthRow(mat, i);
          }
        /* here we assume there is no cancellation other than for ideal j:
	   the lightest row has weight w0, we add wt-1 times w0-1,
	   remove once w0-1, and remove wt entries in the jth column */
	return (w0 - 2) * (mat->wt[j] - 2) - 2;
      }
}

// forcing lighter columns first.
static int
lightColAndMkz(filter_matrix_t *mat, int32_t j)
{
    int i, wj, cte;
    unsigned int k;
    index_t mkz;
    int32_t ind[MERGE_LEVEL_MAX];
    double tfill, tMST;

#if MKZ_TIMINGS
    double tt = seconds();
#endif
    // trick to be sure that columns with wt <= 2 are treated asap
    wj = mat->wt[j];
    cte = mat->ncols; // could be smaller
    if(wj == 1)
	return cte;
    else if(wj == 2){
	fillTabWithRowsForGivenj(ind, mat, j);
	// the more this is < 0, the less the weight is
	return 2 * cte + weightSum(mat, ind[0], ind[1], j);
    }
    else if(wj <= mat->wmstmax){
	fillTabWithRowsForGivenj(ind, mat, j);
	mkz = minCostUsingMST(mat, mat->wt[j], ind, j, &tfill, &tMST);
	return wj * cte + mkz;
    }
    // real traditional Markowitz count
    mkz = mat->nrows;
    for(k = 1; k <= mat->R[j][0]; k++)
	if((i = mat->R[j][k]) != -1){
	    // this should be the weight of row i
      if(matLengthRow(mat, i) < mkz)
          mkz = matLengthRow(mat, i);
	}
    mkz = wj * cte + (mkz-1) * (wj-1);
#if MKZ_TIMINGS
    tmkzcount += (seconds()-tt);
#endif
    return mkz;
}

/* return the cost of merging column j (the smaller, the better) */
static int
MkzCount(filter_matrix_t *mat, int32_t j)
{
    switch(mat->mkztype){
    case 2:
	return lightColAndMkz(mat, j);
    case 1:
	return pureMkz(mat, j);
    case 0:
    default:
	return Cavallar(mat, j);
    }
}

/* pop the top element from the heap, return 0 iff heap is empty */
int
MkzPopQueue(int32_t *dj, int32_t *mkz, filter_matrix_t *mat)
{
  int32_t *Q = mat->MKZQ;
  index_t *A = mat->MKZA;

  /* Q[0] contains the number of items in Q[], thus the first element is
     stored in Q[2..3] */
  *dj = MkzGet(Q, 1, 0);
  *mkz = MkzGet(Q, 1, 1);
  while (mat->wt[*dj] > mat->mergelevelmax)
    {
      /* remove heavy column */
      MkzDelete (Q, A, 1);
      A[*dj] = MKZ_INF;

      if (MkzQueueCardinality(mat->MKZQ) <= 0)
        return 0;

      *dj = MkzGet(Q, 1, 0);
      *mkz = MkzGet(Q, 1, 1);
    }
  A[*dj] = MKZ_INF; /* already done in MkzRemoveJ, but if we don't do it,
                       we get A[j1]=A[j2] for some j1 <> j2 */
  if (MkzQueueCardinality(mat->MKZQ) <= 0)
    return 0;
  MkzAssign(Q, A, 1, Q[0]); /* move entry of index Q[0] in Q,A to index 1 */
  Q[0]--;                   /* decrease number of entries in Q,A */
  MkzDownQueue(Q, A, 1);    /* reorder heap structure */
  return 1;
}

void
MkzInit(filter_matrix_t *mat)
{
    int32_t mkz;
    index_t j;
    uint64_t sz = 0;
    int maxlevel = mat->mergelevelmax;

#if MKZ_TIMINGS
    tmkzup = tmkzdown = tmkzupdown = tmkzcount = 0.0;
#endif
    fprintf(stderr, "Entering initMarkowitz");
    fprintf(stderr, " (wmstmax=%d, type=%d)\n", mat->wmstmax, mat->mkztype);
    // compute number of elligible columns in the heap
    for(j = 0; j < mat->ncols; j++)
      if(0 < mat->wt[j] && mat->wt[j] <= maxlevel)
        sz++;
    
    // Allocating heap MKZQ
    size_t tmp_alloc = (sz+1) * 2 * sizeof(int32_t);
    fprintf(stderr, "Allocating heap for %" PRIu64 " columns (%zuMB)\n", sz,
                                                             tmp_alloc >> 20);
    mat->MKZQ = (int32_t *) malloc (tmp_alloc);
    mat->MKZQ[0] = 0;
    mat->MKZQ[1] = (int32_t) sz; // why not?
    
    // every j needs a pointer (MKZA)
    tmp_alloc = (mat->ncols + 1) * sizeof(index_t);
    fprintf(stderr, "Allocating pointers to heap: %zuMB\n", tmp_alloc >> 20);
    mat->MKZA = (index_t *) malloc (tmp_alloc);

    // init
    for(j = 0; j < mat->ncols; j++)
      if (0 < mat->wt[j] && mat->wt[j] <= maxlevel)
      {
        mkz = MkzCount(mat, j);
#if MKZ_DEBUG >= 1
        printf("j=%d wt=%d", j, mat->wt[j]);
        printf(" => mkz=%d\n", mkz);
#endif
        MkzInsert(mat->MKZQ, mat->MKZA, j, mkz);
      }
      else
        mat->MKZA[j] = MKZ_INF;
    
    // TODO: overflow in mkz???
    MkzCheck(mat);
#if MKZ_DEBUG >= 1
    fprintf(stderr, "Initial queue is\n");
    MkzPrintQueue(mat->MKZQ);
#endif
}

void
MkzClose(filter_matrix_t *mat)
{
    fprintf(stderr, "Max Markowitz count: %d\n",
	    MkzGet(mat->MKZQ, mat->MKZQ[0], 1));
#if MKZ_TIMINGS
    fprintf(stderr, "MKZT: up=%d down=%d updown=%d count=%d\n",
	    (int)tmkzup, (int)tmkzdown, (int)tmkzupdown, (int)tmkzcount);
#endif
    free(mat->MKZQ);
    free(mat->MKZA);
}

/* increment the weight of column j (in absolute value) */
int
MkzIncrCol(filter_matrix_t *mat, int32_t j)
{
    int ind;

#if MKZ_DEBUG >= 1
    fprintf(stderr, "Incr: wt(%d) was %d\n", j, mat->wt[j]);
#endif
    ind = mat->wt[j] = incrS(mat->wt[j]);
    return ind;
}

// Row[i] has been adjoined to column j, so that we can incrementally
// change the Markowitz count.
void
MkzUpdate(filter_matrix_t *mat, int32_t i MAYBE_UNUSED, int32_t j)
{
    int32_t adr = mat->MKZA[j];
    int mkz;

    if(adr == -1){
#if MKZ_DEBUG >= 1
	fprintf(stderr, "Prevented use of adr[%d]=-1 in MkzUpdate\n", j);
#endif
	return;
    }
#if MKZ_DEBUG >= 1
    if((mat->wt[j] == 0) || (mat->wt[j] == 1))
	fprintf(stderr, "W: wt[%d] = %d\n", j, mat->wt[j]);
#endif
#if 1
    // costly?
    mkz = MkzCount(mat, j);
#else
    // old_count = min (r_ii-1)*(w-2) = mu * (w-2)
    // new_count = min(mu, r_i-1)*(w-1)
    mkz = MkzGet(mat->MKZQ, adr, 1)/(mat->wt[j]-2); // mu
    if(matLengthRow(mat,i) < mkz+1)
      mkz = matLengthRow(mat,i)-1;
    mkz *= (mat->wt[j]-1);
#endif
#if MKZ_DEBUG >= 1
    fprintf(stderr, "Updating j=%d (old=%d, new=%d)\n", j,
	    MkzGet(mat->MKZQ, adr, 1), mkz);
#endif
    // nothing to do if new count == old count
    if (mkz != MkzGet(mat->MKZQ, adr, 1))
      {
        // add new count
        MkzSet(mat->MKZQ, adr, 1, mkz);
        // a variant of delete is needed...!
        MkzMoveUpOrDown(mat->MKZQ, mat->MKZA, adr);
      }
}

// Row[i] has been removed for column j, so that we need to update
// the Markowitz count.
void
MkzUpdateDown (filter_matrix_t *mat, int32_t j)
{
  int32_t adr = mat->MKZA[j];
  int mkz;

  if (adr == -1) /* column too heavy or already removed */
    return;

  mkz = MkzCount(mat, j);
  if (mkz != MkzGet(mat->MKZQ, adr, 1))
    {
      // update count
      MkzSet(mat->MKZQ, adr, 1, mkz);
      // a variant of delete is needed...!
      MkzMoveUpOrDown(mat->MKZQ, mat->MKZA, adr);
    }
}

/*
   Updates:
   - mat->wt[j] (weight of column j)

   We arrive here when mat->wt[j] > 0.

*/
void
MkzDecreaseColWeight(filter_matrix_t *mat, int32_t j)
{
    int32_t dj = j;

#if MKZ_DEBUG >= 1
    fprintf(stderr, "Decreasing col %d; was %d\n", j, mat->wt[dj]);
#endif
    mat->wt[dj] = decrS(mat->wt[dj]);
}

/* remove column j from and update matrix */
void
MkzRemoveJ(filter_matrix_t *mat, int32_t j)
{
    int32_t dj = j;

    mat->wt[dj] = 0;

    /* This can happen when maxlevel < weight[dj] <= cwmax initially, thus
       A[dj] was initialized to MKZ_INF, but because of a merge it becomes
       larger than cwmax. FIXME: should we have maxlevel = cwmax? */
    if (mat->MKZA[dj] == MKZ_INF)
	return;

#if MKZ_DEBUG >= 1
    fprintf(stderr, "Removing col %d of weight %d\n", j, mat->wt[dj]);
#endif
    // remove j from the QA structure
    MkzDelete (mat->MKZQ, mat->MKZA, mat->MKZA[dj]);
    mat->MKZA[dj] = MKZ_INF;
#if MKZ_DEBUG >= 1
    MkzIsHeap(mat->MKZQ);
#endif
}
