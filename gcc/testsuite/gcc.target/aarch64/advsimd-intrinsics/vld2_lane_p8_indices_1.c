#include <arm_neon.h>

/* { dg-do compile } */
/* { dg-skip-if "" { *-*-* } { "-fno-fat-lto-objects" } } */
/* { dg-excess-errors "" { xfail arm*-*-* } } */

poly8x8x2_t
f_vld2_lane_p8 (poly8_t * p, poly8x8x2_t v)
{
  poly8x8x2_t res;
  /* { dg-error "lane 8 out of range 0 - 7" "" { xfail arm*-*-* } 0 } */
  res = vld2_lane_p8 (p, v, 8);
  /* { dg-error "lane -1 out of range 0 - 7" "" { xfail arm*-*-* } 0 } */
  res = vld2_lane_p8 (p, v, -1);
  return res;
}
