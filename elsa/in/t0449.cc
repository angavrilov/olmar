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
  
  B();
  
  int x;
  static int y;
  int f(int);
};

B::B()
  : x(3)
//ERROR(8):  , y(7)     // member init of static data
//ERROR(9):  , f(4)     // member init of a member function
{}


// ------------------
// two things:
//   - non-member constructor
//   - member inits on non-constructor
//ERROR(10):  d()
//ERROR(10):    : something(6)
//ERROR(10):  {}


// ------------------
void func()
{
  struct A {
    // local class template
    //ERROR(11): template <class T> struct B {};
    //ERROR(12): template <class T> int foo(T *t);
  };
};


// ------------------
typedef int Func(int);
//ERROR(13): Func const cfunc;


// ------------------
//ERROR(14): friend 
class MyFriend {
  int x;
};


// ------------------
//ERROR(15): template <class T>
union CrazyUnion {
  int x;
};


// ------------------
//ERROR(16): enum B::Blah blah;


// ------------------
enum SomeEnum { SE_ONE };
//ERROR(17): enum SomeEnum { SE_TWO };


// EOF
