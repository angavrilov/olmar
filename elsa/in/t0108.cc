// t0108.cc
// simple template argument tests

template <int n>
class Foo {
  int x;
  Foo() { x=n; }    // assign from template argument
  int f();
  int g();
};

template <int n>
int Foo<n>::f()
{
  return n;
}

// should be legal to change parameter name, I think
template <int m>
int Foo<m>::f()
{
  return m;
}

int main()
{
  Foo<3> f;
  f.f();
  f.x;
  
  return f.g();
}
