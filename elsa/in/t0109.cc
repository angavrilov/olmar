// t0109.cc
// more template tests

template <int n
  //ERROR(1): , int n     // duplicate param
  >
class Foo {};

class Bar;

void f()
{
  // unfortunately, my template implementation is too weak
  // to require arguments to templates, therefore I cannot
  // yet disambiguate these examples

  // <3> is a template argument
  new Foo< 3 > +4 > +5;
                               
  // <3> is not a template argument
  new Bar< 3 > +4 > +5;
}
