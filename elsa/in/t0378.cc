// t0378.cc
// qualified lookup of names after '~'

struct A {
  ~A();
};

typedef A B;
  
namespace N {
  struct C {
    ~C();
  };
};



void foo(A *a, N::C *c)
{
  a->~A();         // explicit dtor call w/o qualification
  a->A::~B();      // B is looked up in the scope of 'foo', *not* A (!)
  
  // I believe the following is legal C++, but both icc and gcc reject it.
  c->N::~C();      // C is looked up in the scope of N
}


// EOF
