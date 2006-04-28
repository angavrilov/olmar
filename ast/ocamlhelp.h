// ocamlhelp.h            see license.txt for copyright and terms of use
// ocaml serialization helpers -- included by generated ast code

#ifndef OCAMLHELP_H
#define OCAMLHELP_H


#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/memory.h>
#include <caml/alloc.h>
#include "strtable.h"
#include "srcloc.h"
#include "thashtbl.h"       // THashTbl
#include "sobjset.h"        // SObjSet

// -------------------------- ocaml helpers -----------------------

// take this only if we really want the ocaml interface
// _and_ have defined the value type

class ToOcamlData {
public:
  SObjSet<const void*> stack;		// used to detect cycles in the ast
};

// ocaml serialization for base types
value ocaml_from_SourceLoc(const SourceLoc &, ToOcamlData *);


// hand written ocaml serialization function
inline
value ocaml_from_cstring(const char * s, ToOcamlData *d){
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


#endif // OCAMLHELP_H

