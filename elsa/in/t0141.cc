// t0141.cc
// test operator~

// turn on operator overloading
int dummy();             // line 5
void ddummy() { __testOverload(dummy(), 5); }

enum E1 {};
enum E2 {};

void operator~ (E1);     // line 11

void f1()
{
  E1 e1;
  E2 e2;
  
  __testOverload(~e1, 11);
  __testOverload(~e2, 0);
}




