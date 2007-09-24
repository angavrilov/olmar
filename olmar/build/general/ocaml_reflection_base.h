//  Copyright 2006 Hendrik Tews, All rights reserved.                  *
//  See file license.txt for terms of use                              *
//**********************************************************************

// general ocaml reflection type and utility functions
// included in every ocaml reflection

#ifndef OCAML_REFLECTION_BASE
#define OCAML_REFLECTION_BASE


#define CAML_NAME_SPACE     // want only to see caml_... identifiers
extern "C" {
#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/memory.h>
#include <caml/alloc.h>
};
#include "sobjset.h"        // SObjSet
#include "xassert.h"	    // xassert
#include "str.h"	    // string


// --------------------- ocaml value test macros  ---------------------------

// returns true on the values that we expect back from ocaml:
// namly on plain structured data blocks
#define IS_OCAML_AST_VALUE(x) (Is_long(x) || VALUE_TAG(Tag_hd(Hd_val(x))))

#define VALUE_TAG(x) ((x) < No_scan_tag && (x) != Forward_tag && \
		      (x) != Infix_tag  && (x) != Object_tag && \
		      (x) != Closure_tag && (x) != Lazy_tag)


// --------------------- other ocaml helpers  -------------------------------

const value Val_None = Val_int(0);


// --------------------- shared values --------------------------------------

value find_ocaml_shared_node_value(void const *);
value find_ocaml_shared_list_value(void const *);
void register_ocaml_shared_node_value(void const *, value);
void register_ocaml_shared_list_value(void const *, value);


// --------------------- Ocaml_reflection_data ------------------------------

class Ocaml_reflection_data {
public:
  SObjSet<const void*> stack;		// used to detect cycles in the ast

  Ocaml_reflection_data() : stack() {};

  // destructor checks if the stack is empty
  ~Ocaml_reflection_data();
};

// hand written ocaml serialization function
inline
value ocaml_reflect_cstring(char const * s, Ocaml_reflection_data *d){
  xassert(s);
  return(caml_copy_string(s));
}


// hand written ocaml serialization function
inline
value ocaml_reflect_string(string const * s, Ocaml_reflection_data *d){
  return(ocaml_reflect_cstring(s->c_str(), d));
}


// hand written ocaml serialization function
inline
value ocaml_from_int(const int &i, Ocaml_reflection_data *d){
  // don't allocate
  /* 
   * if(!(i <= Max_long && Min_long <= i))
   *   xassert(false);
   */
  xassert(i <= Max_long && Min_long <= i);
  return(Val_int(i));
}


value ocaml_ast_annotation(const void *, Ocaml_reflection_data *);

#endif // OCAML_REFLECTION_BASE
