// t0117.cc
// experiments with __getStandardConversion

// copied from stdconv.h
enum StandardConversion {
  SC_IDENTITY        = 0x000,  // types are identical

  // conversion group 1 (comes first)
  SC_LVAL_TO_RVAL    = 0x001,  // 4.1: int& -> int
  SC_ARRAY_TO_PTR    = 0x002,  // 4.2: char[] -> char*
  SC_FUNC_TO_PTR     = 0x003,  // 4.3: int ()(int) -> int (*)(int)
  SC_GROUP_1_MASK    = 0x00F,

  // conversion group 3 (comes last conceptually)
  SC_QUAL_CONV       = 0x010,  // 4.4: int* -> int const*
  SC_GROUP_3_MASK    = 0x0F0,

  // conversion group 2 (goes in the middle)
  SC_INT_PROM        = 0x200,  // 4.5: int... -> int..., no info loss possible
  SC_FLOAT_PROM      = 0x300,  // 4.6: float -> double, no info loss possible
  SC_INT_CONV        = 0x400,  // 4.7: int... -> int..., info loss possible
  SC_FLOAT_CONV      = 0x500,  // 4.8: float... -> float..., info loss possible
  SC_FLOAT_INT_CONV  = 0x600,  // 4.9: int... <-> float..., info loss possible
  SC_PTR_CONV        = 0x700,  // 4.10: 0 -> Foo*, Child* -> Parent*
  SC_PTR_MEMB_CONV   = 0x800,  // 4.11: int Child::* -> int Parent::*
  SC_BOOL_CONV       = 0x900,  // 4.12: various types <-> bool
  SC_GROUP_2_MASK    = 0xF00,

  SC_ERROR           = 0xFFFF, // cannot convert
};

void f()
{
  __getStandardConversion((int)3, (int)4, SC_IDENTITY);
}
