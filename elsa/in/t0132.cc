// t0132.cc
// looking more closely at 13.6 para 14

// For every T, where T is a pointer to object type, there exist
// candidate operator functions of the form
//
//   ptrdiff_t operator-(T, T);


struct Int {
  operator int* ();        // line 11
};

struct Float {
  operator float* ();      // line 15
};

// the built-in operators are all non-functions, and 0 is my code for that
enum { BUILTIN=0 };


void f()
{
  Int i;
  Float f;

  // there is no candidate for this, since int* and float* do not convert
  i-f;
}







struct Parent {};
struct Child : Parent {};

struct A {
  operator Parent* ();        // line 41
  operator Child* ();         // line 42
};

struct B {
  operator Child* ();         // line 46
};

void g()
{
  A a;
  B b;

  // candidate operator-(Parent*,Parent*):
  //   A -> Parent* is ambiguous, because either conversion would
  //   work, and we can't decide between them.     (WRONG)
  // candidate operator-(Child*,Child*):
  //   Both conversions are unique, A -> Child* compares as not worse
  //   than (the ambiguous) A -> Parent*, but B -> Child* compares
  //   as better than B -> Parent*, because the latter requires a
  //   derived-to-base standard conversion wheras the former is
  //   the identity standard conversion.  
  a-b;
}







