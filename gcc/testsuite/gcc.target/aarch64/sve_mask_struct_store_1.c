/* { dg-do compile } */
/* { dg-options "-O2 -ftree-vectorize -ffast-math -march=armv8-a+sve" } */

#define TEST_LOOP(NAME, OUTTYPE, INTYPE, MASKTYPE)		\
  void __attribute__ ((noinline, noclone))			\
  NAME##_2 (OUTTYPE *__restrict dest, INTYPE *__restrict src,	\
	    MASKTYPE *__restrict cond, INTYPE bias, int n)	\
  {								\
    for (int i = 0; i < n; ++i)					\
      {								\
	INTYPE value = src[i] + bias;				\
	if (cond[i])						\
	  {							\
	    dest[i * 2] = value;				\
	    dest[i * 2 + 1] = value;				\
	  }							\
      }								\
  }

#define TEST2(NAME, OUTTYPE, INTYPE) \
  TEST_LOOP (NAME##_i8, OUTTYPE, INTYPE, signed char) \
  TEST_LOOP (NAME##_i16, OUTTYPE, INTYPE, unsigned short) \
  TEST_LOOP (NAME##_f32, OUTTYPE, INTYPE, float) \
  TEST_LOOP (NAME##_f64, OUTTYPE, INTYPE, double)

#define TEST1(NAME, OUTTYPE) \
  TEST2 (NAME##_i8, OUTTYPE, signed char) \
  TEST2 (NAME##_i16, OUTTYPE, unsigned short) \
  TEST2 (NAME##_i32, OUTTYPE, int) \
  TEST2 (NAME##_i64, OUTTYPE, unsigned long)

#define TEST(NAME) \
  TEST1 (NAME##_i8, signed char) \
  TEST1 (NAME##_i16, unsigned short) \
  TEST1 (NAME##_i32, int) \
  TEST1 (NAME##_i64, unsigned long) \
  TEST2 (NAME##_f16_f16, _Float16, _Float16) \
  TEST2 (NAME##_f32_f32, float, float) \
  TEST2 (NAME##_f64_f64, double, double)

TEST (test)

/*    Mask |  8 16 32 64
    -------+------------
    In   8 |  1  1  1  1
        16 |  1  1  1  1
        32 |  1  1  1  1
        64 |  1  1  1  1.  */
/* { dg-final { scan-assembler-times {\tst2b\t.z[0-9]} 16 } } */

/*    Mask |  8 16 32 64
    -------+------------
    In   8 |  2  2  2  2
        16 |  2  1  1  1 x2 (for _Float16)
        32 |  2  1  1  1
        64 |  2  1  1  1.  */
/* { dg-final { scan-assembler-times {\tst2h\t.z[0-9]} 28 } } */

/*    Mask |  8 16 32 64
    -------+------------
    In   8 |  4  4  4  4
        16 |  4  2  2  2
        32 |  4  2  1  1 x2 (for float)
        64 |  4  2  1  1.  */
/* { dg-final { scan-assembler-times {\tst2w\t.z[0-9]} 50 } } */

/*    Mask |  8 16 32 64
    -------+------------
    In   8 |  8  8  8  8
        16 |  8  4  4  4
        32 |  8  4  2  2
        64 |  8  4  2  1 x2 (for double).  */
/* { dg-final { scan-assembler-times {\tst2d\t.z[0-9]} 98 } } */
