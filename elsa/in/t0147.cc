// t0147.cc
// inherit a conversion operator

struct A {
  operator int& ();
};

struct B : A {
  operator int ();
};

void f1()
{
  B b;

  // needs B to inherit A::operator int& ()
  b = 3;
}
