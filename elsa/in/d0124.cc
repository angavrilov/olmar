// see elsa/in/gnu/d0124c.cc and elsa/in/c/d0124b.c for contrast;
// without the const, this should fail in ISO C++ but pass in GNU C++
// and in ANSI and GNU C
char
  //ERROR(1):/*
  const
  //ERROR(1):*/
  *a = "hello";
