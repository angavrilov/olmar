// t0449.cc
// exercising some cases gcov says are not hit

// icc fails to reject: 3


// ------------------
template <class T>
struct A {
  int foo(T *t);
};

//ERROR(1): class A;

//ERROR(2): template class A;

//ERROR(3): template A<int>;


// ------------------
template <class T>
int foo(T t)
{ return 1; }

template int foo(int);

//ERROR(4): template int foo(float), foo(char);


// ------------------
//ERROR(5): namespace foo { int x; }


// ------------------
//ERROR(6): int bar { return x; }


// ------------------
struct B {
  ~B()
//ERROR(7):    : something(5)     // stray member init
  {}
};

struct C {
  C();
  ~C();
};

C::C() 
try
  {}
catch (int)
  {}

              C::~C()
//ERROR(8):   try
                {}
//ERROR(8):   catch (int)
//ERROR(8):     {}

// EOF
