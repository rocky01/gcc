/* { dg-do run { target aarch64_sve_hw } } */
/* { dg-options "-O2 -ftree-vectorize -march=armv8-a+sve" } */
/* { dg-require-effective-target fenv_exceptions } */

#ifndef TEST_EXCEPTIONS
#define TEST_EXCEPTIONS 1
#endif

#include <fenv.h>

#include "sve_vcond_4.c"

#define N 401

#define RUN_LOOP(TYPE1, TYPE2, CMP, EXPECT_INVALID)			\
  {									\
    TYPE1 dest1[N], dest2[N], dest3[N], src[N];				\
    TYPE2 a[N], b[N];							\
    for (int i = 0; i < N; ++i)						\
      {									\
	src[i] = i * i;							\
	if (i % 5 == 0)							\
	  a[i] = 0;							\
	else if (i % 3)							\
	  a[i] = i * 0.1;						\
	else								\
	  a[i] = i;							\
	if (i % 7 == 0)							\
	  b[i] = __builtin_nan ("");					\
	else if (i % 6)							\
	  b[i] = i * 0.1;						\
	else								\
	  b[i] = i;							\
	asm volatile ("" ::: "memory");					\
      }									\
    feclearexcept (FE_ALL_EXCEPT);					\
    test_##TYPE1##_##TYPE2##_##CMP##_var (dest1, src, 11, a, b, N);	\
    test_##TYPE1##_##TYPE2##_##CMP##_zero (dest2, src, 22, a, N);	\
    test_##TYPE1##_##TYPE2##_##CMP##_sel (dest3, 33, 44, a, 9, N);	\
    if (TEST_EXCEPTIONS							\
	&& !fetestexcept (FE_INVALID) != !(EXPECT_INVALID))		\
      __builtin_abort ();						\
    for (int i = 0; i < N; ++i)						\
      {									\
	if (dest1[i] != (CMP (a[i], b[i]) ? src[i] : 11))		\
	  __builtin_abort ();						\
	if (dest2[i] != (CMP (a[i], 0) ? src[i] : 22))			\
	  __builtin_abort ();						\
	if (dest3[i] != (CMP (a[i], 9) ? 33 : 44))			\
	  __builtin_abort ();						\
      }									\
  }

#define RUN_CMP(CMP, EXPECT_INVALID) \
  RUN_LOOP (int32_t, float, CMP, EXPECT_INVALID) \
  RUN_LOOP (uint32_t, float, CMP, EXPECT_INVALID) \
  RUN_LOOP (int64_t, float, CMP, EXPECT_INVALID) \
  RUN_LOOP (uint64_t, float, CMP, EXPECT_INVALID) \
  RUN_LOOP (float, float, CMP, EXPECT_INVALID) \
  RUN_LOOP (int32_t, double, CMP, EXPECT_INVALID) \
  RUN_LOOP (uint32_t, double, CMP, EXPECT_INVALID) \
  RUN_LOOP (int64_t, double, CMP, EXPECT_INVALID) \
  RUN_LOOP (uint64_t, double, CMP, EXPECT_INVALID) \
  RUN_LOOP (double, double, CMP, EXPECT_INVALID)

int __attribute__ ((optimize (1)))
main (void)
{
  RUN_CMP (eq, 0)
  RUN_CMP (ne, 0)
  RUN_CMP (olt, 1)
  RUN_CMP (ole, 1)
  RUN_CMP (oge, 1)
  RUN_CMP (ogt, 1)
  RUN_CMP (ordered, 0)
  RUN_CMP (unordered, 0)
  RUN_CMP (ueq, 0)
  RUN_CMP (ult, 0)
  RUN_CMP (ule, 0)
  RUN_CMP (uge, 0)
  RUN_CMP (ugt, 0)
  RUN_CMP (nueq, 0)
  RUN_CMP (nult, 0)
  RUN_CMP (nule, 0)
  RUN_CMP (nuge, 0)
  RUN_CMP (nugt, 0)
  return 0;
}
