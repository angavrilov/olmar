// t0454.cc
// variety of template argument syntax

template <int n>
struct A {};
  
int f();
  
struct B {
  int b;
};

void foo()
{
  B b;
  int x;
  int *p;

  A< f() > a1;        // error: not const
  A< b.b > a2;        // error: not const
  A< -1 > a3;
  A< x++ > a4;        // error: not const (side effect)
  A< 1+2 > a5;
  A< 1>2 > a6;        // error: unparenthesized '>'
  A< &x > a7;         // error: type mismatch
  A< *p > a8;         // error: not const
  A< 1?2:3 > a9;
  A< x=3 > a10;       // error: not const (side effect)
  A< delete p > a11;  // error: not const (side effect)
  A< throw x > a12;   // error: not const (side effect)
}


// EOF
