/* { dg-do compile } */

int
__attribute__ ((ifunc))
foo (int, int)
{ 
}

/* { dg-error "control reaches end of indirect function" ""  { target *-*-* } 7 } */
