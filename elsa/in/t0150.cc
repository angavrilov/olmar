// t0150.cc
// operator= with correlated parameters

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

struct Ap {
  operator A* ();
};

struct Apr {
  operator A* & ();
};

struct Apvr {
  operator A* volatile & ();
};

struct Bp {
  operator B* ();
};

struct Bpr {
  operator B* & ();
};

struct Cp {
  operator C* ();
};

void f1()
{
  Ap ap;
  Apr apr;
  Apvr apvr;
  Bp bp;
  Bpr bpr;
  Cp cp;

  apr = ap;
  apvr = ap;
  apr = ap;
  //ERROR(1): ap = ap;       // lhs isn't reference
  //ERROR(2): bpr = cp;      // would not be sound
  bpr = bp;
  apr = bp;
}


