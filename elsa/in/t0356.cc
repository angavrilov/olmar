// t0356.cc
// argument-dependent lookup bug reported by Altac Edena
// this is very similar to the example of 3.4.2 para 2

namespace N {
  class B {};
  void g(B);
}

class A {
public:
  void f() {
    N::B b;
    g(b);
  }
};
