// ocamlhelp.cc            see license.txt for copyright and terms of use
// implementation of ocamlhelp.h -- ocaml serialization helpers 


#include "ocamlhelp.h"


// hand written ocaml serialization function
value option_some_constr(value v){
  CAMLparam1(v);
  CAMLlocal1(result);
  result = caml_alloc(1, 0);  // the option cell
  Store_field(result, 0, v);
  CAMLreturn(result);
}
