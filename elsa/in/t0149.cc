// t0149.cc
// operator%

struct A {
  operator int& ();
};

struct Av {
  operator int volatile & ();
};

struct B {
  operator float& ();
};

void f1()
{
  A a;
  Av av;
  B b;

  a %= 3;
  av %= 3;
  //ERROR(1): b %= 3;     // can't convert to reference-to-integral
}


