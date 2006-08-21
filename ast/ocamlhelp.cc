// ocamlhelp.cc            see license.txt for copyright and terms of use
// implementation of ocamlhelp.h -- ocaml serialization helpers 


#include "ocamlhelp.h"


// hand written ocaml serialization function
value option_some_constr(value v){
  CAMLparam1(v);
  CAMLlocal1(result);
  result = caml_alloc(1, 0);  // the option cell
  Store_field(result, 0, v);
  xassert(IS_OCAML_AST_VALUE(result));
  CAMLreturn(result);
}


value ocaml_list_rev(value l){
  CAMLparam1(l);
  static value * list_rev_callback = NULL;
  if(list_rev_callback == NULL)
    list_rev_callback = caml_named_value("List.rev");
  xassert(list_rev_callback);
  CAMLreturn(caml_callback(*list_rev_callback, l));
}
