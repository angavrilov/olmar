// t0127.cc
// some operator overload resolution tests

class A {};

class B : public A {
public:
  void operator+(A &a);             // 8
};

void operator+(A &a, A &b);         // 11
void operator+(B &a, B &b);         // 12

void some_random_crap();            // 14

// the built-in operators are all non-functions, and 0 is my code for that
enum { BUILTIN=0 };

class C {
public:
  operator int ();                  // 21
};

class D {
public:
  operator int* ();                 // 26
};

class E {
public:
  operator int ();                  // 31
  operator int* ();                 // 32
  void operator-(A &a);             // 33
};

void f()
{
  A a1,a2;
  B b1,b2;
  C c;
  D d;
  E e;


  // turn on overload resolution
  __testOverload(some_random_crap(), 14);

  __testOverload(a1+a2, 11);

  __testOverload(b1+b2, 12);

  __testOverload(b1+a1, 8);

  __testOverload(c+1, BUILTIN);
  __testOverload(c+(char)1, BUILTIN);

  __testOverload(d+1, BUILTIN);
  __testOverload(1+d, BUILTIN);

  //ERROR(1): __testOverload(e+1, BUILTIN);    // ambiguous
                   
  // BIN_MINUS
  __testOverload(e-a1, 33);
  __testOverload(c-1, BUILTIN);
  __testOverload(d-1, BUILTIN);
  __testOverload(d-d, BUILTIN);
  //ERROR(2): __testOverload(1-d, BUILTIN);    // no viable
}
