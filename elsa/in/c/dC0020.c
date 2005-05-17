struct A {};

void f() {
  (int) & (((struct {struct A y;}*)0)->y);
}
