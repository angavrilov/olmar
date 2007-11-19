//  Copyright 2006 Hendrik Tews, All rights reserved.                  *
//  See file license.txt for terms of use                              *
//**********************************************************************

// general ocaml reflection type and utility functions

#include "ocaml_reflection_base.h"


//***************************************************************************
//***************************************************************************
//*********              shared values                        ***************
//***************************************************************************
//***************************************************************************


value find_ocaml_shared_value(void const * node, int typ) {
  CAMLparam0();
  CAMLlocal3(caml_addr, caml_type, result);

  static value * get_shared_node_closure = NULL;
  if(get_shared_node_closure == NULL)
    get_shared_node_closure = caml_named_value("get_shared_node");
  xassert(get_shared_node_closure);

  caml_addr = caml_copy_nativeint((int) node);
  xassert(IS_OCAML_INT32(caml_addr));

  xassert(typ <= Max_long && Min_long <= typ);
  caml_type = Val_int(typ);
  result = caml_callback2(*get_shared_node_closure, caml_addr, caml_type);
  CAMLreturn(result);
}


void register_ocaml_shared_value(void const * node_addr, 
				      value node_val, int typ) {
  CAMLparam1(node_val);
  CAMLlocal2(addr_val, caml_type);
  
  static value * register_shared_node_closure = NULL;
  if(register_shared_node_closure == NULL)
    register_shared_node_closure = caml_named_value("register_shared_node");
  xassert(register_shared_node_closure);

  addr_val = caml_copy_nativeint((int) node_addr);
  xassert(IS_OCAML_INT32(addr_val));

  xassert(typ <= Max_long && Min_long <= typ);
  caml_type = Val_int(typ);
  caml_callback3(*register_shared_node_closure, addr_val, caml_type, node_val);
  CAMLreturn0;
}


//***************************************************************************
//***************************************************************************
//*********  hand written serialization/reflection functions  ***************
//***************************************************************************
//***************************************************************************


// hand written ocaml serialization function
value ocaml_reflect_cstring(char const * s){
  CAMLparam0();
  CAMLlocal1(res);

  xassert(s);

  res = find_ocaml_shared_value(s, -1);
  if(res != Val_None) {
    xassert(Is_block(res) && Tag_val(res) == 0 && Wosize_val(res) == 1);
    CAMLreturn(Field(res, 0));
  }

  res = caml_copy_string(s);
  register_ocaml_shared_value(s, res, -1);
  
  CAMLreturn(res);
}




// hand written ocaml serialization function
value ocaml_ast_annotation(const void *thisp)
{
  CAMLparam0();
  CAMLlocal1(result);

  static value * create_ast_annotation_closure = NULL;
  if(create_ast_annotation_closure == NULL)
    create_ast_annotation_closure = caml_named_value("create_ast_annotation");
  xassert(create_ast_annotation_closure);

  result = caml_copy_nativeint((int) thisp);
  xassert(IS_OCAML_INT32(result));
  result = caml_callback(*create_ast_annotation_closure, result);
  xassert(IS_OCAML_AST_VALUE(result));

  CAMLreturn(result);
}


// hand written ocaml serialization function
value make_reference(value elem) {
  CAMLparam1(elem);
  CAMLlocal1(result);
  result = caml_alloc_small(1, 0); // the reference cell
  Field(result, 0) = elem;
  CAMLreturn(result);
}


