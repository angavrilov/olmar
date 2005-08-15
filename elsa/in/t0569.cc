// t0569.cc
// arg-dep lookup finds a friend

template <class T>
void f(T &t)
{
  g(t,t);
}

namespace N
{
  struct A {
    struct B {
    };

    friend void g(B &a, B &b);
  };
}

void foo()
{
  N::A::B e;
  f(e);
}
