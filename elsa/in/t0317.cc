// t0317.cc
// static vs nonstatic in overload resoln

struct A {
  int foo(int);
  static int foo(int,int);

  void f();
};

void A::f()
{
  foo(2);
  foo(2,3);
}

// EOF
