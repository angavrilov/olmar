// t0254.cc
// dependent inner template scope and funky 'template' keyword

template <class S, class T>
struct A {
  typedef typename S::template Inner<T>::its_type my_type;
  
  // must use the 'template' keyword
  //ERROR(1): typedef typename S::Inner<T>::its_type another_my_type;
};

// one possible type to use in place of 'S'
struct Outer {
  template <class T>
  struct Inner {
    typedef T *its_type;
  };
};

// put it all together
int f()
{
  A<Outer, int>::my_type pointer_to_integer;

  int x = *pointer_to_integer;
  
  return x;
}


// EOF
