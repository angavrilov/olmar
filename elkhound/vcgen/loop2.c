// loop2.c
// simple example of writing then reading via a loop
// different from loop1.c in use of inference, abstraction

void assert(int b)
  thmprv_pre(
    int pre_mem = mem;
    b)
  thmprv_post(
    mem == pre_mem)
{}

int main()
{
  int x, y;
  int arr[5];

  #define equalsIndexTo(len)                      \
    (thmprv_forall int i;                         \
      (0 <= i && i < len) ==> arr[i] == i);

  for (x=0; x<5; x=x+1) {
    thmprv_invariant
      0 <= x &&
      //x < 5 &&
      equalsIndexTo(x);

    arr[x] = x;
  }

  thmprv_assert equalsIndexTo(5);

  for (y=4; y>=0; y=y-1) {
    thmprv_invariant
      //0 <= y &&
      y <= 4 &&
      //(thmprv_forall int i;
      // (0 <= i && i < 5) ==> arr[i] == i) &&
      true;

    assert(arr[y] == y);
  }

  return 0;
}
