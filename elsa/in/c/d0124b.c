// see elsa/in/d0124.cc and elsa/in/gnu/d0124c.cc for contrast;
// without the const, this should fail in ISO C++ but pass in GNU C++
// and in ANSI and GNU C
char *a = "hello";
char const *b = "hello";
