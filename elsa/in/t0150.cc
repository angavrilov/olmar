// t0150.cc
// operator= with correlated parameters

//         A          .
//        / \         .
//       B   C        .
//        \ /         .
//         D          .
struct A {};
struct B : A {};
struct C : A {};
struct D : C {};
enum E { E_VAL };

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

struct Er {
  operator E& ();
};

struct Apm {
  operator int A::* ();
};

struct Apmr {
  operator int A::* & ();
};

struct Bpm {
  operator int B::* ();
};

struct Bpmr {
  operator int B::* & ();
};



void f1()
{
  Ap ap;
  Apr apr;
  Apvr apvr;
  Bp bp;
  Bpr bpr;
  Cp cp;
  Er er;
  Apm apm;
  Apmr apmr;
  Bpm bpm;
  Bpmr bpmr;

  apr = ap;
  apvr = ap;
  apr = ap;
  //ERROR(1): ap = ap;       // lhs isn't reference
  //ERROR(2): bpr = cp;      // would not be sound
  bpr = bp;
  apr = bp;

  er = E_VAL;

  bpmr = bpm;
  bpmr = apm;    // inverted subtyping for ptr-to-member
  //ERROR(3): apmr = bpm;    // violates inverted order

}


