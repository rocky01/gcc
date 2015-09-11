/* Test the `vreinterpretQp16_s64' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vreinterpretQp16_s64 (void)
{
  poly16x8_t out_poly16x8_t;
  int64x2_t arg0_int64x2_t;

  out_poly16x8_t = vreinterpretq_p16_s64 (arg0_int64x2_t);
}

