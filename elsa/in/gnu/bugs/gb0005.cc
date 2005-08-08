// gb0005.cc
// failure to instantiate, failure to diagnose,
// though no diagnostic is required...

struct A {};

void f(A *x);

template <typename T>
struct B {
  void g(A const *p)
  {
    f(p);
  }
};
