// t0118.cc
// test getImplicitConversion

// copied from stdconv.h
enum StandardConversion {
  SC_IDENTITY        = 0x00,  // types are identical

  // conversion group 1 (comes first)
  SC_LVAL_TO_RVAL    = 0x01,  // 4.1: int& -> int
  SC_ARRAY_TO_PTR    = 0x02,  // 4.2: char[] -> char*
  SC_FUNC_TO_PTR     = 0x03,  // 4.3: int ()(int) -> int (*)(int)
  SC_GROUP_1_MASK    = 0x03,

  // conversion group 3 (comes last conceptually)
  SC_QUAL_CONV       = 0x04,  // 4.4: int* -> int const*
  SC_GROUP_3_MASK    = 0x04,

  // conversion group 2 (goes in the middle)
  SC_INT_PROM        = 0x10,  // 4.5: int... -> int..., no info loss possible
  SC_FLOAT_PROM      = 0x20,  // 4.6: float -> double, no info loss possible
  SC_INT_CONV        = 0x30,  // 4.7: int... -> int..., info loss possible
  SC_FLOAT_CONV      = 0x40,  // 4.8: float... -> float..., info loss possible
  SC_FLOAT_INT_CONV  = 0x50,  // 4.9: int... <-> float..., info loss possible
  SC_PTR_CONV        = 0x60,  // 4.10: 0 -> Foo*, Child* -> Parent*
  SC_PTR_MEMB_CONV   = 0x70,  // 4.11: int Child::* -> int Parent::*
  SC_BOOL_CONV       = 0x80,  // 4.12: various types <-> bool
  SC_GROUP_2_MASK    = 0xF0,

  SC_ERROR           = 0xFF,  // cannot convert
};

// copied from implconv.h
enum Kind {
  IC_NONE,             // no conversion possible
  IC_STANDARD,         // 13.3.3.1.1: standard conversion sequence
  IC_USER_DEFINED,     // 13.3.3.1.2: user-defined conversion sequence
  IC_ELLIPSIS,         // 13.3.3.1.3: ellipsis conversion sequence
  IC_AMBIGUOUS,        // 13.3.3.1 para 10
  NUM_KINDS
} kind;


struct A {
};

struct B {
  B(A&);               // line 47
};

struct C : A {
};

// __getImplicitConversion(
//   <expression with source type>,
//   <expression with destination type>,
//   <implicit conversion kind>,
//   <standard conversion 1>,
//   <line number of user-defined conversion, or 0 if none>,
//   <standard conversion 2, or 0 if none>
// )


void f()
{
  // elementary conversions
  __getImplicitConversion((int)0, (int)0,
                          IC_STANDARD, SC_IDENTITY, 0, 0);
  __getImplicitConversion((int)0, (int &)0,
                          IC_STANDARD, SC_IDENTITY,0,0);

  // constructor conversions
  A a;
  B b;
  C c;
  __getImplicitConversion((A)a, (B)b,
                          IC_USER_DEFINED, SC_IDENTITY, 47, SC_IDENTITY);
  __getImplicitConversion((A&)a, (B)b,
                          IC_USER_DEFINED, SC_IDENTITY, 47, SC_IDENTITY);
  __getImplicitConversion((C&)c, (B)b,
                          IC_USER_DEFINED, SC_PTR_CONV, 47, SC_IDENTITY);


}
