/* { dg-do compile } */
/* { dg-options "-O2 -mindirect-branch=thunk -mindirect-branch-loop=lfence -fno-pic" } */

typedef void (*dispatch_t)(long offset);

dispatch_t dispatch;

void
male_indirect_jump (long offset)
{
  dispatch(offset);
}

/* { dg-final { scan-assembler "push(?:l|q)\[ \t\]*_?dispatch" { target { ! x32 } } } } */
/* { dg-final { scan-assembler "jmp\[ \t\]*__x86_indirect_thunk" { target { ! x32 } } } } */
/* { dg-final { scan-assembler "jmp\[ \t\]*__x86_indirect_thunk_(r|e)ax" { target x32 } } } */
/* { dg-final { scan-assembler "jmp\[ \t\]*\.LIND" } } */
/* { dg-final { scan-assembler "call\[ \t\]*\.LIND" } } */
/* { dg-final { scan-assembler {\tlfence} } } */
