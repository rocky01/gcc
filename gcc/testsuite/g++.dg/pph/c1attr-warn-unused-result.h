#ifndef __PPH_GUARD_H
#define __PPH_GUARD_H
/* { dg-options "-w" } */

#define WUR __attribute__((warn_unused_result))
#define WURAI __attribute__((warn_unused_result, always_inline)) inline
typedef WUR int (*fnt) (void);

typedef struct { long i; } A;
typedef struct { long i; long j; } B;
typedef struct { char big[1024]; fnt fn; } C;

WUR int check1 (void);
WUR void check2 (void);
int foo WUR;
int bar (void);
extern WURAI int check3 (void) { return bar (); }
WUR A check4 (void);
WUR B check5 (void);
WUR C check6 (void);
A bar7 (void);
B bar8 (void);
C bar9 (void);
extern WURAI A check7 (void) { return bar7 (); }
extern WURAI B check8 (void) { return bar8 (); }
extern WURAI C check9 (void) { return bar9 (); }
/* This is useful for checking whether return value of statement
   expressions (returning int in this case) is used.  */
extern WURAI int check_int_result (int res) { return res; }
#define GU(v) ({ int e = 0; (v) = bar (); if ((v) < 23) e = 14; e; })
fnt fnptr;
WUR int check10 (void);
int baz (void);
extern WURAI int check11 (void) { return baz (); }
int k;

void
test (void)
{
  int i = 0, j;
  const fnt pcheck1 = check1;
  const fnt pcheck3 = check3;
  A a;
  B b;
  C c;
  if (check1 ())
    return;
  i += check1 ();
  i += ({ check1 (); });
  check1 ();
  (void) check1 ();
  check1 (), bar ();
  check2 ();
  (void) check2 ();
  check2 (), bar ();
  if (check3 ())
    return;
  i += check3 ();
  i += ({ check3 (); });
  check3 ();
  (void) check3 ();
  check3 (), bar ();
  a = check4 ();
  if (a.i)
    return;
  if (check4 ().i)
    return;
  if (({ check4 (); }).i)
    return;
  check4 ();
  (void) check4 ();
  check4 (), bar ();
  b = check5 ();
  if (b.i + b.j)
    return;
  if (check5 ().j)
    return;
  if (({ check5 (); }).j)
    return;
  check5 ();
  (void) check5 ();
  check5 (), bar ();
  c = check6 ();
  if (c.big[12] + c.big[29])
    return;
  if (check6 ().big[27])
    return;
  if (({ check6 (); }).big[0])
    return;
  check6 ();
  (void) check6 ();
  check6 (), bar ();
  a = check7 ();
  if (a.i)
    return;
  if (check7 ().i)
    return;
  if (({ check7 (); }).i)
    return;
  check7 ();
  (void) check7 ();
  check7 (), bar ();
  b = check8 ();
  if (b.i + b.j)
    return;
  if (check8 ().j)
    return;
  if (({ check8 (); }).j)
    return;
  check8 ();
  (void) check8 ();
  check8 (), bar ();
  c = check9 ();
  if (c.big[12] + c.big[29])
    return;
  if (check9 ().big[27])
    return;
  if (({ check9 (); }).big[0])
    return;
  check9 ();
  (void) check9 ();
  check9 (), bar ();
  if (check_int_result (GU (j)))
    return;
  i += check_int_result (GU (j));
  i += ({ check_int_result (GU (j)); });
  check_int_result (GU (j));
  (void) check_int_result (GU (j));
  check_int_result (GU (j)), bar ();
  if (fnptr ())
    return;
  i += fnptr ();
  i += ({ fnptr (); });
  fnptr ();
  (void) fnptr ();
  fnptr (), bar ();
  fnptr = check1;
  if (fnptr ())
    return;
  i += fnptr ();
  i += ({ fnptr (); });
  fnptr ();
  (void) fnptr ();
  fnptr (), bar ();
  fnptr = check3;
  if (fnptr ())
    return;
  i += fnptr ();
  i += ({ fnptr (); });
  fnptr ();
  (void) fnptr ();
  fnptr (), bar ();
  if (bar9 ().fn ())
    return;
  i += bar9 ().fn ();
  i += ({ bar9 ().fn (); });
  bar9 ().fn ();
  (void) bar9 ().fn ();
  bar9 ().fn (), bar ();
  if ((k ? check1 : check10) ())
    return;
  i += (k ? check1 : check10) ();
  i += ({ (k ? check1 : check10) (); });
  (k ? check1 : check10) ();
  (void) (k ? check1 : check10) ();
  (k ? check1 : check10) (), bar ();
  if ((k ? check3 : check11) ())
    return;
  i += (k ? check3 : check11) ();
  i += ({ (k ? check3 : check11) (); });
  (k ? check3 : check11) ();
  (void) (k ? check3 : check11) ();
  (k ? check3 : check11) (), bar ();
  if (pcheck1 ())
    return;
  i += pcheck1 ();
  i += ({ pcheck1 (); });
  pcheck1 ();
  (void) pcheck1 ();
  pcheck1 (), bar ();
  if (pcheck3 ())
    return;
  i += pcheck3 ();
  i += ({ pcheck3 (); });
  pcheck3 ();
  (void) pcheck3 ();
  pcheck3 (), bar ();
}
#endif
