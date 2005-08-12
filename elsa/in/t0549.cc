// t0549.cc
// apply const to reference during template arg deduction

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


