// template <class T> void foo(S1<T> s1, const typename T::T2 t2) {}

// originally found in package 'aspell'

// ERR-MATCH:

template <class T> struct S1 {};

template <class T> void foo(S1<T> s1, const typename T::T2 t2) {}
