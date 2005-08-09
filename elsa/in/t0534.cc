// t0534.cc
// template argument deduction with partially-specified arguments

// According to my interpretation of the standard, *all* of the
// function calls below are invalid, because they pass an 'int'
// argument to an 'unsigned' template parameter, but template
// argument deduction requires that the match be nearly exact.
//
// However, both GCC and ICC accept most of them, apparently
// with the policy that a parameter type that is concrete after
// explicitly-provided template arguments are substituted is to
// be ignored during deduction, and hence can vary considerably
// from the argument type.

template <class T>
void f(const T &a, const T &b);

template <class T, class U>
void g(const T &a, const T &b, U u);

template <class T>
void h(T, T);

template <class T>
void j(T, unsigned);

void foo()
{
  unsigned u;
  int i;

  //f(u, i);          // rejected by both GCC and ICC
  f<unsigned>(u, i);

  g<unsigned>(u, i, 3);

  //h(u, i);          // rejected by both GCC and ICC
  h<unsigned>(u, i);

  j<unsigned>(u, i);
  
  j(u, i);
}
