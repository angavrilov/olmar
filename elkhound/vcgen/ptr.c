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
  


void bar()
{
  int x, y;
  int *p = &x;
  int *q = &y;
          
  *p = 5;
  *q = 7;

  thmprv_assert x == 5;
  thmprv_assert y == 7;


}
