// invar.c
// messing with invariant points

int f(int x)
  thmprv_pre(x > 10)
  thmprv_post(result > 40)
{
  int y = x;
  thmprv_assert y > 10;

  y = y + 10;

  thmprv_invariant y > 20;

  y = y + 10;

  thmprv_assert y > 30;

  return y + 10;
}


int *object(int *ptr);
int offset(int *ptr);
int length(int *obj);

int sum(int *array, int len)
  thmprv_pre(
    offset(array) == 0 &&
    length(object(array)) == len )
  // don't know what to say about result..
{                                           
  int tot = 0;
  int i = 0;
  
  while (i < len) {
    thmprv_invariant 
      offset(array) == 0 &&
      length(object(array)) == len &&
      0 <= i && i < len;

    tot = tot + array[i];
  }

  return tot;
}




