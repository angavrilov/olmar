// t0177.cc
// a weird "typename" thing

class A {
  static int foo();
};

void bar()
{
  typename A::foo();
}
