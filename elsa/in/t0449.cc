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


// ------------------
struct C {
  //ERROR(18): auto int x;
  //ERROR(19): extern int y;
  //ERROR(20): register int z;
};


// ------------------
//ERROR(21): int AE_THREE;    // would conflict
enum AnotherEnum { AE_ONE, AE_TWO, AE_THREE, AE_FOUR };
//ERROR(22): int AE_TWO;      // would conflict


// ------------------
//ERROR(23): int operator+;


// ------------------
int operator++ (B &c, int);
//ERROR(24): int operator++ (C &c, float);
//ERROR(25): int operator++ (C &c, int = 2);


// ------------------
struct D {
  int f();
  int f(int);
};

//ERROR(26): int D::f;


// ------------------
//ERROR(27): int &*ptr_to_ref;
//ERROR(28): int &&ref_to_ref;

typedef int &reference;
//ERROR(29): reference &ref_to_ref;


// ------------------
struct E {
  E();
};
 
// grouping parens on a ctor declarator
(E::E)()
{}


// ------------------
// destructors must be class members
//ERROR(30): ~foo() {}


// ------------------
//ERROR(31): int someFunc(int x(3));
//ERROR(32): int someFunc(int x = {3});


// ------------------
struct F {
  //ERROR(33): operator int (int);
};


// ------------------
//ERROR(34): reference array_of_ref[3];

typedef void VOID;
//ERROR(35): VOID array_of_void[4];

//ERROR(36): Func array_of_func[5];

int *makeSomeInts(int x)
{
  //ERROR(37): return new (int[/*oops*/]);

  //ERROR(38): return new int[4][x];
}




// EOF
