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


int *mem;
int *pre_mem;        
int select(int *mem, int *addr);
int update(int *mem, int *addr, int value);

void inc(int *x)
  thmprv_post mem == update(pre_mem, x, select(pre_mem, x)+1);
{
  *x = *x + 1;
}

void callInc()
{
  int x = 6;
  int *p = &x;

  thmprv_assert x == 6;

  inc(p);

  thmprv_assert x == 7;
}
