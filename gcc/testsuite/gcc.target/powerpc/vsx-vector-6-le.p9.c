/* { dg-do compile { target { powerpc64le-*-* && lp64 } } } */
/* { dg-skip-if "" { powerpc*-*-darwin* } } */
/* { dg-require-effective-target powerpc_p9vector_ok } */
/* { dg-options "-mvsx -O2 -mcpu=power9" } */
/* { dg-skip-if "do not override -mcpu" { powerpc*-*-* } { "-mcpu=*" } { "-mcpu=power9" } } */

/* Expected instruction counts for Little Endian targeting Power9. */

/* { dg-final { scan-assembler-times "xvabsdp" 1 } } */
/* { dg-final { scan-assembler-times "xvadddp" 1 } } */
/* { dg-final { scan-assembler-times "xxlnor" 7 } } */
/* { dg-final { scan-assembler-times "xxlor" 20 } } */
/* { dg-final { scan-assembler-times "xvcmpeqdp" 5 } } */
/* { dg-final { scan-assembler-times "xvcmpgtdp" 8 } } */
/* { dg-final { scan-assembler-times "xvcmpgedp" 8 } } */
/* { dg-final { scan-assembler-times "xvrdpim" 1 } } */
/* { dg-final { scan-assembler-times "xvmaddadp" 1 } } */
/* { dg-final { scan-assembler-times "xvmsubadp" 1 } } */
/* { dg-final { scan-assembler-times "xvsubdp" 1 } } */
/* { dg-final { scan-assembler-times "xvmaxdp" 1 } } */
/* { dg-final { scan-assembler-times "xvmindp" 1 } } */
/* { dg-final { scan-assembler-times "xvmuldp" 1 } } */
/* { dg-final { scan-assembler-times "vperm" 1 } } */
/* { dg-final { scan-assembler-times "xvrdpic" 1 } } */
/* { dg-final { scan-assembler-times "xvsqrtdp" 1 } } */
/* { dg-final { scan-assembler-times "xvrdpiz" 1 } } */
/* { dg-final { scan-assembler-times "xvmsubasp" 1 } } */
/* { dg-final { scan-assembler-times "xvnmaddasp" 1 } } */
/* { dg-final { scan-assembler-times "vmsumshs" 1 } } */
/* { dg-final { scan-assembler-times "xxland" 13 } } */

/* Source code for the test in vsx-vector-6.h */
#include "vsx-vector-6.h"
