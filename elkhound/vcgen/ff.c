// ff.c
// testing factflow mechanism

void foo(int x)
  thmprv_pre x >= 0;
{
  int arr[5];
  while (x < 5) {
    thmprv_invariant true;     // x<5 inferred
    arr[x] = x;                // verifies can prove 0 <= x && x < 5
    x = x + 1;
  }
}


