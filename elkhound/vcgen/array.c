// array.c
// some simple array stuff
// NUMERRORS 3

//  int length(int *obj);
//  int *object(int *ptr);
//  int offset(int *ptr);

thmprv_predicate int okSelOffset(int mem, int offset);
thmprv_predicate int okSelOffsetRange(int mem, int offset, int len);
int length(int object);
int firstIndexOf(int *offset);
int *restOf(int *offset);
int sel(int mem, int index);
thmprv_predicate int isWhole(int x);
int *sub(int index, int *rest);

void init(int *a)
  //thmprv_pre length(object(a))==5 && offset(a)==0;

  //  thmprv_pre(   //0 <= offset(a) && offset(a)+5 <= length(object(a)) )
  //    thmprv_forall(int i; (0<=i && i<5) ==> okSelOffset(mem, a+i))
  //  )

  //  thmprv_pre(
  //    length(sel(mem, firstIndexOf(a))) >= 5 &&
  //    firstIndexOf(restOf(a)) == 0 &&
  //    isWhole(restOf(restOf(a))) &&

  //    // what a mess..
  //    thmprv_exists(int obj, idx, whl;
  //      a == sub(obj, sub(idx, whl)))
  //  )

  thmprv_pre(
    okSelOffsetRange(mem, a, 5)
  )
{
  a[0] = 0;
  a[1] = 1;
  a[2] = 2;
  a[3] = 3;
  a[4] = 4;
  a[5] = 5;         // ERROR(3)
}

void foo()
{
  int array[5];
  int *a = (int*)array;

  int x = a[0];
  x = a[4];

  x = a[5];     // ERROR(1)
  x = a[-1];    // ERROR(2)

  init(a);
}
