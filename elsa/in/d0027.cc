struct B {};
struct C {
  C (B *a);
};
C f = new B;
