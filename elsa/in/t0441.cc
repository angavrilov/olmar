// t0441.cc
// out-of-line defn of a member template

template <class T>
struct A {
  template <class U>
  struct C;
};

template <class T>
template <class U>
struct A<T>::C {
  int c1();
};

void foo()
{
  A<int>::C<int> c;
  c.c1();
}


// EOF
