// t0369.cc
// from Altac Edena
// error: there is no function called `f'

struct A {
  A(int a = f());

  void foo(int x, int y = f());

  int bar(int x, int y, int z = f())
  {
    return x+y+z;
  }

  void h()
  {
    foo(3,4);
    foo(5 /*A::f() implicit*/);

    bar(3,4,5);
    bar(6,7 /*A::f() implicit*/);
  }

  static int f();
};

void g()
{
  A a;

  a.foo(3,4);
  a.foo(5 /*A::f() passed implicitly*/);

  a.bar(3,4,5);
  a.bar(6,7 /*A::f() implicit*/);
}
