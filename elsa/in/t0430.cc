// t0430.cc
// build a dependent qualified type out of a typedef
 
template <class T>
struct A {
  typedef T *foo_t;
};

template <class T>
void foo(T t)
{
  typedef A<T> AT;
  typedef typename AT::foo_t bar;
}

void goo()
{
  int x = 2;
  foo(x);
}
