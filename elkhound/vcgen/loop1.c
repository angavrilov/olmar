// loop1.c
// simple example of writing then reading via a loop

void assert(int b)
  thmprv_pre b;
{}

int main()
{
  int x;
  int arr[10];

  for (x=0;x<5;x++) {
    thmprv_invariant
      thmprv_forall int i;
        (0 <= i && i < x) ==> arr[i] == i;

    arr[x] = x;
  }

  //thmprv_invariant
  //  thmprv_forall int i;
  //    (0 <= i && i < 5) ==> arr[i] == i;
        
  for (x=4;x>=0;x--) {
    thmprv_invariant x >= 0 && x <= 4;

    assert(arr[x] == x);
  }

  return 0;
}
