// my_caml.h
// C interface to create Ocaml data structures
// includes e.g. wrapper functions that have proper
// type information, thereby suppressing some warnings

#ifndef MY_CAML_H
#define MY_CAML_H

// pull in the basic stuff, wrapped in extern "C" so the
// linker will find the correct functions when it's linked
// with the Ocaml runtime
extern "C" {
  #include "caml/mlvalues.h"
  #include "caml/alloc.h"
  #include "caml/memory.h"
}

// copy_string that knows its argument isn't modified
inline value my_copy_string(char const *str)
  { return copy_string(const_cast<char*>(str)); }



// versions of the CAMLxparam<n> macros which don't
// yield so many "unused variable" warnings (at least
// under gcc); these are copied from /usr/lib/ocaml/caml/memory.h
// from Ocaml dist version 3.00, and modified from there
#undef CAMLxparam1
#undef CAMLxparam2
#undef CAMLxparam3
#undef CAMLxparam4
#undef CAMLxparam5

#define CAMLxparam1(x) \
  struct caml__roots_block caml__roots_##x; \
  int caml__dummy_##x __attribute__((unused)) = ( \
    (caml__roots_##x.next = local_roots), \
    (local_roots = &caml__roots_##x), \
    (caml__roots_##x.nitems = 1), \
    (caml__roots_##x.ntables = 1), \
    (caml__roots_##x.tables [0] = &x), \
    0)

#define CAMLxparam2(x, y) \
  struct caml__roots_block caml__roots_##x; \
  int caml__dummy_##x __attribute__((unused)) = ( \
    (caml__roots_##x.next = local_roots), \
    (local_roots = &caml__roots_##x), \
    (caml__roots_##x.nitems = 1), \
    (caml__roots_##x.ntables = 2), \
    (caml__roots_##x.tables [0] = &x), \
    (caml__roots_##x.tables [1] = &y), \
    0)

#define CAMLxparam3(x, y, z) \
  struct caml__roots_block caml__roots_##x; \
  int caml__dummy_##x __attribute__((unused)) = ( \
    (caml__roots_##x.next = local_roots), \
    (local_roots = &caml__roots_##x), \
    (caml__roots_##x.nitems = 1), \
    (caml__roots_##x.ntables = 3), \
    (caml__roots_##x.tables [0] = &x), \
    (caml__roots_##x.tables [1] = &y), \
    (caml__roots_##x.tables [2] = &z), \
    0)

#define CAMLxparam4(x, y, z, t) \
  struct caml__roots_block caml__roots_##x; \
  int caml__dummy_##x __attribute__((unused)) = ( \
    (caml__roots_##x.next = local_roots), \
    (local_roots = &caml__roots_##x), \
    (caml__roots_##x.nitems = 1), \
    (caml__roots_##x.ntables = 4), \
    (caml__roots_##x.tables [0] = &x), \
    (caml__roots_##x.tables [1] = &y), \
    (caml__roots_##x.tables [2] = &z), \
    (caml__roots_##x.tables [3] = &t), \
    0)

#define CAMLxparam5(x, y, z, t, u) \
  struct caml__roots_block caml__roots_##x; \
  int caml__dummy_##x __attribute__((unused)) = ( \
    (caml__roots_##x.next = local_roots), \
    (local_roots = &caml__roots_##x), \
    (caml__roots_##x.nitems = 1), \
    (caml__roots_##x.ntables = 5), \
    (caml__roots_##x.tables [0] = &x), \
    (caml__roots_##x.tables [1] = &y), \
    (caml__roots_##x.tables [2] = &z), \
    (caml__roots_##x.tables [3] = &t), \
    (caml__roots_##x.tables [4] = &u), \
    0)



#endif // MY_CAML_H
