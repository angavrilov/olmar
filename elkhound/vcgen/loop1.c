// loop1.c
// simple example of writing then reading via a loop

void assert(int b)
  thmprv_pre (
    int pre_mem = mem;
    b
  )
  thmprv_post (
    mem == pre_mem
  )
{}

int main()
{
  int x;
  int arr[10];

  for (x=0; x<5; x=x+1) {
    thmprv_invariant
      0 <= x && x < 5 &&
      (thmprv_forall int i;
        (0 <= i && i < x) ==> arr[i] == i);

    arr[x] = x;
  }

  thmprv_assert
    (thmprv_forall int i;
      (0 <= i && i < 5) ==> arr[i] == i);

  for (x=4; x>=0; x=x-1) {
    thmprv_invariant
      0 <= x && x <= 4 &&
      (thmprv_forall int i;
        (0 <= i && i < 5) ==> arr[i] == i);

    assert(arr[x] == x);
  }

  return 0;
}
