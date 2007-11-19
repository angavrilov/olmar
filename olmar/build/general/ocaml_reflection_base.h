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


//***************************************************************************
//***************************************************************************
//*********         ocaml value test macros                   ***************
//***************************************************************************
//***************************************************************************


// returns true on the values that we expect back from ocaml:
// namly on plain structured data blocks
#define IS_OCAML_AST_VALUE(x) (Is_long(x) || VALUE_TAG(Tag_hd(Hd_val(x))))

#define VALUE_TAG(x) ((x) < No_scan_tag && (x) != Forward_tag && \
		      (x) != Infix_tag  && (x) != Object_tag && \
		      (x) != Closure_tag && (x) != Lazy_tag)

// returns true on int32 objects (among other custom blocks)
#define IS_OCAML_INT32(x) (Is_block(x) && Tag_hd(Hd_val(x)) == Custom_tag)
// returns true on string objects
#define IS_OCAML_STRING(x) (Is_block(x) && Tag_hd(Hd_val(x)) == String_tag)
// returns true on float objects
#define IS_OCAML_FLOAT(x) (Is_block(x) && Tag_hd(Hd_val(x)) == Double_tag)


//***************************************************************************
//***************************************************************************
//*********                  misc stuff                       ***************
//***************************************************************************
//***************************************************************************


value const Val_None = Val_int(0);
int const Tag_some = 0;


//***************************************************************************
//***************************************************************************
//*********              shared values                        ***************
//***************************************************************************
//***************************************************************************


value find_ocaml_shared_value(void const *, int);
void register_ocaml_shared_value(void const *, value, int);


//***************************************************************************
//***************************************************************************
//*********  hand written serialization/reflection functions  ***************
//***************************************************************************
//***************************************************************************


value ocaml_reflect_cstring(char const * s);


// hand written ocaml serialization function
inline
value ocaml_reflect_string(string const * s){
  return(ocaml_reflect_cstring(s->c_str()));
}


// hand written ocaml serialization function
inline
value ocaml_reflect_int(const int *i){
  // don't allocate
  // if(!(*i <= Max_long && Min_long <= *i))
  //   xassert(false);
  xassert(*i <= Max_long && Min_long <= *i);
  return(Val_int(*i));
}

// hand written ocaml serialization function
value ocaml_ast_annotation(const void *);
value make_reference(value elem);


#endif // OCAML_REFLECTION_BASE
