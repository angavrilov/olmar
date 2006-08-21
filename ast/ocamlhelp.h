// ocamlhelp.h            see license.txt for copyright and terms of use
// ocaml serialization helpers -- included by generated ast code

#ifndef OCAMLHELP_H
#define OCAMLHELP_H


#define CAML_NAME_SPACE     // want only to see caml_... identifiers
extern "C" {
#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/memory.h>
#include <caml/alloc.h>
};
#include "strtable.h"
#include "srcloc.h"
#include "thashtbl.h"       // THashTbl
#include "sobjset.h"        // SObjSet


// returns true on the values that we expect back from ocaml:
// namly on plain structured data blocks
#define IS_OCAML_AST_VALUE(x) (Is_long(x) || VALUE_TAG(Tag_hd(Hd_val(x))))

#define VALUE_TAG(x) ((x) < No_scan_tag && (x) != Forward_tag && \
		      (x) != Infix_tag  && (x) != Object_tag && \
		      (x) != Closure_tag && (x) != Lazy_tag)

// returns true on int32 objects (among other custom blocks)
#define IS_OCAML_INT32(x) (Is_block(x) && Tag_hd(Hd_val(x)) == Custom_tag)
#define IS_OCAML_STRING(x) (Is_block(x) && Tag_hd(Hd_val(x)) == String_tag)

// code snipped to examine block size & tags
// cerr << hex << val_loc << dec
//      << " size " << Wosize_val(val_loc)
//      << " tag " << Tag_hd(Hd_val(val_loc)) 
//      << endl << flush;


// -------------------------- ocaml helpers -----------------------


// callback for List.rev
value ocaml_list_rev(value l);

// The class ToOcamlData is not used in any way here. It only 
// appears here because all the serialization functions have a 
// standardized interface with a ToOcamlData pointer as second 
// argument. 
// Every user of this code must define the class ToOcamlData.
class ToOcamlData;

const value Val_None = Val_int(0);

// hand written ocaml serialization function
value option_some_constr(value v);

// hand written ocaml serialization function
inline
value ocaml_from_cstring(const char * s, ToOcamlData *d){
  xassert(s);
  return(caml_copy_string(s));
}


// hand written ocaml serialization function
inline
value ocaml_from_bool(const bool &b, ToOcamlData *d){
  return(Val_bool(b));
}

// hand written ocaml serialization function
inline
value ocaml_from_int(const int &i, ToOcamlData *d){
  // don't allocate
  xassert(i <= Max_long && Min_long <= i);
  return(Val_int(i));
}


// hand written ocaml serialization function
inline
value ocaml_from_StringRef(const StringRef &s, ToOcamlData *d){
  // StringRef is const char *
  return(ocaml_from_cstring(s, d));
}

// hand written ocaml serialization function
inline
value ocaml_from_string(const string &s, ToOcamlData *d){
  return(ocaml_from_cstring(s.c_str(), d));
}


//*********************** ocaml_val cleanup **********************************
// all these functions are empty, 
// however, defining them here empty is better than another hack in astgen
// hand written ocaml serialization cleanup
inline void detach_ocaml_cstring(const char &) {}
inline void detach_ocaml_bool(const bool &) {}
inline void detach_ocaml_int(const int &) {}
inline void detach_ocaml_StringRef(const StringRef &) {}
inline void detach_ocaml_string(const string &) {}

#endif // OCAMLHELP_H

