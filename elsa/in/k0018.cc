// member function taking an array argument with default value __null

// Assertion failed: isArrayType(), file cc_type.cc line 870

// originally found in package anjuta

// ERR-MATCH: Assertion failed: isArrayType

struct S {
    void f(int a[] = __null);
};
