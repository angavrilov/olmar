// t0265.cc
// does <dependent> match <dependent>?

template <class T>
struct A {
  int foo(typename T::type1 x);
  int foo(typename T::type2 x);
};


template <class T>
int A<T>::foo(typename T::type1 x)
{
  return 1;
}


template <class T>
int A<T>::foo(typename T::type2 x)
{
  return 2;
}
