// 7.3.3k.cc

// I'm supposed to find the call below ambiguous, but I do not,
// and it's not at all clear how to modify things so I do.

struct A { int x(); };
struct B : A { };
struct C : A {
  using A::x;
  int x(int);
};

struct D : B, C {
  using C::x;
  int x(double);
};
int f(D* d) {
  return d->x();            // ambiguous: B::x or C::x
}
