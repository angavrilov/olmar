// array.c
// some simple array stuff

void foo()
{
  int array[5];
  int x = array[0];
  x = array[4];
  
  x = array[5];     // bad!
  x = array[-1];    // bad!
}
