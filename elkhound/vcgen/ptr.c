// ptr.c
// some simple pointer manipulations

void foo()
{
  int x = 5, y = 7;
  int *p = &x;     
  int *q = &y;

  *p = 6;

  thmprv_assert x == 6;
  thmprv_assert *q == 7;


}
  

