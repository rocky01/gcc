/* Test the `vshr_ns8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vshr_ns8 (void)
{
  int8x8_t out_int8x8_t;
  int8x8_t arg0_int8x8_t;

  out_int8x8_t = vshr_n_s8 (arg0_int8x8_t, 1);
}

/* { dg-final { scan-assembler "vshr\.s8\[ 	\]+\[dD\]\[0-9\]+, \[dD\]\[0-9\]+, #\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */
