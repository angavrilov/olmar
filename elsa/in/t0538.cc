// t0538.cc
// gotta push the semantic scope stack, not syntactic

namespace N {
  typedef int INT;
  struct A {
    void f();
  };
}

using N::A;

// the "N::" is not syntactally present, so Elsa doesn't
// push it onto the stack, so I don't find "INT"
void A::f()
{
  INT i;
}
