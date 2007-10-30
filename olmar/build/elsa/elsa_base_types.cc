//  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *
//  See file license.txt for terms of use                              *
//**********************************************************************

// ocaml reflection for base types in the elsa ast

#include "elsa_base_types.h"
#include "cc_type.h"

value ocaml_reflect_SourceLoc(const SourceLoc * loc){
  CAMLparam0();
  CAMLlocal3(val_s, val_loc, result);
  
  char const *name;
  int line, col;

  static value * source_loc_hash = NULL;
  static value * source_loc_hash_find_closure = NULL;
  static value * source_loc_hash_add_closure = NULL;
  static value * not_found_id = NULL;

  if(!source_loc_hash)
    source_loc_hash = caml_named_value("source_loc_hash");
  xassert(source_loc_hash);
  if(!source_loc_hash_find_closure)
    source_loc_hash_find_closure = caml_named_value("source_loc_hash_find");
  xassert(source_loc_hash_find_closure);
  if(!source_loc_hash_add_closure)
    source_loc_hash_add_closure = caml_named_value("source_loc_hash_add");
  xassert(source_loc_hash_add_closure);
  if(!not_found_id)
    not_found_id = caml_named_value("not_found_exception_id");
  xassert(source_loc_hash
	  && source_loc_hash_find_closure 
	  && source_loc_hash_add_closure
	  && not_found_id);

  // cerr << "sourceloc " << loc << endl << flush;
  val_loc = caml_copy_nativeint(*loc);
  xassert(IS_OCAML_INT32(val_loc));

  result = caml_callback2_exn(*source_loc_hash_find_closure, 
			      *source_loc_hash, val_loc);
  xassert(IS_OCAML_AST_VALUE(result));

  bool result_is_exception = Is_exception_result(result);
  result = Extract_exception(result);

  if(result_is_exception){
    // cerr << "got exception ";
    if(Field(result, 0) != *not_found_id) {
      cerr << "Unexpected ocaml exception in ocaml_reflect_SourceLoc\n";
      xassert(false);
      CAMLnoreturn;
    }
    // had a not found exception
    sourceLocManager->decodeLineCol(*loc, name, line, col);

    val_s = caml_copy_string(name);
    xassert(IS_OCAML_STRING(val_s));

    result = caml_alloc_tuple(3);
    Store_field(result, 0, val_s);
    Store_field(result, 1, ocaml_reflect_int(&line));
    Store_field(result, 2, ocaml_reflect_int(&col));
    result = caml_callback3(*source_loc_hash_add_closure, *source_loc_hash,
			    val_loc, result);
    xassert(IS_OCAML_AST_VALUE(result));
  }

  CAMLreturn(result);
}



// hand written ocaml serialization function
value ocaml_reflect_Array_size(Array_size * size){
  // we do allocate here
  CAMLparam0();
  CAMLlocal1(result);

  static value * create_array_size_NO_SIZE_constructor_closure = NULL;
  static value * create_array_size_DYN_SIZE_constructor_closure = NULL;
  static value * create_array_size_FIXED_SIZE_constructor_closure = NULL;

  switch(*size){

  case ArrayType::NO_SIZE:
    if(create_array_size_NO_SIZE_constructor_closure == NULL)
      create_array_size_NO_SIZE_constructor_closure = 
        caml_named_value("create_array_size_NO_SIZE_constructor");
    xassert(create_array_size_NO_SIZE_constructor_closure);
    result = caml_callback(*create_array_size_NO_SIZE_constructor_closure,
			       Val_unit);
    break;
  case ArrayType::DYN_SIZE:
    if(create_array_size_DYN_SIZE_constructor_closure == NULL)
      create_array_size_DYN_SIZE_constructor_closure = 
        caml_named_value("create_array_size_DYN_SIZE_constructor");
    xassert(create_array_size_DYN_SIZE_constructor_closure);
    result = caml_callback(*create_array_size_DYN_SIZE_constructor_closure,
			       Val_unit);
    break;
  default:
    if(create_array_size_FIXED_SIZE_constructor_closure == NULL)
      create_array_size_FIXED_SIZE_constructor_closure = 
        caml_named_value("create_array_size_FIXED_SIZE_constructor");
    xassert(create_array_size_FIXED_SIZE_constructor_closure);
    result = ocaml_reflect_int(size);
    result = 
      caml_callback(*create_array_size_FIXED_SIZE_constructor_closure,
		     result);
    break;
  }
  xassert(IS_OCAML_AST_VALUE(result));

  CAMLreturn(result);
}
