// t0311.cc
// 5.16 para 6

//       A    E   .
//      / \       .
//     B   C      .
//      \ /       .
//       D        .
struct A {};
struct B : public A {};
struct C : public A {};
struct D : public B, public C {};
struct E {};

void foo(int x)
{
  A *pa = 0;
  B *pb = 0;
  C *pc = 0;
  D *pd = 0;
  E *pe = 0;

  __checkType(x? pb : pa, (A*)0);
  //(x? pb : pc);
  //(x? pb : pe);
  //(x? pd : pa);
  
  // I really don't know what the right spec is here, so I'm
  // just going to cross my fingers ....
}
