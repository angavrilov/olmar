// overloaded function resolution in an array

// originally found in package fltk

// error: failed to resolve address-of of overloaded function `foo' assigned
// to type `struct Anon_struct_1 []'

// error: failed to resolve address-of of overloaded function `foo' assigned
// to type `void (*[])(int /*anon*/, long int /*anon*/)'

typedef void (*funcType)(int, long);

void foo(int x);

void foo(int x, long y);

funcType array1[] = {foo};

struct {
    funcType f;
} array2[] = { {foo} };
