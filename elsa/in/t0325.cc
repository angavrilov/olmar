template<class T> struct A {};

template<class T> struct B: A<T> {
  T &f(int i) {
    return A<T>::g(i);
  }
};

struct C {};

struct D {
  B<int*> shells;
  B<C*> delays;
};
