// cc.in82
// ambiguity with angle brackets?

template <int n> class C { /*...*/ };

int main()
{
  int x;
  C<  3+4  > a;      // ok; same as C<7> a;
  C<  3<4  > b;      // ok; same as C<1> b;
  //C<  3>4  > c;      // no!
  C<  3&&4  > c;      // no!
  C< (3>4) > d;      // ok; same as C<0> d;

  //new C< 3 > +4 > +5;
}
