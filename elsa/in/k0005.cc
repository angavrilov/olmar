// declaring a constructor with class name

// originally found in package groff

// error: undeclared identifier `S::S'

// error: the name `S::S' is overloaded, but the type `()(int x)' doesn't
// match any of the 2 declar

struct S {
    S::S(int x);
};
S::S(int x) {}
