//  Copyright 2006 Hendrik Tews, All rights reserved.                  *
//  See file license.txt for terms of use                              *
//**********************************************************************

// general ocaml reflection type and utility functions

#include "ocaml_reflection_base.h"


// --------------------- Ocaml_reflection_data ------------------------------

Ocaml_reflection_data::~Ocaml_reflection_data() {
  // can't use xassert in a destructor (because C++ cannot raise an
  // exception when unwinding the stack
  // only print informative messages and switch them off 
  // together with xassert
#if !defined(NDEBUG_NO_ASSERTIONS)
  if(stack.size() != 0)
    cerr << "Destructor ~ElsaOcamlData called in a bad state\n";
#endif

}

// --------------------- shared values --------------------------------------

// amount to shift void pointers to fit in an ocaml int
static unsigned const addr_shift = 2;

value find_ocaml_shared_node_value(void const * node) {
  CAMLparam0();
  CAMLlocal2(caml_addr, result);
  int addr = (int) node >> addr_shift;

  static value * get_shared_node_closure = NULL;
  if(get_shared_node_closure == NULL)
    get_shared_node_closure = caml_named_value("get_shared_node");
  xassert(get_shared_node_closure);

  xassert(addr <= Max_long && Min_long <= addr);
  caml_addr = Val_int(addr);
  result = caml_callback(*get_shared_node_closure, caml_addr);
  CAMLreturn(result);
}


value find_ocaml_shared_list_value(void const * list) {
  CAMLparam0();
  CAMLlocal2(caml_addr, result);
  int addr = (int) list >> addr_shift;

  static value * get_shared_list_closure = NULL;
  if(get_shared_list_closure == NULL)
    get_shared_list_closure = caml_named_value("get_shared_list");
  xassert(get_shared_list_closure);

  xassert(addr <= Max_long && Min_long <= addr);
  caml_addr = Val_int(addr);
  result = caml_callback(*get_shared_list_closure, caml_addr);
  CAMLreturn(result);
}


void register_ocaml_shared_node_value(void const * node_addr, value node_val) {
  CAMLparam1(node_val);
  CAMLlocal1(addr_val);
  int addr = (int) node_addr >> addr_shift;
  
  static value * register_shared_node_closure = NULL;
  if(register_shared_node_closure == NULL)
    register_shared_node_closure = caml_named_value("register_shared_node");
  xassert(register_shared_node_closure);

  xassert(addr <= Max_long && Min_long <= addr);
  addr_val = Val_int(addr);
  caml_callback2(*register_shared_node_closure, addr_val, node_val);
  CAMLreturn0;
}


void register_ocaml_shared_list_value(void const * list_addr, value list_val) {
  CAMLparam1(list_val);
  CAMLlocal1(addr_val);
  int addr = (int) list_addr >> addr_shift;
  
  static value * register_shared_list_closure = NULL;
  if(register_shared_list_closure == NULL)
    register_shared_list_closure = caml_named_value("register_shared_list");
  xassert(register_shared_list_closure);

  xassert(addr <= Max_long && Min_long <= addr);
  addr_val = Val_int(addr);
  caml_callback2(*register_shared_list_closure, addr_val, list_val);
  CAMLreturn0;
}


// hand written ocaml serialization function
value ocaml_ast_annotation(const void *thisp, Ocaml_reflection_data *d)
{
  CAMLparam0();
  CAMLlocal1(result);

  static value * create_ast_annotation_closure = NULL;
  if(create_ast_annotation_closure == NULL)
    create_ast_annotation_closure = caml_named_value("create_ast_annotation");
  xassert(create_ast_annotation_closure);

  result = ocaml_from_int(((unsigned) thisp) >> addr_shift, d);
  result = caml_callback(*create_ast_annotation_closure, result);
  xassert(IS_OCAML_AST_VALUE(result));

  CAMLreturn(result);
}
