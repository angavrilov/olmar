// array.c
// some simple array stuff
// NUMERRORS 2

int length(int *obj);
int *object(int *ptr);
int offset(int *ptr);

void init(int *a)
  thmprv_pre length(object(a))==5 &&
             offset(a)==0;
{
  a[0] = 0;
  a[1] = 1;
  a[2] = 2;
  a[3] = 3;
  a[4] = 4;
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
