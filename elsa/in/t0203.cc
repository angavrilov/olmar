// t0203.cc
// tricky use of typename and lookups

namespace M
{
  template <class T>
  struct B {
    typedef int myint;
  };
}

namespace N
{
  using M::B;

  template <class T>
  struct C {
    typename B<int>::myint a;
  };
}

N::C<char> c;
