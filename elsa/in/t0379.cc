// t0379.cc
// typedef as qualifier

namespace A {
  struct B {
    int f();
    int g();
  };
  int x;
  
  namespace C {
    int y;
  }
}


// both A and B get searched, since both appear explicitly
int A::B::f()
{
  return x;
}


typedef A::B AB;

// A does not appear explicitly, so does not get searched
int AB::g()
{
  return x;
}


// can't nominate a namespace with a typedef
//ERROR(1): typedef A::C AC;


// EOF
