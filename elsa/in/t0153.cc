// t0153.cc
// explicit ctor call expr, as RHS of assignment

struct A {
  A() {}
  A(int x) {}
};
int main() {
  int arg = 3;
  A a;
  a = A(arg);
}
