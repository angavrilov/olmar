// t0111.cc
// some nontype template arguments
          
template <int n>
class Foo {
public:
  int arr[n];
};

void f()
{
  Foo<3> x;
  Foo<1+2> y;   // same type!
  Foo<2+2> z;   // different type!
  x.arr;
  y.arr;
  z.arr;
}

