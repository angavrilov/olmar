// t0187.cc
// template class with template variable as the base
// (this probably duplicates some other existing test..)

template <class T>
struct C : T {
};

struct B {
  int a;
};

int foo()
{
  C<B> c;
  return c.a;     // refers to B::a, since C<B> inherits B
}

