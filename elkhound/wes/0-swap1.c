void foo(int *ptr, int *x) {
  *ptr = 5;
  *x = 7;
  return;
}

int main() {
  int x, y;

  int final_x, final_y;

  x = 55;
  y = 77;

  foo(&x,&y);

  final_x = x;
  final_y = y;

  assert(final_x == 5);

  return 0;
}
