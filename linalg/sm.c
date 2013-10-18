/* Shirokauer maps 
   
Input:

* A list of the (npurged) a,b pairs. This is obtained from the
  purgedfile.
* A matrix of (small_nrows) rows and (npurged) cols, which indicates
  the contents of each relation-set. This is obtained from the
  indexfile.
* The sub-group order (ell) such that ell | p-1
  Note: All computations are done mod ell^2.
* (eps): the exponent used in the computation of the Shirokauer maps.
  Note: eps = ppcm(eps_i), where eps_i = ell^(deg(f_i)) - 1 and f = f_1 ... f_k mod ell
  
Output

* A matrix of (small_nrows) rows and (nmaps)=deg(f) cols (mpz_t).  For each
  relation (rel) the (nmaps) Shirokauer maps are computed as the second
  least-significant digit of the ell-adic representation of the polynomial equal
  to (rel^eps - 1) / ell.  
*/

#include "cado.h"

#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include "utils.h"
#include "timing.h"


typedef struct {
  poly_t num;
  poly_t denom;
} relset_struct_t;


typedef relset_struct_t relset_t[1];
typedef relset_struct_t * relset_ptr;
typedef const relset_struct_t * relset_srcptr;


/* Q = P^a mod f, mod p. Note, p is mpz_t */
void
poly_power_mod_f_mod_mpz_Barrett (poly_t Q, const poly_t P, const poly_t f,
				  const mpz_t a, const mpz_t p,
                                  MAYBE_UNUSED const mpz_t invp)
{
  int k = mpz_sizeinbase(a, 2);

  if (mpz_cmp_ui(a, 0) == 0) {
    Q->deg = 0;
    mpz_set_ui(Q->coeff[0], 1);
    return;
  }
  
  // Initialize Q to P
  poly_copy(Q, P);

  // Horner
  for (k -= 2; k >= 0; k--)
  {
    poly_sqr_mod_f_mod_mpz(Q, Q, f, p, NULL);  // R <- R^2
    if (mpz_tstbit(a, k))
      poly_mul_mod_f_mod_mpz(Q, Q, P, f, p, NULL);  // R <- R*P
  }
}


relset_ptr build_rel_sets(const char * purgedname, const char * indexname,
			  int * small_nrows, poly_t F, const mpz_t ell2)
{
  purgedfile_stream ps;
  FILE * ix = fopen(indexname, "r");

  /* array of (a,b) pairs from (purgedname) file */
  poly_t *pairs;
  
  /* Array of (small_nrows) relation sets built from array (pairs) and (indexname) file  */
  relset_ptr rels;

  purgedfile_stream_init(ps);
  purgedfile_stream_openfile(ps, purgedname);

  pairs = (poly_t *) malloc(ps->nrows * sizeof(poly_t));

  /* Parse purgedfile (a,b)-pairs only */
  ps->parse_only_ab = 1;
  int npairs;
  for(npairs = 0 ; purgedfile_stream_get(ps, NULL) >= 0 ; npairs++) {
    ASSERT_ALWAYS(npairs < ps->nrows);
    if (ps->b == 0) {
	/* freerels */
	poly_alloc(pairs[npairs], 0);
	poly_setcoeff_int64(pairs[npairs], 0, ps->a);
	pairs[npairs]->deg=0;
      }
    else {
      /* an (a,b)-pair is a degree-1 poly */
      poly_alloc(pairs[npairs], 1);
      poly_setcoeff_int64(pairs[npairs], 0, ps->a);
      poly_setcoeff_int64(pairs[npairs], 1, -ps->b);
      pairs[npairs]->deg = 1;
    }
  }

  /* small_ncols isn't used here: we don't care */
  int small_ncols;
  int ret = fscanf(ix, "%d %d", small_nrows, &small_ncols);
  ASSERT(ret == 2);

  rels = malloc(*small_nrows * sizeof(relset_t));

  for (int k = 0 ; k < *small_nrows ; k++) {
    poly_alloc(rels[k].num, F->deg);
    poly_alloc(rels[k].denom, F->deg);
  }
    
  unsigned int ridx;
  long e, nc;
  poly_t tmp;
  
  mpz_t ee;
  mpz_init(ee);  

  poly_alloc(tmp, F->deg);

  for(int i = 0 ; i < *small_nrows ; i++) {
    ret = fscanf(ix, "%ld", &nc); 
    ASSERT_ALWAYS(ret == 1);

    (rels[i].num)->deg = 0;
    (rels[i].denom)->deg = 0;
    poly_setcoeff_si(rels[i].num, 0, 1);      /* rels[i].num = 1   */
    poly_setcoeff_si(rels[i].denom, 0, 1);    /* rels[i].denom = 1 */

    for(int k = 0 ; k < nc ; k++) {
      ret = fscanf(ix, "%x:%ld", &ridx, &e); 
      ASSERT_ALWAYS(ret == 2);

      /* Should never happen! */
      ASSERT_ALWAYS(e != 0);

      if (e > 0) {
	  mpz_set_si(ee, e);
	  /* TODO: poly_long_power_mod_f_mod_mpz */
	  poly_power_mod_f_mod_mpz(tmp, pairs[ridx], F, ee, ell2);
	  poly_mul_mod_f_mod_mpz(rels[i].num, rels[i].num, tmp, F, ell2, NULL);
      }
      else {
	  mpz_set_si(ee, -e);
	  /* TODO: poly_long_power_mod_f_mod_mpz */
	  poly_power_mod_f_mod_mpz(tmp, pairs[ridx], F, ee, ell2);
	  poly_mul_mod_f_mod_mpz(rels[i].denom, rels[i].denom, tmp, F, ell2, NULL);
      }
    }
    cleandeg(rels[i].num, F->deg);
    cleandeg(rels[i].denom, F->deg);

  }
  poly_free(tmp);

  fclose(ix);
  purgedfile_stream_closefile(ps);
  purgedfile_stream_clear(ps);

  while (npairs > 0)
    poly_free (pairs[--npairs]);
  free (pairs);
  mpz_clear (ee);
  
  return rels;
}

struct thread_info {
  int offset;
  int nb;
  relset_ptr rels;
  poly_srcptr F;
  mpz_srcptr eps;
  mpz_srcptr ell2;
  mpz_srcptr invl2;
  poly_t *sm;
};

void * thread_start(void *arg) {
  struct thread_info *ti = (struct thread_info *) arg;
  relset_ptr rels = ti->rels;
  poly_srcptr F = ti->F;
  mpz_srcptr eps = ti->eps;
  mpz_srcptr ell2 = ti->ell2;
  mpz_srcptr invl2 = ti->invl2;
  poly_t *sm = ti->sm;
  int offset = ti->offset;
  mpz_t tmp;

  mpz_init(tmp);

  poly_t g, U, V;
  poly_alloc(g, 0);
  poly_alloc (U,0);
  poly_alloc (V,0);


  /* poly_t SMn, SMd; */
  /* poly_alloc(SMn, F->deg); */
  /* poly_alloc(SMd, F->deg); */

  for (int i = 0; i < ti->nb; i++) {

    if (rels[offset+i].denom->deg == 0)
      {
	mpz_invert(tmp, rels[offset+i].denom->coeff[0], ell2);
	poly_mul_mpz(rels[offset+i].num, rels[offset+i].num, tmp);
	poly_reduce_mod_mpz(rels[offset+i].num, rels[offset+i].num, ell2);
      }
    else
      {
	poly_xgcd_mpz(g, F, rels[offset+i].denom, U, V, ell2);
	poly_mul(rels[offset+i].num, rels[offset+i].num, V);
	int d = poly_mod_f_mod_mpz(rels[offset+i].num->coeff, rels[offset+i].num->deg,
		    F->coeff, F->deg, ell2, NULL);
	cleandeg(rels[offset+i].num, d);
      }

    /* poly_power_mod_f_mod_mpz_Barrett(SMn, rels[offset+i].num, */
    /*     F, eps, ell2, invl2); */
    /* poly_sub_ui(SMn, 1); */

    /* poly_power_mod_f_mod_mpz_Barrett(SMd, rels[offset+i].denom, */
    /*     F, eps, ell2, invl2); */
    /* poly_sub_ui(SMd, 1); */

    poly_power_mod_f_mod_mpz_Barrett(sm[i], rels[offset+i].num,
        F, eps, ell2, invl2);
    poly_sub_ui(sm[i], 1);

    /* poly_sub_mod_mpz(sm[i], SMn, SMd, ell2); */
  }

  mpz_clear(tmp);
  /* poly_free(SMn); */
  /* poly_free(SMd); */
  poly_free(g);
  poly_free(U);
  poly_free(V);
  return NULL;
}

#define SM_BLOCK 500

void mt_sm(int nt, const char * outname, relset_ptr rels, int sr, poly_t F,
    const mpz_t eps, const mpz_t ell, const mpz_t ell2)
{
  // allocate space for results of threads
  poly_t **SM;
  SM = (poly_t **) malloc(nt*sizeof(poly_t *));
  for (int i = 0; i < nt; ++i) {
    SM[i] = (poly_t *) malloc(SM_BLOCK*sizeof(poly_t));
    for (int j = 0; j < SM_BLOCK; ++j)
      poly_alloc(SM[i][j], F->deg);
  }

  // We'll use a rotating buffer of thread id.
  pthread_t *threads;
  threads = (pthread_t *) malloc(nt*sizeof(pthread_t));
  int active_threads = 0;  // number of running threads
  int threads_head = 0;    // next thread to wait / restart.
  
  // Prepare the main loop
  int i = 0; // counter of relation-sets.
  int out_cpt = 0; // counter of already printed relation-sets;
  FILE * out = fopen(outname, "w");
  fprintf(out, "%d\n", sr);
  mpz_t invl2;
  mpz_init(invl2);
  barrett_init(invl2, ell2);

  // Arguments for threads
  struct thread_info *tis;
  tis = (struct thread_info*) malloc(nt*sizeof(struct thread_info));
  for (int i = 0; i < nt; ++i) {
    tis[i].rels = rels;
    tis[i].F = F;
    tis[i].eps = eps;
    tis[i].ell2 = ell2;
    tis[i].invl2 = invl2;
    tis[i].sm = SM[i];
    // offset and nb must be adjusted.
  }

  // Main loop
  while ((i < sr) || (active_threads > 0)) {
    // Start / restart threads as many threads as allowed
    if ((active_threads < nt) && (i < sr)) { 
      tis[threads_head].offset = i;
      tis[threads_head].nb = MIN(SM_BLOCK, sr-i);
      pthread_create(&threads[threads_head], NULL, 
          &thread_start, (void *)(&tis[threads_head]));
      i += SM_BLOCK;
      active_threads++;
      threads_head++; 
      if (threads_head == nt) 
        threads_head = 0;
      continue;
    }
    // Wait for the next thread to finish in order to print result.
    pthread_join(threads[threads_head], NULL);
    active_threads--;
    for (int k = 0; k < SM_BLOCK; ++k) {
      if (out_cpt >= sr)
        break;
      poly_ptr sm = SM[threads_head][k];
      for(int j=0; j<F->deg; j++) {
        if (j > sm->deg) {
          fprintf(out, "0 ");
          continue;
        }
        ASSERT_ALWAYS(mpz_divisible_p(sm->coeff[j], ell));
        mpz_divexact(sm->coeff[j], sm->coeff[j], ell);
        ASSERT_ALWAYS(mpz_cmp(ell, sm->coeff[j])>0);

        mpz_out_str(out, 10, sm->coeff[j]);
        fprintf(out, " ");
      }
      fprintf(out, "\n");
      out_cpt++;
    }
    // If we are at the end, no job will be restarted, but head still
    // must be incremented.
    if (i >= sr) { 
      threads_head++;
      if (threads_head == nt) 
        threads_head = 0;
    }
  }

  mpz_clear(invl2);
  fclose(out);
  free(tis);
  free(threads);
  for (int i = 0; i < nt; ++i) {
    for (int j = 0; j < SM_BLOCK; ++j) {
      poly_free(SM[i][j]);
    }
    free(SM[i]);
  }
  free(SM);
}


void shirokauer_maps(const char * outname, relset_ptr rels, int sr, poly_t F,
		     const mpz_t eps, const mpz_t ell, const mpz_t ell2)
{
  FILE * out = fopen(outname, "w");
  poly_t SM; //, SMn, SMd;
  mpz_t invl2;
  mpz_t tmp;
  
  mpz_init(tmp);

  mpz_init(invl2);
  barrett_init(invl2, ell2);


  fprintf(stderr, "\tBuilding %d relation sets mod ell^2:\n", sr);
  fprintf(stderr, "\tell^2 = ");
  mpz_out_str(stderr, 10, ell2);
  fprintf(stderr, "\n");

  /* poly_alloc(SMn, F->deg); */
  /* poly_alloc(SMd, F->deg); */
  poly_alloc(SM, F->deg);
  SM->deg = 0;
  poly_setcoeff_si(SM, 0, 1);

  poly_t g, U, V;
  poly_alloc(g, 0);
  poly_alloc (U,0);
  poly_alloc (V,0);
  
  fprintf(out, "%d\n", sr);

  for (int i=0; i<sr; i++) {

    /* poly_power_mod_f_mod_mpz_Barrett(SMn, rels[i].num, F, eps, ell2, invl2); */
    /* poly_sub_ui(SMn, 1); */

    /* poly_power_mod_f_mod_mpz_Barrett(SMd, rels[i].denom, F, eps, ell2, invl2); */
    /* poly_sub_ui(SMd, 1); */

    /* fprintf(stderr, "rels[i].denom->alloc: %d\n", rels[i].denom->alloc); */
    /* fprintf(stderr, "rels[i].denom->deg: %d\n", rels[i].denom->deg); */
    /* fprintf(stderr, ">> %d   %d\n", i, F->deg); */
    /* poly_print(rels[i].num); */

    if (rels[i].denom->deg == 0)
      {
        mpz_invert(tmp, rels[i].denom->coeff[0], ell2);
        poly_mul_mpz(rels[i].num, rels[i].num, tmp);
        poly_reduce_mod_mpz(rels[i].num, rels[i].num, ell2);
      }
    else
      {
        poly_xgcd_mpz (g, F, rels[i].denom, U, V, ell2);
        poly_mul (rels[i].num, rels[i].num, V);
        int d = poly_mod_f_mod_mpz (rels[i].num->coeff, rels[i].num->deg,
                                    F->coeff, F->deg, ell2, NULL);
        cleandeg(rels[i].num, d);
      }

    poly_power_mod_f_mod_mpz_Barrett(SM, rels[i].num, F, eps, ell2, invl2);
    poly_sub_ui(SM, 1);

    /* poly_sub_mod_mpz(SM, SMn, SMd, ell2); */

    for(int j=0; j<F->deg; j++) {
      if (j > SM->deg) {
          fprintf(out, "0 ");
          continue;
      }
      ASSERT_ALWAYS(mpz_divisible_p(SM->coeff[j], ell));
      mpz_divexact(SM->coeff[j], SM->coeff[j], ell);
      ASSERT_ALWAYS(mpz_cmp(ell, SM->coeff[j])>0);

      mpz_out_str(out, 10, SM->coeff[j]);
      fprintf(out, " ");
    }
    fprintf(out, "\n");
  }

  /* poly_free(SMn); */
  /* poly_free(SMd); */
  poly_free(SM);
  poly_free(U);
  poly_free(V);
  mpz_clear(invl2);
  mpz_clear(tmp);
  fclose(out);
}


static void declare_usage(param_list pl)
{
  param_list_decl_usage(pl, "poly", "(required) poly file");
  param_list_decl_usage(pl, "purged", "(required) purged file");
  param_list_decl_usage(pl, "index", "(required) index file");
  param_list_decl_usage(pl, "out", "output file");
  param_list_decl_usage(pl, "gorder", "(required) group order");
  param_list_decl_usage(pl, "smexp", "(required) sm-exponent");
  param_list_decl_usage(pl, "mt", "number of threads (default 1)");
}

static void usage (const char *argv, const char * missing, param_list pl)
{
  if (missing) {
    fprintf(stderr, "\nError: missing or invalid parameter \"-%s\"\n",
        missing);
  }
  param_list_print_usage(pl, argv, stderr);
  exit (EXIT_FAILURE);
}


/* -------------------------------------------------------------------------- */

int main (int argc, char **argv)
{
  char *argv0 = argv[0];

  const char *polyfile = NULL;
  const char *purgedfile = NULL;
  const char *indexfile = NULL;
  const char *outfile = NULL;
  const char *group_order = NULL;
  const char *sm_exponent = NULL;

  param_list pl;
  cado_poly pol;
  poly_t F;
  int deg;
  mpz_t *f;
  relset_ptr rels;
  int sr;
  mpz_t ell, ell2, eps;
  int mt = 1;
  double t0;

  /* read params */
  param_list_init(pl);
  declare_usage(pl);

  if (argc == 1)
    usage (argv[0], NULL, pl);

  argc--,argv++;
  for ( ; argc ; ) {
    if (param_list_update_cmdline (pl, &argc, &argv)) { continue; }
    fprintf (stderr, "Unhandled parameter %s\n", argv[0]);
    usage (argv0, NULL, pl);
  }


  if ((polyfile = param_list_lookup_string(pl, "poly")) == NULL) {
      fprintf(stderr, "Error: parameter -poly is mandatory\n");
      param_list_print_usage(pl, argv0, stderr);
      exit(EXIT_FAILURE);
  }

  if ((purgedfile = param_list_lookup_string(pl, "purged")) == NULL) {
      fprintf(stderr, "Error: parameter -purged is mandatory\n");
      param_list_print_usage(pl, argv0, stderr);
      exit(EXIT_FAILURE);
  }

  if ((indexfile = param_list_lookup_string(pl, "index")) == NULL) {
      fprintf(stderr, "Error: parameter -index is mandatory\n");
      param_list_print_usage(pl, argv0, stderr);
      exit(EXIT_FAILURE);
  }

  if ((group_order = param_list_lookup_string(pl, "gorder")) == NULL) {
      fprintf(stderr, "Error: parameter -gorder is mandatory\n");
      param_list_print_usage(pl, argv0, stderr);
      exit(EXIT_FAILURE);
  }

  if ((sm_exponent = param_list_lookup_string(pl, "smexp")) == NULL) {
      fprintf(stderr, "Error: parameter -smexp is mandatory\n");
      param_list_print_usage(pl, argv0, stderr);
      exit(EXIT_FAILURE);
  }

  param_list_parse_int(pl, "mt", &mt);
  if (mt < 1) {
    fprintf(stderr, "Error: parameter mt must be at least 1\n");
    param_list_print_usage(pl, argv0, stderr);
    exit(EXIT_FAILURE);
  }

  outfile = param_list_lookup_string(pl, "out");

  cado_poly_init (pol);
  cado_poly_read(pol, polyfile);
  
  if (param_list_warn_unused(pl))
    usage (argv0, NULL, pl);
  param_list_print_command_line (stdout, pl);

  /* Construct poly_t F from cado_poly pol (algebraic side) */
  deg = pol->pols[ALGEBRAIC_SIDE]->degree;
  f = pol->pols[ALGEBRAIC_SIDE]->f;
  ASSERT_ALWAYS(deg > 1);
  poly_alloc (F, deg);
  for (int i = deg; i >= 0; --i)
    poly_setcoeff (F, i, f[i]);

  fprintf(stderr, "F = ");
  poly_print(F);

  /* read ell from command line (assuming radix 10) */
  mpz_init_set_str(ell, group_order, 10);

  mpz_init(ell2);
  mpz_mul(ell2, ell, ell);

  fprintf(stderr, "\nSub-group order:\n\tell = ");
  mpz_out_str(stderr, 10, ell);
  fprintf(stderr, "\n");

  mpz_init_set_str(eps, sm_exponent, 10);

  fprintf(stderr, "\nShirokauer maps' exponent:\n\teps = ");
  mpz_out_str(stderr, 10, eps);
  fprintf(stderr, "\n");

  t0 = seconds();
  rels = build_rel_sets(purgedfile, indexfile, &sr, F, ell2);


  fprintf(stderr, "\nComputing Shirokauer maps for %d relations\n", sr);


  if (mt == 1)
    shirokauer_maps(outfile, rels, sr, F, eps, ell, ell2);
  else
    mt_sm(mt, outfile, rels, sr, F, eps, ell, ell2);

  fprintf(stderr, "\nsm completed in %2.2lf seconds\n", seconds() - t0);

  mpz_clear(eps);
  mpz_clear(ell);
  mpz_clear(ell2);
  poly_free(F);
  cado_poly_clear(pol);
  param_list_clear(pl);

  return 0;
}
