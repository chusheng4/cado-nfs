#include "cado.h"
/* MPFQ generated file -- do not edit */

#include "mpfq_u64k1_t.h"

#include "binary-dotprods-backends.h"
/* Active handler: simd_u64k */
/* Automatically generated code  */
/* Active handler: Mpfq::defaults */
/* Active handler: Mpfq::defaults::vec */
/* Active handler: simd_dotprod */
/* Active handler: io */
/* Active handler: trivialities */
/* Active handler: simd_char2 */
/* Options used:{
   family=[ u64k1, u64k2, u64k3, u64k4, ],
   k=1,
   tag=u64k1,
   vbase_stuff={
    choose_byfeatures=<code>,
    families=[
     [ u64k1, u64k2, u64k3, u64k4, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_1, tag=p_1, }, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_2, tag=p_2, }, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_3, tag=p_3, }, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_4, tag=p_4, }, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_5, tag=p_5, }, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_6, tag=p_6, }, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_7, tag=p_7, }, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_8, tag=p_8, }, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_9, tag=p_9, }, ],
     [ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_pz, tag=pz, }, ],
     ],
    member_templates_restrict={
     p_1=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_1, tag=p_1, }, ],
     p_2=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_2, tag=p_2, }, ],
     p_3=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_3, tag=p_3, }, ],
     p_4=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_4, tag=p_4, }, ],
     p_5=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_5, tag=p_5, }, ],
     p_6=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_6, tag=p_6, }, ],
     p_7=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_7, tag=p_7, }, ],
     p_8=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_8, tag=p_8, }, ],
     p_9=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_p_9, tag=p_9, }, ],
     pz=[ { cpp_ifdef=COMPILE_MPFQ_PRIME_FIELD_pz, tag=pz, }, ],
     u64k1=[ u64k1, u64k2, u64k3, u64k4, ],
     u64k2=[ u64k1, u64k2, u64k3, u64k4, ],
     u64k3=[ u64k1, u64k2, u64k3, u64k4, ],
     u64k4=[ u64k1, u64k2, u64k3, u64k4, ],
     },
    vc:includes=[ <stdarg.h>, ],
    },
   virtual_base={
    filebase=mpfq_vbase,
    global_prefix=mpfq_,
    name=mpfq_vbase,
    substitutions=[
     [ (?^:mpfq_u64k1_elt \*), void *, ],
     [ (?^:mpfq_u64k1_src_elt\b), const void *, ],
     [ (?^:mpfq_u64k1_elt\b), void *, ],
     [ (?^:mpfq_u64k1_dst_elt\b), void *, ],
     [ (?^:mpfq_u64k1_elt_ur \*), void *, ],
     [ (?^:mpfq_u64k1_src_elt_ur\b), const void *, ],
     [ (?^:mpfq_u64k1_elt_ur\b), void *, ],
     [ (?^:mpfq_u64k1_dst_elt_ur\b), void *, ],
     [ (?^:mpfq_u64k1_vec \*), void *, ],
     [ (?^:mpfq_u64k1_src_vec\b), const void *, ],
     [ (?^:mpfq_u64k1_vec\b), void *, ],
     [ (?^:mpfq_u64k1_dst_vec\b), void *, ],
     [ (?^:mpfq_u64k1_vec_ur \*), void *, ],
     [ (?^:mpfq_u64k1_src_vec_ur\b), const void *, ],
     [ (?^:mpfq_u64k1_vec_ur\b), void *, ],
     [ (?^:mpfq_u64k1_dst_vec_ur\b), void *, ],
     [ (?^:mpfq_u64k1_poly \*), void *, ],
     [ (?^:mpfq_u64k1_src_poly\b), const void *, ],
     [ (?^:mpfq_u64k1_poly\b), void *, ],
     [ (?^:mpfq_u64k1_dst_poly\b), void *, ],
     ],
    },
   w=64,
   } */


/* Functions operating on the field structure */

/* Element allocation functions */

/* Elementary assignment functions */

/* Assignment of random values */

/* Arithmetic operations on elements */

/* Operations involving unreduced elements */

/* Comparison functions */

/* Input/output functions */

/* Vector functions */

/* Polynomial functions */

/* Functions related to SIMD operation */

/* Member templates related to SIMD operation */

/* Object-oriented interface */
/* *simd_dotprod::code_for_member_template_dotprod */
void mpfq_u64k1_u64k1_dotprod(mpfq_u64k1_dst_field K0 MAYBE_UNUSED, mpfq_u64k1_dst_field K1 MAYBE_UNUSED, mpfq_u64k1_dst_vec xw, mpfq_u64k1_src_vec xu1, mpfq_u64k1_src_vec xu0, unsigned int n)
{
    uint64_t * w = xw[0];
    const uint64_t * u0 = xu0[0];
    const uint64_t * u1 = xu1[0];
    dotprod_64K_64(w,u1,u0,n,1);
}

/* *simd_dotprod::code_for_member_template_dotprod */
void mpfq_u64k1_u64k2_dotprod(mpfq_u64k1_dst_field K0 MAYBE_UNUSED, mpfq_u64k2_dst_field K1 MAYBE_UNUSED, mpfq_u64k1_dst_vec xw, mpfq_u64k2_src_vec xu1, mpfq_u64k1_src_vec xu0, unsigned int n)
{
    uint64_t * w = xw[0];
    const uint64_t * u0 = xu0[0];
    const uint64_t * u1 = xu1[0];
    dotprod_64K_128(w,u0,u1,n,1);
}

/* *simd_dotprod::code_for_member_template_dotprod */
void mpfq_u64k1_u64k3_dotprod(mpfq_u64k1_dst_field K0 MAYBE_UNUSED, mpfq_u64k3_dst_field K1 MAYBE_UNUSED, mpfq_u64k1_dst_vec xw, mpfq_u64k3_src_vec xu1, mpfq_u64k1_src_vec xu0, unsigned int n)
{
    uint64_t * w = xw[0];
    const uint64_t * u0 = xu0[0];
    const uint64_t * u1 = xu1[0];
    dotprod_64K_64(w,u1,u0,n,3);
}

/* *simd_dotprod::code_for_member_template_dotprod */
void mpfq_u64k1_u64k4_dotprod(mpfq_u64k1_dst_field K0 MAYBE_UNUSED, mpfq_u64k4_dst_field K1 MAYBE_UNUSED, mpfq_u64k1_dst_vec xw, mpfq_u64k4_src_vec xu1, mpfq_u64k1_src_vec xu0, unsigned int n)
{
    uint64_t * w = xw[0];
    const uint64_t * u0 = xu0[0];
    const uint64_t * u1 = xu1[0];
    dotprod_64K_64(w,u1,u0,n,4);
}

/* *simd_dotprod::code_for_member_template_addmul_tiny */
void mpfq_u64k1_u64k1_addmul_tiny(mpfq_u64k1_dst_field K MAYBE_UNUSED, mpfq_u64k1_dst_field L MAYBE_UNUSED, mpfq_u64k1_dst_vec w, mpfq_u64k1_src_vec u, mpfq_u64k1_dst_vec v, unsigned int n)
{
    vaddmul_tiny_64K_64L((uint64_t*)w[0],(const uint64_t*)u[0],(const uint64_t*)v[0],n,1,1);
}

/* *simd_dotprod::code_for_member_template_addmul_tiny */
void mpfq_u64k1_u64k2_addmul_tiny(mpfq_u64k1_dst_field K MAYBE_UNUSED, mpfq_u64k2_dst_field L MAYBE_UNUSED, mpfq_u64k2_dst_vec w, mpfq_u64k1_src_vec u, mpfq_u64k2_dst_vec v, unsigned int n)
{
    vaddmul_tiny_64K_64L((uint64_t*)w[0],(const uint64_t*)u[0],(const uint64_t*)v[0],n,1,2);
}

/* *simd_dotprod::code_for_member_template_addmul_tiny */
void mpfq_u64k1_u64k3_addmul_tiny(mpfq_u64k1_dst_field K MAYBE_UNUSED, mpfq_u64k3_dst_field L MAYBE_UNUSED, mpfq_u64k3_dst_vec w, mpfq_u64k1_src_vec u, mpfq_u64k3_dst_vec v, unsigned int n)
{
    vaddmul_tiny_64K_64L((uint64_t*)w[0],(const uint64_t*)u[0],(const uint64_t*)v[0],n,1,3);
}

/* *simd_dotprod::code_for_member_template_addmul_tiny */
void mpfq_u64k1_u64k4_addmul_tiny(mpfq_u64k1_dst_field K MAYBE_UNUSED, mpfq_u64k4_dst_field L MAYBE_UNUSED, mpfq_u64k4_dst_vec w, mpfq_u64k1_src_vec u, mpfq_u64k4_dst_vec v, unsigned int n)
{
    vaddmul_tiny_64K_64L((uint64_t*)w[0],(const uint64_t*)u[0],(const uint64_t*)v[0],n,1,4);
}

/* *simd_dotprod::code_for_member_template_transpose */
void mpfq_u64k1_u64k1_transpose(mpfq_u64k1_dst_field K MAYBE_UNUSED, mpfq_u64k1_dst_field L MAYBE_UNUSED, mpfq_u64k1_dst_vec w, mpfq_u64k1_src_vec u)
{
    vtranspose_64K_64L((uint64_t*)w[0],(const uint64_t*)u[0],1,1);
}

/* *simd_dotprod::code_for_member_template_transpose */
void mpfq_u64k1_u64k2_transpose(mpfq_u64k1_dst_field K MAYBE_UNUSED, mpfq_u64k2_dst_field L MAYBE_UNUSED, mpfq_u64k1_dst_vec w, mpfq_u64k2_src_vec u)
{
    vtranspose_64K_64L((uint64_t*)w[0],(const uint64_t*)u[0],1,2);
}

/* *simd_dotprod::code_for_member_template_transpose */
void mpfq_u64k1_u64k3_transpose(mpfq_u64k1_dst_field K MAYBE_UNUSED, mpfq_u64k3_dst_field L MAYBE_UNUSED, mpfq_u64k1_dst_vec w, mpfq_u64k3_src_vec u)
{
    vtranspose_64K_64L((uint64_t*)w[0],(const uint64_t*)u[0],1,3);
}

/* *simd_dotprod::code_for_member_template_transpose */
void mpfq_u64k1_u64k4_transpose(mpfq_u64k1_dst_field K MAYBE_UNUSED, mpfq_u64k4_dst_field L MAYBE_UNUSED, mpfq_u64k1_dst_vec w, mpfq_u64k4_src_vec u)
{
    vtranspose_64K_64L((uint64_t*)w[0],(const uint64_t*)u[0],1,4);
}


/* vim:set ft=cpp: */
