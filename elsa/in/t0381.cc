// t0381.cc
// some games with elaborated type specifiers

struct C {};
struct D {};

int C;

int foo()
{
  {
    // would make use of 'D' below refer to typedef
    //ERROR(1): typedef struct C D;
    
    struct D d;
  }


  struct C c;

  return C;
}


