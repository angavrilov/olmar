// t0549.cc
// apply const to reference during template arg deduction


// actually this is perfectly legal (8.3.2p1)
typedef float &FLOATREF;
float q;
FLOATREF const fr = q;


template <class T>
void f(T const x);

void foo()
{
  int x = 0;
  f<int&>(x);
}


struct A {
  typedef int &INTREF;
};


template <class T>
void g(T *t, typename T::INTREF const r);

void bar()
{
  A *a = 0;
  int x = 0;
  g(a, x);
}



template <class T>
struct B {
  void h(typename T::INTREF const r);
};

B<A> b;
