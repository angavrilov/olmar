// ff.c
// testing factflow mechanism

void foo(int x, int j)
  thmprv_pre 0 <= x && x < 5;
{
  int arr[5];
  while (x < 5) {
    thmprv_invariant true;     // x<5 inferred
    arr[x] = x;                // verifies can prove 0 <= x && x < 5
    x = x + 1;                 // x>=0 retained because is monotonic
  }
  
  while (j < 6) {
    thmprv_invariant true;     // want x<=5 inferred
    arr[x-1] = j;
    j = j + 1;
  }
}


