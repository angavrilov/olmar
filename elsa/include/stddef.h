// stddef.h
// cppstd section 18.1

#ifndef __STDDEF_H
#define __STDDEF_H

// not "(void*)0", "__null", or any other junk.  NULL is 0, in my book.
#undef NULL
#define NULL 0

#undef offsetof
#define offsetof(objtype, field) (&(((objtype*)0)->field))

typedef signed int ptrdiff_t;

typedef signed int size_t;

#endif // __STDDEF_H
