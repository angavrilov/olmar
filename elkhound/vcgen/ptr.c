// ptr.c
// some simple pointer manipulations

void foo()
{
  int x = 5;
  int *y = &x;
  
  *y = 6;
  
  thmprv_assert x == 6;
}
  

