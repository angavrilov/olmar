// t0297.cc
// overloaded operator()

struct A {
  int operator() ();            // line 5
  int operator() (int);         // line 6
  int operator() (int,int);     // line 7
};

void foo()
{
  A a;

  __testOverload( a(),     5);
  __testOverload( a(3),    6);
  __testOverload( a(4,5),  7);
}


