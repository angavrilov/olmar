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


//int *object(int *ptr);
//int offset(int *ptr);
//int length(int *obj);

thmprv_predicate int okSelOffset(int *mem, int offset);

int sum(int *array, int len)
  thmprv_pre(
    thmprv_forall(int i; (0<=i && i<len) ==> okSelOffset(mem, array+i))
  )

    //offset(array) == 0 &&
    //length(object(array)) == len
  // don't know what to say about result..
{                                           
  int tot = 0;
  int i = 0;
  
  while (i < len) {
    thmprv_invariant 
      //offset(array) == 0 &&
      //length(object(array)) == len &&
      0 <= i && i < len;

    tot = tot + array[i];
  }

  return tot;
}


int callSum()
{
  int arr[10];
  int *p = (int*)arr;
  return sum(p, 10);
}


