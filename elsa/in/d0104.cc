// /home/ballB/ddd-3.3.1-13/Command-qJh2.ii:2579:15:
// error: reference to `rdbuf' is ambiguous, because it could either
// refer to strstreambase::rdbuf or ios::rdbuf

// error: reference to `f' is ambiguous, because it could either refer to C::f or A::f

struct A {
  int *f() {}
};

struct B:virtual A {};

struct C:virtual A {
  int *f() {}
};

struct D: C, B {};

void f(D &d)
{
  d.f();
}
