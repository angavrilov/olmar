//  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *
//  See file license.txt for terms of use                              *
//**********************************************************************

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

value ocaml_from_StringRef(StringRef sr, ToOcamlData *d){
  CAMLparam0();
  CAMLlocal2(result, tmp);

  static value *reg_string_callback = NULL;
  if(reg_string_callback == NULL)
    reg_string_callback = caml_named_value("Reg_StringRef");
  xassert(reg_string_callback);

  xassert(sr);
  tmp = caml_copy_string(sr);
  result = caml_callback2(*reg_string_callback, Val_long(unsigned(sr)>>1), tmp);

  CAMLreturn(result);
}
