// t0146.cc
// operator=

// turn on operator overloading
int dummy();                    // line 5
void ddummy() { __testOverload(dummy(), 5); }

struct A {
  operator int& ();
  //ERROR(2): operator short& ();     // would create ambiguity
};

struct B {
  operator int ();
};

struct C {
  void operator= (int);       // line 18
};

// not a problem to have two conversion operators, if only
// one of them can yield a reference
struct D {
  operator int& ();
  operator int ();
};

struct E {
  operator int volatile & ();
};

enum { ENUMVAL };

void f1()
{
  A a;
  B b;
  C c;
  D d;
  E e;

  a = b;
  __testOverload(c = b, 18);
  //ERROR(1): b = b;           // 'b' can't convert to an L&
  d = b;
  e = 3;
  
  // similar to netscape test; hit all the operators
  int mFlags;
  mFlags = ENUMVAL;
  mFlags *= ENUMVAL;
  mFlags /= ENUMVAL;
  mFlags += ENUMVAL;
  mFlags -= ENUMVAL;
}
