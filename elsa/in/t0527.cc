// t0527.cc
// use an enumerator to hide a type
      
// well-known: use a variable
struct A {};
int A;                       

// news to me: use an enumerator
struct B {};
enum { B };

// conflict with explicit typedef isn't ok, though
typedef int C;
//ERROR(1): enum { C };

// nor is it ok to conflict with a variable
int d;
//ERROR(2): enum { d };

// EOF
