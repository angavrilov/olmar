// t0574.cc
// trouble disambiguating a conversion

struct A {
  A(A const &);    // would be implicitly defined even if not explicitly present
  A(int *);
};

class B {
public:
  operator A();
private:
  // GCC and ICC accept the "a2 = b" line with this entry
  // private, so they must be selecting the other one
  operator int*();
};


void foo(B &b)
{
  // GCC and ICC agree that this is ambiguous
  //ERROR(1): A a1(b);

  // but this is not; see 8.5p14
  A a2 = b;
}
