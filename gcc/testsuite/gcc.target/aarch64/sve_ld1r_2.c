/* { dg-do compile } */
/* { dg-options "-O3 -march=armv8-a+sve -fno-tree-loop-distribute-patterns" } */

#include <stdint.h>

#define NUM_ELEMS(TYPE) (1024 / sizeof (TYPE))

#define DEF_LOAD_BROADCAST(TYPE)			\
  void __attribute__ ((noinline, noclone))		\
  set_##TYPE (TYPE *restrict a, TYPE *restrict b)	\
  {							\
    for (int i = 0; i < NUM_ELEMS (TYPE); i++)		\
      a[i] = *b;					\
  }

#define DEF_LOAD_BROADCAST_IMM(TYPE, IMM, SUFFIX)	\
  void __attribute__ ((noinline, noclone))		\
  set_##TYPE##_##SUFFIX (TYPE *a)			\
  {							\
    for (int i = 0; i < NUM_ELEMS (TYPE); i++)		\
      a[i] = IMM;					\
  }

#define FOR_EACH_LOAD_BROADCAST(T)		\
  T (int8_t)					\
  T (int16_t)					\
  T (int32_t)					\
  T (int64_t)

#define FOR_EACH_LOAD_BROADCAST_IMM(T)					\
  T (int16_t, 129, imm_129)						\
  T (int32_t, 129, imm_129)						\
  T (int64_t, 129, imm_129)						\
									\
  T (int16_t, -130, imm_m130)						\
  T (int32_t, -130, imm_m130)						\
  T (int64_t, -130, imm_m130)						\
									\
  T (int16_t, 0x1234, imm_0x1234)					\
  T (int32_t, 0x1234, imm_0x1234)					\
  T (int64_t, 0x1234, imm_0x1234)					\
									\
  T (int16_t, 0xFEDC, imm_0xFEDC)					\
  T (int32_t, 0xFEDC, imm_0xFEDC)					\
  T (int64_t, 0xFEDC, imm_0xFEDC)					\
									\
  T (int32_t, 0x12345678, imm_0x12345678)				\
  T (int64_t, 0x12345678, imm_0x12345678)				\
									\
  T (int32_t, 0xF2345678, imm_0xF2345678)				\
  T (int64_t, 0xF2345678, imm_0xF2345678)				\
									\
  T (int64_t, (int64_t) 0xFEBA716B12371765, imm_FEBA716B12371765)

FOR_EACH_LOAD_BROADCAST (DEF_LOAD_BROADCAST)
FOR_EACH_LOAD_BROADCAST_IMM (DEF_LOAD_BROADCAST_IMM)

/* { dg-final { scan-assembler-times {\tld1rb\tz[0-9]+\.b, p[0-7]/z, } 1 } } */
/* { dg-final { scan-assembler-times {\tld1rh\tz[0-9]+\.h, p[0-7]/z, } 5 } } */
/* { dg-final { scan-assembler-times {\tld1rw\tz[0-9]+\.s, p[0-7]/z, } 7 } } */
/* { dg-final { scan-assembler-times {\tld1rd\tz[0-9]+\.d, p[0-7]/z, } 8 } } */
