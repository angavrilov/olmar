// compound initializer for struct which was earlier forward-declared as class

// originally found in package ''

// a.ii:8:3: error: cannot convert initializer type `int' to type `struct S'

// ERR-MATCH: cannot convert initializer type `.*?' to type `.*?'

class S;
struct S {
  int foo, bar;
};

S s = { 42, 84 };
