// ocamlhelp.h            see license.txt for copyright and terms of use
// ocaml serialization helpers -- included by generated ast code

#ifndef OCAMLHELP_H
#define OCAMLHELP_H


#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/memory.h>
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
value ocaml_from_int(const int, ToOcamlData *);
value ocaml_from_StringRef(const StringRef &, ToOcamlData *);
value ocaml_from_string(const string &, ToOcamlData *);
value ocaml_from_bool(const bool &, ToOcamlData *);
value ocaml_from_SourceLoc(const SourceLoc &, ToOcamlData *);

value create_builtin_cons_constructor(value, value);


#endif // OCAMLHELP_H

