// array.c
// some simple array stuff
// NUMERRORS 3

int length(int *obj);
int *object(int *ptr);
int offset(int *ptr);

void init(int *a)
  //thmprv_pre length(object(a))==5 && offset(a)==0;
  thmprv_pre 0 <= offset(a) && offset(a)+5 <= length(object(a));
{
  a[0] = 0;
  a[1] = 1;
  a[2] = 2;
  a[3] = 3;
  a[4] = 4;
  a[5] = 5;         // ERROR 3
}

void foo()
{
  int array[5];
  int x = array[0];
  x = array[4];

  x = array[5];     // ERROR 1
  x = array[-1];    // ERROR 2

  init(array);
}
