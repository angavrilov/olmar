// t0435.cc
// need for ArrayType with dependent size

template <int n>
struct A {
  int foo(int (*p)[n]);
  int foo(int (*p)[n+1]);
};

template <int n>
int A<n>::foo(int (*p)[n])
{
  return 1;
}

template <int n>
int A<n>::foo(int (*p)[n+1])
{
  return 2;
}

void f()
{
  A<3> a;
  int arr1[3];
  int arr2[3];
  
  a.foo(&arr1);
  a.foo(&arr2);
}


// EOF
