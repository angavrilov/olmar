// t0192.cc
// template function forward declarations

template <class T>
int foo(T *t);

int g()
{
  int *x;
  return foo<int>(x);
}

template <class T>
int foo(T *t)
{
  return sizeof(*t);
}


