// t0126.cc
// test overload resolution of member functions

struct A {
  int f();        // line 5
  int f(int);     // line 6
};

void func()
{

  // TODO: extend __testOverload to test these; they do appear
  // to work

  A a;
  a.f();
  a.f(3);
}
