// t0144.cc
// operator++

// turn on operator overloading
int dummy();             // line 5
void ddummy() { __testOverload(dummy(), 5); }

struct A {
  operator int& ();
};

struct B {
  void operator++ ();              // line 13
};

struct C {
  operator volatile int& ();
};

struct D {
  operator int ();
};

struct E {
  operator const int& ();
};


void f1()
{
  A a;
  B b;
  C c;
  D d;
  E e;

  ++a;                          // A::operator int& ()
  __testOverload(++b, 13);      // B::operator ++ ()
  ++c;                          // C::operator volatile int& ()
  //ERROR(1): ++d;              // not an lvalue
  //ERROR(2): ++e;              // can't unify with VQ T&
}
