// ptrptr.c
// experiment with pointer to pointer



void foo()
{
  int x,y,z;
  int *p, *q, *r;
  int **a, **b, **c;
  
  x=1;
  y=2;
  z=3;
  
  p=&x;
  q=&y;
  r=&z;
  
  a=&p;
  b=&q;
  c=&r;
  
  thmprv_assert **a == 1;     // works
  
  p = &y;

  thmprv_assert **a == 2;     // also works!
}


