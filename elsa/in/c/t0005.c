// test constructs where doElaboration makes a difference and that are
// still legal in C

// implicit cdtors should not be being made
struct A {
  int x;
};

struct A f() {
  // declarator should not make cdtor statements
  struct A a2;
  struct A a3 = {1};            // IN_compound
  struct A a4 = (struct A) {2};
  struct A a5;
  struct A a6 = a5;             // IN_expr
  return a2;
}
