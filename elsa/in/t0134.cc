// t0134.cc
// test 'computeLUB'

enum {
  BAD=0,
  OK=1,
  AMBIG=2
};

//         A          .
//        / \         .
//       B   C        .
//        \ / \       .
//         E   D      .
struct A {};
struct B : A {};
struct C : A {};
struct E : B, C {};
struct D : C {};

//        F     G     .
//        | \ / |     .
//        | / \ |     .
//        H     I     .
struct F {};
struct G {};
struct H : F, G {};
struct I : F, G {};

void f()
{
  __computeLUB((int*)0, (int*)0, (int*)0, OK);
  __computeLUB((int*)0, (float*)0, 0, BAD);
  __computeLUB((int*)0, (int const*)0, (int const*)0, OK);
  __computeLUB((int volatile*)0, (int const*)0, (int volatile const*)0, OK);

  __computeLUB((int volatile **)0,
               (int const **)0,
               (int volatile const * const *)0, OK);

  __computeLUB((int volatile * const *)0,
               (int const * const *)0,
               (int volatile const * const *)0, OK);

  __computeLUB((int volatile **)0,
               (int const * volatile *)0,
               (int volatile const * const volatile *)0, OK);

  __computeLUB((A*)0, (B*)0, (A*)0, OK);
  __computeLUB((C*)0, (B*)0, (A*)0, OK);
  __computeLUB((E*)0, (D*)0, (C*)0, OK);
  __computeLUB((B*)0, (D*)0, (A*)0, OK);

  __computeLUB((H*)0, (G*)0, (G*)0, OK);
  __computeLUB((H*)0, (I*)0, 0, AMBIG);
  __computeLUB((H*)0, (A*)0, 0, BAD);
}
