// error: no template parameter list supplied for `A'

// note that the 'static' is critical for the bug

template<class T> class A {
  static int s;
};

int A<int>::s = 0;
