// t0561.cc
// template parameter of ptr-to-func type

template <int (*f)(int)>
struct A {
  int foo(int x)
  {
    return f(x);
  }
};

int f1(int);

A<f1> af1;

int f2(int) { return 1; }

A<f2> af2;

int f3(int) { return 1; }

A<&f3> af3;

