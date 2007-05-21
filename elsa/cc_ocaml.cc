//  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *
//  See file license.txt for terms of use                              *
//**********************************************************************

// implementation of cc_ocaml.h -- ocaml serialization utility functions


#include "cc_ocaml.h"
extern "C" {
#include <caml/fail.h>
}
#include "cc.ast.gen.h"		// Function->toOcaml

// #define DEBUG_CIRCULARITIES

#if defined(DEBUG_CIRCULARITIES) || defined(DEBUG_CAML_GLOBAL_ROOTS)
#include <iomanip.h>
#endif


CircularAstPart::CircularAstPart() : 
  ca_type(CA_Empty), 
  val(Val_unit),
  next(NULL)
{
  caml_register_global_root(&val);
}

CircularAstPart::~CircularAstPart() {
  caml_remove_global_root(&val);
}


ToOcamlData::ToOcamlData()  : 
  stack(), 
  source_loc_hash(Val_unit),
  postponed_count(0),
  postponed_circles(NULL)
{

  xassert(caml_start_up_done);

  caml_register_global_root(&source_loc_hash);

  static value * source_loc_hash_init_closure = NULL;
  if(!source_loc_hash_init_closure)
    source_loc_hash_init_closure = caml_named_value("source_loc_hash_init");
  xassert(source_loc_hash_init_closure);

  source_loc_hash = caml_callback(*source_loc_hash_init_closure, Val_unit);
}


ToOcamlData::~ToOcamlData() {
  // can't use xassert in a destructor (because C++ cannot raise an
  // exception when unwinding the stack
  // only print informative messages and switch them off 
  // together with xassert
#if !defined(NDEBUG_NO_ASSERTIONS)
  if(postponed_circles != NULL ||
     postponed_count != 0 ||
     stack.size() != 0)
    cerr << "Destructor ~ToOcamlData called in a bad state\n";
#endif

  caml_remove_global_root(&source_loc_hash);
}



// hand written ocaml serialization function
value ref_constr(value elem, ToOcamlData * data) {
  CAMLparam1(elem);
  CAMLlocal1(result);
  result = caml_alloc(1, 0); // the reference cell
  Store_field(result, 0, elem);
  xassert(IS_OCAML_AST_VALUE(result));
  CAMLreturn(result);
}


// hand written ocaml serialization function
// not really serialization, but closely related
CircularAstPart * init_ca_part(ToOcamlData * data, value val,
			       CircularAstType type) {

  // no need to register val as long as we don't allocate here
  CircularAstPart * part = new CircularAstPart;
  part->ca_type = type;
  part->val = val;
  part->next = data->postponed_circles;
  data->postponed_circles = part;
  data->postponed_count++;

  return part;
}

// hand written ocaml serialization function
// not relly a serialization function, but handwritten ;-)
void postpone_circular_CType(ToOcamlData * data, value val, CType * type) {

  // no need to register val as long as we don't allocate here
  xassert(type != NULL);
  CircularAstPart * part = init_ca_part(data, val, CA_CType);
  part->ast.type = type;
# ifdef DEBUG_CIRCULARITIES
  cerr << "postpone (" << data->postponed_count
       << ") type " << type << " in cell " 
       << hex << "0x" << val << dec << "\n";
# endif // DEBUG_CIRCULARITIES
}

// hand written ocaml serialization function
// not relly a serialization function, but handwritten ;-)
void postpone_circular_Function(ToOcamlData * data, value val,
				Function * func) {

  // no need to register val as long as we don't allocate here
  xassert(func != NULL);
  CircularAstPart * part = init_ca_part(data, val, CA_Function);
  part->ast.func = func;
# ifdef DEBUG_CIRCULARITIES
  cerr << "postpone (" << data->postponed_count
       << ") Function " << func << " in cell " 
       << hex << "0x" << val << dec << "\n";
# endif // DEBUG_CIRCULARITIES
}


// hand written ocaml serialization function
// not relly a serialization function, but handwritten ;-)
void postpone_circular_Expression(ToOcamlData * data, value val,
				  Expression * ex) {

  // no need to register val as long as we don't allocate here
  xassert(ex != NULL);
  CircularAstPart * part = init_ca_part(data, val, CA_Expression);
  part->ast.expression = ex;
# ifdef DEBUG_CIRCULARITIES
  cerr << "postpone (" << data->postponed_count
       << ") Expression " << ex << " in cell " 
       << hex << "0x" << val << dec << "\n";
# endif // DEBUG_CIRCULARITIES
}


// hand written ocaml serialization function
// not relly a serialization function, but handwritten ;-)
void postpone_circular_TypeSpecifier(ToOcamlData * data, value val,
				     TypeSpecifier * ts) {
  // no need to register val as long as we don't allocate here
  xassert(ts != NULL);
  CircularAstPart * part = init_ca_part(data, val, CA_TypeSpecifier);
  part->ast.typeSpecifier = ts;
# ifdef DEBUG_CIRCULARITIES
  cerr << "postpone (" << data->postponed_count
       << ") TypeSpecifier " << ts << " in cell " 
       << hex << "0x" << val << dec << "\n";
# endif // DEBUG_CIRCULARITIES
}


// hand written ocaml serialization function
// not relly a serialization function, but handwritten ;-)
void postpone_circular_Variable(ToOcamlData * data, value val, Variable * var) {
  // no need to register val as long as we don't allocate here
  xassert(var != NULL);
  CircularAstPart * part = init_ca_part(data, val, CA_Variable);
  part->ast.variable = var;
# ifdef DEBUG_CIRCULARITIES
  cerr << "postpone (" << data->postponed_count
       << ") Variable " << var << " in cell " 
       << hex << "0x" << val << dec << "\n";
# endif // DEBUG_CIRCULARITIES
}


// hand written ocaml serialization function
// not relly a serialization function, but handwritten ;-)
void postpone_circular_CompoundInfo(ToOcamlData * data, value val, 
				    CompoundType * c) {
  // no need to register val as long as we don't allocate here
  xassert(c != NULL);
  CircularAstPart * part = init_ca_part(data, val, CA_CompoundInfo);
  part->ast.compound = c;
# ifdef DEBUG_CIRCULARITIES
  cerr << "postpone (" << data->postponed_count
       << ") CompoundInfo " << var << " in cell " 
       << hex << "0x" << val << dec << "\n";
# endif // DEBUG_CIRCULARITIES
}


// hand written ocaml serialization function
// not relly a serialization function, but handwritten ;-)
void postpone_circular_OverloadSet(ToOcamlData * data, value val,
				   OverloadSet * os) {

  // no need to register val as long as we don't allocate here
  xassert(os != NULL);
  CircularAstPart * part = init_ca_part(data, val, CA_OverloadSet);
  part->ast.overload = os;
# ifdef DEBUG_CIRCULARITIES
  cerr << "postpone (" << data->postponed_count
       << ") OverloadSet " << os << " in cell " 
       << hex << "0x" << val << dec << "\n";
# endif // DEBUG_CIRCULARITIES
}


// hand written ocaml serialization function
// not relly a serialization function, but handwritten ;-)
void postpone_circular_StringRefMapVariable(ToOcamlData * data, value val,
					    StringRefMap<Variable> * map) {

  // no need to register val as long as we don't allocate here
  xassert(map != NULL);
  CircularAstPart * part = init_ca_part(data, val, CA_StringRefMapVariable);
  part->ast.string_var_map = map;
# ifdef DEBUG_CIRCULARITIES
  cerr << "postpone (" << data->postponed_count
       << ") StringRefMap<Variable> " << map << " in cell " 
       << hex << "0x" << val << dec << "\n";
# endif // DEBUG_CIRCULARITIES
}


// hand written ocaml serialization function
// not relly a serialization function, but handwritten ;-)
void finish_circular_pointers(ToOcamlData * data) {
  CAMLparam0();
  CAMLlocal4(val, cell, key, var);
  CircularAstPart * part;

  while(data->postponed_circles != NULL) {
    part = data->postponed_circles;
    data->postponed_circles = data->postponed_circles->next;
#   ifdef DEBUG_CIRCULARITIES
    cerr << "dispatch (" << data->postponed_count 
    	 << ") cell " << hex << "0x" << part->val << dec;
#   endif // DEBUG_CIRCULARITIES
    data->postponed_count--;

    cell = part->val;
    xassert(data->stack.size() == 0);
  
    switch(part->ca_type) {
    case CA_CType:
#     ifdef DEBUG_CIRCULARITIES
      cerr << " (CType)\n";
#     endif // DEBUG_CIRCULARITIES
      val = part->ast.type->toOcaml(data);
      break;

    case CA_Function:
#     ifdef DEBUG_CIRCULARITIES
      cerr << " (Function)\n";
#     endif // DEBUG_CIRCULARITIES
      val = part->ast.func->toOcaml(data);
      break;

    case CA_Expression:
#     ifdef DEBUG_CIRCULARITIES
      cerr << " (Expression)\n";
#     endif // DEBUG_CIRCULARITIES
      val = part->ast.expression->toOcaml(data);
      break;

    case CA_TypeSpecifier:
#     ifdef DEBUG_CIRCULARITIES
      cerr << " (TypeSpecifier)\n";
#     endif // DEBUG_CIRCULARITIES
      val = part->ast.typeSpecifier->toOcaml(data);
      break;

    case CA_Variable:
#     ifdef DEBUG_CIRCULARITIES
      cerr << " (Variable)\n";
#     endif // DEBUG_CIRCULARITIES
      val = part->ast.variable->toOcaml(data);
      break;

    case CA_CompoundInfo:
#     ifdef DEBUG_CIRCULARITIES
      cerr << " (CompoundInfo)\n";
#     endif // DEBUG_CIRCULARITIES
      val = part->ast.compound->toCompoundInfo(data);
      break;

    case CA_OverloadSet:
#     ifdef DEBUG_CIRCULARITIES
      cerr << " (OverloadSet)\n";
#     endif // DEBUG_CIRCULARITIES
      val = part->ast.overload->toOcaml(data);
      break;

    case CA_StringRefMapVariable:
      { // start a block to make some variables local to this case

#       ifdef DEBUG_CIRCULARITIES
	cerr << " (OverloadSet)\n";
#       endif // DEBUG_CIRCULARITIES

	static value * scope_hashtbl_add_closure = NULL;
	if(scope_hashtbl_add_closure == NULL)
	  scope_hashtbl_add_closure = caml_named_value("scope_hashtbl_add");
	xassert(scope_hashtbl_add_closure);

	// xassert(Is_block(cell));
	// xassert((Tag_val(cell) == 0));
	// xassert((Wosize_val(cell) == 2));
	// xassert(Is_long(Field(cell, 0)));
	// xassert((Long_val(Field(cell, 0)) == 0));
	// xassert(Is_block(Field(cell, 1)));
	// xassert((Tag_val(Field(cell, 1)) == 0));

	xassert(Is_block(cell) &&	// it's a block
		(Tag_val(cell) == 0) && // it's a record
		(Wosize_val(cell) == 2) && // with two fields
		Is_long(Field(cell, 0)) && // field 0 (size) is an int
		(Long_val(Field(cell, 0)) == 0) && // which is zero
		Is_block(Field(cell, 1)) && // field 1 (data) is a block
		(Tag_val(Field(cell, 1)) == 0)); // data is an array

#       if !defined(NDEBUG_NO_ASSERTIONS)
	// HT: XXX the following is probably not 64 bit clean: for i/check_size 
	// I need a type which can hold the maximal ocaml array size
	unsigned check_size = Wosize_val(Field(cell, 1));
	// don't allocate -- no need to register
	value check_array = Field(cell, 1); 
	for(unsigned i = 0; i < check_size; i++) {
	  xassert(Is_long(Field(check_array, i)) && // the array contains Empty
		  (Long_val(Field(check_array, i)) == 0));
	}
#       endif // NDEBUG_NO_ASSERTIONS

	StringRefMap<Variable>::Iter var_iter(*(part->ast.string_var_map));
	while(!var_iter.isDone()) {
	  key = ocaml_from_cstring(var_iter.key(), data);
	  var = var_iter.value()->toOcaml(data);
	  val = caml_callback3(*scope_hashtbl_add_closure, cell, key, var);
	  xassert(val == Val_unit);
	  var_iter.adv();
	}
      }
      break;

    case CA_Empty:
    default:
      xassert(false);
    }

    switch(part->ca_type) {
    case CA_CType:
    case CA_Function:
    case CA_Expression:
    case CA_TypeSpecifier:
    case CA_Variable:
    case CA_CompoundInfo:
      // update an option ref
      // check that cell is ref None
      xassert(Is_block(cell) && 	      // it's a block
	      (Tag_val(cell) == 0) &&         // it's an array/tuple/record
	      (Wosize_val(cell) == 1) &&      // with one cell
	      Is_long(Field(cell, 0)) &&      // containing None
	      (Long_val(Field(cell, 0)) == 0));

      // construct Some val
      val = option_some_constr(val);
      // assign the reference cell
      Store_field(cell, 0, val);
      break;

    case CA_OverloadSet:
      // update an list ref
      // check that cell is ref []
      xassert(Is_block(cell) && 	          // it's a block
	      (Tag_val(cell) == 0) &&             // it's an array/tuple/record
	      (Wosize_val(cell) == 1) &&          // with one cell
	      (Field(cell, 0) == Val_emptylist)); // containing []

      // assign the reference cell
      Store_field(cell, 0, val);
      break;

    case CA_StringRefMapVariable:
      // checked already above that cell is an empty hashtable
      break;
	      
    default:
      xassert(false);
    }

    delete(part);
  }

  xassert(data->postponed_count == 0);
  CAMLreturn0;
}





// hand written ocaml serialization function
value ocaml_from_SourceLoc(const SourceLoc &loc, ToOcamlData *d){
  CAMLparam0();
  CAMLlocal3(val_s, val_loc, result);
  
  char const *name;
  int line, col;

  static value * source_loc_hash_find_closure = NULL;
  static value * source_loc_hash_add_closure = NULL;
  static value * not_found_id = NULL;

  if(!source_loc_hash_find_closure)
    source_loc_hash_find_closure = caml_named_value("source_loc_hash_find");
  if(!source_loc_hash_add_closure)
    source_loc_hash_add_closure = caml_named_value("source_loc_hash_add");
  if(!not_found_id)
    not_found_id = caml_named_value("not_found_exception_id");
  xassert(source_loc_hash_find_closure && source_loc_hash_add_closure
	  && not_found_id);

  // cerr << "sourceloc " << loc << endl << flush;
  val_loc = caml_copy_nativeint(loc);
  xassert(IS_OCAML_INT32(val_loc));

  result = caml_callback2_exn(*source_loc_hash_find_closure, 
			      d->source_loc_hash, val_loc);
  xassert(IS_OCAML_AST_VALUE(result));

  bool result_is_exception = Is_exception_result(result);
  result = Extract_exception(result);

  if(result_is_exception){
    // cerr << "got exception ";
    if(Field(result, 0) != *not_found_id) {

/*
#define printhd(val) "[size " << dec << (Field((val), -1) >> 10) << \
                     " color " << ((Field((val), -1) >> 8) & 0x3) << \
                     " tag " << (Field((val), -1) & 0xff) << hex << "]"
*/                   
//       cerr << hex 
// 	   << result << " "
// 	   << printhd(result) << "|"
// 	   << Field(result, 0) << "\n" 
// 	   << printhd(Field(result, 0)) << "|"
// 	   << Field(Field(result, 0), 0) << "\n"
// 	   << printhd(Field(Field(result, 0), 0)) << "|"
// 	   << "\"" << (char *) Field(Field(result, 0), 0)
// 	   << "\""
// 	   << dec
// 	   << endl;
      // cerr << "... reraising\n" << endl << flush;

      caml_raise(result);
      CAMLnoreturn;
    }
    // cerr << "Not_found\n" << endl << flush;

    // had a not found exception
    sourceLocManager->decodeLineCol(loc, name, line, col);

    val_s = caml_copy_string(name);
    xassert(IS_OCAML_STRING(val_s));

    result = caml_alloc_tuple(3);
    Store_field(result, 0, val_s);
    Store_field(result, 1, Val_int(line));
    Store_field(result, 2, Val_int(col));
    result = caml_callback3(*source_loc_hash_add_closure, d->source_loc_hash,
			    val_loc, result);
    xassert(IS_OCAML_AST_VALUE(result));
  }
  // else {
  //   cerr << "val found" << endl << flush;
  // }

  CAMLreturn(result);
}


// hand written ocaml serialization function
value ocaml_ast_annotation(const void *thisp, ToOcamlData *d)
{
  CAMLparam0();
  CAMLlocal1(result);

  static value * create_ast_annotation_closure = NULL;
  if(create_ast_annotation_closure == NULL)
    create_ast_annotation_closure = caml_named_value("create_ast_annotation");
  xassert(create_ast_annotation_closure);

  result = ocaml_from_int(((unsigned) thisp) >> 2, d);
  result = caml_callback(*create_ast_annotation_closure, result);
  xassert(IS_OCAML_AST_VALUE(result));

  CAMLreturn(result);
}


int get_max_annotation()
{
  // don't allocate in here, but play save
  CAMLparam0();
  CAMLlocal1(max);

  static value * ocaml_max_annotation = NULL;
  if(ocaml_max_annotation == NULL)
    ocaml_max_annotation = caml_named_value("ocaml_max_annotation");
  xassert(ocaml_max_annotation);

  max = caml_callback(*ocaml_max_annotation, Val_unit);
  xassert(Is_long(max));
  CAMLreturn(Int_val(max));
}


// hand written ocaml serialization function
value ocaml_from_DeclFlags(const DeclFlags &f, ToOcamlData *d){
  CAMLparam0();
  CAMLlocal2(camlf, result);
  // cout << "DeclFlags start marshal\n" << flush;

  static value * declFlag_from_int32_closure = NULL;
  if(declFlag_from_int32_closure == NULL)
    declFlag_from_int32_closure = caml_named_value("declFlag_from_int32");
  xassert(declFlag_from_int32_closure);

  camlf = caml_copy_int32(f);
  xassert(IS_OCAML_INT32(camlf));
  result = caml_callback(*declFlag_from_int32_closure, camlf);
  xassert(IS_OCAML_AST_VALUE(result));

  // cout << "DeclFlags end marshal\n" << flush;
  CAMLreturn(result);
}


// hand written ocaml serialization function
value ocaml_from_SimpleTypeId(const SimpleTypeId &id, ToOcamlData *d){
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_ST_CHAR_constructor_closure = NULL;
  static value * create_ST_UNSIGNED_CHAR_constructor_closure = NULL;
  static value * create_ST_SIGNED_CHAR_constructor_closure = NULL;
  static value * create_ST_BOOL_constructor_closure = NULL;
  static value * create_ST_INT_constructor_closure = NULL;
  static value * create_ST_UNSIGNED_INT_constructor_closure = NULL;
  static value * create_ST_LONG_INT_constructor_closure = NULL;
  static value * create_ST_UNSIGNED_LONG_INT_constructor_closure = NULL;
  static value * create_ST_LONG_LONG_constructor_closure = NULL;
  static value * create_ST_UNSIGNED_LONG_LONG_constructor_closure = NULL;
  static value * create_ST_SHORT_INT_constructor_closure = NULL;
  static value * create_ST_UNSIGNED_SHORT_INT_constructor_closure = NULL;
  static value * create_ST_WCHAR_T_constructor_closure = NULL;
  static value * create_ST_FLOAT_constructor_closure = NULL;
  static value * create_ST_DOUBLE_constructor_closure = NULL;
  static value * create_ST_LONG_DOUBLE_constructor_closure = NULL;
  static value * create_ST_FLOAT_COMPLEX_constructor_closure = NULL;
  static value * create_ST_DOUBLE_COMPLEX_constructor_closure = NULL;
  static value * create_ST_LONG_DOUBLE_COMPLEX_constructor_closure = NULL;
  static value * create_ST_FLOAT_IMAGINARY_constructor_closure = NULL;
  static value * create_ST_DOUBLE_IMAGINARY_constructor_closure = NULL;
  static value * create_ST_LONG_DOUBLE_IMAGINARY_constructor_closure = NULL;
  static value * create_ST_VOID_constructor_closure = NULL;
  static value * create_ST_ELLIPSIS_constructor_closure = NULL;
  static value * create_ST_CDTOR_constructor_closure = NULL;
  static value * create_ST_ERROR_constructor_closure = NULL;
  static value * create_ST_DEPENDENT_constructor_closure = NULL;
  static value * create_ST_IMPLINT_constructor_closure = NULL;
  static value * create_ST_NOTFOUND_constructor_closure = NULL;
  static value * create_ST_PROMOTED_INTEGRAL_constructor_closure = NULL;
  static value * create_ST_PROMOTED_ARITHMETIC_constructor_closure = NULL;
  static value * create_ST_INTEGRAL_constructor_closure = NULL;
  static value * create_ST_ARITHMETIC_constructor_closure = NULL;
  static value * create_ST_ARITHMETIC_NON_BOOL_constructor_closure = NULL;
  static value * create_ST_ANY_OBJ_TYPE_constructor_closure = NULL;
  static value * create_ST_ANY_NON_VOID_constructor_closure = NULL;
  static value * create_ST_ANY_TYPE_constructor_closure = NULL;
  static value * create_ST_PRET_STRIP_REF_constructor_closure = NULL;
  static value * create_ST_PRET_PTM_constructor_closure = NULL;
  static value * create_ST_PRET_ARITH_CONV_constructor_closure = NULL;
  static value * create_ST_PRET_FIRST_constructor_closure = NULL;
  static value * create_ST_PRET_FIRST_PTR2REF_constructor_closure = NULL;
  static value * create_ST_PRET_SECOND_constructor_closure = NULL;
  static value * create_ST_PRET_SECOND_PTR2REF_constructor_closure = NULL;

  value result;

  switch(id){

  case ST_CHAR:
    if(create_ST_CHAR_constructor_closure == NULL)
      create_ST_CHAR_constructor_closure = 
        caml_named_value("create_ST_CHAR_constructor");
    xassert(create_ST_CHAR_constructor_closure);
    result = caml_callback(*create_ST_CHAR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_UNSIGNED_CHAR:
    if(create_ST_UNSIGNED_CHAR_constructor_closure == NULL)
      create_ST_UNSIGNED_CHAR_constructor_closure = 
        caml_named_value("create_ST_UNSIGNED_CHAR_constructor");
    xassert(create_ST_UNSIGNED_CHAR_constructor_closure);
    result = caml_callback(*create_ST_UNSIGNED_CHAR_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_SIGNED_CHAR:
    if(create_ST_SIGNED_CHAR_constructor_closure == NULL)
      create_ST_SIGNED_CHAR_constructor_closure = 
        caml_named_value("create_ST_SIGNED_CHAR_constructor");
    xassert(create_ST_SIGNED_CHAR_constructor_closure);
    result = caml_callback(*create_ST_SIGNED_CHAR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_BOOL:
    if(create_ST_BOOL_constructor_closure == NULL)
      create_ST_BOOL_constructor_closure = 
        caml_named_value("create_ST_BOOL_constructor");
    xassert(create_ST_BOOL_constructor_closure);
    result = caml_callback(*create_ST_BOOL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_INT:
    if(create_ST_INT_constructor_closure == NULL)
      create_ST_INT_constructor_closure = 
        caml_named_value("create_ST_INT_constructor");
    xassert(create_ST_INT_constructor_closure);
    result = caml_callback(*create_ST_INT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_UNSIGNED_INT:
    if(create_ST_UNSIGNED_INT_constructor_closure == NULL)
      create_ST_UNSIGNED_INT_constructor_closure = 
        caml_named_value("create_ST_UNSIGNED_INT_constructor");
    xassert(create_ST_UNSIGNED_INT_constructor_closure);
    result = caml_callback(*create_ST_UNSIGNED_INT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_LONG_INT:
    if(create_ST_LONG_INT_constructor_closure == NULL)
      create_ST_LONG_INT_constructor_closure = 
        caml_named_value("create_ST_LONG_INT_constructor");
    xassert(create_ST_LONG_INT_constructor_closure);
    result = caml_callback(*create_ST_LONG_INT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_UNSIGNED_LONG_INT:
    if(create_ST_UNSIGNED_LONG_INT_constructor_closure == NULL)
      create_ST_UNSIGNED_LONG_INT_constructor_closure = 
        caml_named_value("create_ST_UNSIGNED_LONG_INT_constructor");
    xassert(create_ST_UNSIGNED_LONG_INT_constructor_closure);
    result = caml_callback(*create_ST_UNSIGNED_LONG_INT_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_LONG_LONG:
    if(create_ST_LONG_LONG_constructor_closure == NULL)
      create_ST_LONG_LONG_constructor_closure = 
        caml_named_value("create_ST_LONG_LONG_constructor");
    xassert(create_ST_LONG_LONG_constructor_closure);
    result = caml_callback(*create_ST_LONG_LONG_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_UNSIGNED_LONG_LONG:
    if(create_ST_UNSIGNED_LONG_LONG_constructor_closure == NULL)
      create_ST_UNSIGNED_LONG_LONG_constructor_closure = 
        caml_named_value("create_ST_UNSIGNED_LONG_LONG_constructor");
    xassert(create_ST_UNSIGNED_LONG_LONG_constructor_closure);
    result = caml_callback(*create_ST_UNSIGNED_LONG_LONG_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_SHORT_INT:
    if(create_ST_SHORT_INT_constructor_closure == NULL)
      create_ST_SHORT_INT_constructor_closure = 
        caml_named_value("create_ST_SHORT_INT_constructor");
    xassert(create_ST_SHORT_INT_constructor_closure);
    result = caml_callback(*create_ST_SHORT_INT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_UNSIGNED_SHORT_INT:
    if(create_ST_UNSIGNED_SHORT_INT_constructor_closure == NULL)
      create_ST_UNSIGNED_SHORT_INT_constructor_closure = 
        caml_named_value("create_ST_UNSIGNED_SHORT_INT_constructor");
    xassert(create_ST_UNSIGNED_SHORT_INT_constructor_closure);
    result = caml_callback(*create_ST_UNSIGNED_SHORT_INT_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_WCHAR_T:
    if(create_ST_WCHAR_T_constructor_closure == NULL)
      create_ST_WCHAR_T_constructor_closure = 
        caml_named_value("create_ST_WCHAR_T_constructor");
    xassert(create_ST_WCHAR_T_constructor_closure);
    result = caml_callback(*create_ST_WCHAR_T_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_FLOAT:
    if(create_ST_FLOAT_constructor_closure == NULL)
      create_ST_FLOAT_constructor_closure = 
        caml_named_value("create_ST_FLOAT_constructor");
    xassert(create_ST_FLOAT_constructor_closure);
    result = caml_callback(*create_ST_FLOAT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_DOUBLE:
    if(create_ST_DOUBLE_constructor_closure == NULL)
      create_ST_DOUBLE_constructor_closure = 
        caml_named_value("create_ST_DOUBLE_constructor");
    xassert(create_ST_DOUBLE_constructor_closure);
    result = caml_callback(*create_ST_DOUBLE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_LONG_DOUBLE:
    if(create_ST_LONG_DOUBLE_constructor_closure == NULL)
      create_ST_LONG_DOUBLE_constructor_closure = 
        caml_named_value("create_ST_LONG_DOUBLE_constructor");
    xassert(create_ST_LONG_DOUBLE_constructor_closure);
    result = caml_callback(*create_ST_LONG_DOUBLE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_FLOAT_COMPLEX:
    if(create_ST_FLOAT_COMPLEX_constructor_closure == NULL)
      create_ST_FLOAT_COMPLEX_constructor_closure = 
        caml_named_value("create_ST_FLOAT_COMPLEX_constructor");
    xassert(create_ST_FLOAT_COMPLEX_constructor_closure);
    result = caml_callback(*create_ST_FLOAT_COMPLEX_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_DOUBLE_COMPLEX:
    if(create_ST_DOUBLE_COMPLEX_constructor_closure == NULL)
      create_ST_DOUBLE_COMPLEX_constructor_closure = 
        caml_named_value("create_ST_DOUBLE_COMPLEX_constructor");
    xassert(create_ST_DOUBLE_COMPLEX_constructor_closure);
    result = caml_callback(*create_ST_DOUBLE_COMPLEX_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_LONG_DOUBLE_COMPLEX:
    if(create_ST_LONG_DOUBLE_COMPLEX_constructor_closure == NULL)
      create_ST_LONG_DOUBLE_COMPLEX_constructor_closure = 
        caml_named_value("create_ST_LONG_DOUBLE_COMPLEX_constructor");
    xassert(create_ST_LONG_DOUBLE_COMPLEX_constructor_closure);
    result = caml_callback(*create_ST_LONG_DOUBLE_COMPLEX_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_FLOAT_IMAGINARY:
    if(create_ST_FLOAT_IMAGINARY_constructor_closure == NULL)
      create_ST_FLOAT_IMAGINARY_constructor_closure = 
        caml_named_value("create_ST_FLOAT_IMAGINARY_constructor");
    xassert(create_ST_FLOAT_IMAGINARY_constructor_closure);
    result = caml_callback(*create_ST_FLOAT_IMAGINARY_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_DOUBLE_IMAGINARY:
    if(create_ST_DOUBLE_IMAGINARY_constructor_closure == NULL)
      create_ST_DOUBLE_IMAGINARY_constructor_closure = 
        caml_named_value("create_ST_DOUBLE_IMAGINARY_constructor");
    xassert(create_ST_DOUBLE_IMAGINARY_constructor_closure);
    result = caml_callback(*create_ST_DOUBLE_IMAGINARY_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_LONG_DOUBLE_IMAGINARY:
    if(create_ST_LONG_DOUBLE_IMAGINARY_constructor_closure == NULL)
      create_ST_LONG_DOUBLE_IMAGINARY_constructor_closure = 
        caml_named_value("create_ST_LONG_DOUBLE_IMAGINARY_constructor");
    xassert(create_ST_LONG_DOUBLE_IMAGINARY_constructor_closure);
    result = caml_callback(*create_ST_LONG_DOUBLE_IMAGINARY_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_VOID:
    if(create_ST_VOID_constructor_closure == NULL)
      create_ST_VOID_constructor_closure = 
        caml_named_value("create_ST_VOID_constructor");
    xassert(create_ST_VOID_constructor_closure);
    result = caml_callback(*create_ST_VOID_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_ELLIPSIS:
    if(create_ST_ELLIPSIS_constructor_closure == NULL)
      create_ST_ELLIPSIS_constructor_closure = 
        caml_named_value("create_ST_ELLIPSIS_constructor");
    xassert(create_ST_ELLIPSIS_constructor_closure);
    result = caml_callback(*create_ST_ELLIPSIS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_CDTOR:
    if(create_ST_CDTOR_constructor_closure == NULL)
      create_ST_CDTOR_constructor_closure = 
        caml_named_value("create_ST_CDTOR_constructor");
    xassert(create_ST_CDTOR_constructor_closure);
    result = caml_callback(*create_ST_CDTOR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_ERROR:
    if(create_ST_ERROR_constructor_closure == NULL)
      create_ST_ERROR_constructor_closure = 
        caml_named_value("create_ST_ERROR_constructor");
    xassert(create_ST_ERROR_constructor_closure);
    result = caml_callback(*create_ST_ERROR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_DEPENDENT:
    if(create_ST_DEPENDENT_constructor_closure == NULL)
      create_ST_DEPENDENT_constructor_closure = 
        caml_named_value("create_ST_DEPENDENT_constructor");
    xassert(create_ST_DEPENDENT_constructor_closure);
    result = caml_callback(*create_ST_DEPENDENT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_IMPLINT:
    if(create_ST_IMPLINT_constructor_closure == NULL)
      create_ST_IMPLINT_constructor_closure = 
        caml_named_value("create_ST_IMPLINT_constructor");
    xassert(create_ST_IMPLINT_constructor_closure);
    result = caml_callback(*create_ST_IMPLINT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_NOTFOUND:
    if(create_ST_NOTFOUND_constructor_closure == NULL)
      create_ST_NOTFOUND_constructor_closure = 
        caml_named_value("create_ST_NOTFOUND_constructor");
    xassert(create_ST_NOTFOUND_constructor_closure);
    result = caml_callback(*create_ST_NOTFOUND_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_PROMOTED_INTEGRAL:
    if(create_ST_PROMOTED_INTEGRAL_constructor_closure == NULL)
      create_ST_PROMOTED_INTEGRAL_constructor_closure = 
        caml_named_value("create_ST_PROMOTED_INTEGRAL_constructor");
    xassert(create_ST_PROMOTED_INTEGRAL_constructor_closure);
    result = caml_callback(*create_ST_PROMOTED_INTEGRAL_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_PROMOTED_ARITHMETIC:
    if(create_ST_PROMOTED_ARITHMETIC_constructor_closure == NULL)
      create_ST_PROMOTED_ARITHMETIC_constructor_closure = 
        caml_named_value("create_ST_PROMOTED_ARITHMETIC_constructor");
    xassert(create_ST_PROMOTED_ARITHMETIC_constructor_closure);
    result = caml_callback(*create_ST_PROMOTED_ARITHMETIC_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_INTEGRAL:
    if(create_ST_INTEGRAL_constructor_closure == NULL)
      create_ST_INTEGRAL_constructor_closure = 
        caml_named_value("create_ST_INTEGRAL_constructor");
    xassert(create_ST_INTEGRAL_constructor_closure);
    result = caml_callback(*create_ST_INTEGRAL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_ARITHMETIC:
    if(create_ST_ARITHMETIC_constructor_closure == NULL)
      create_ST_ARITHMETIC_constructor_closure = 
        caml_named_value("create_ST_ARITHMETIC_constructor");
    xassert(create_ST_ARITHMETIC_constructor_closure);
    result = caml_callback(*create_ST_ARITHMETIC_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_ARITHMETIC_NON_BOOL:
    if(create_ST_ARITHMETIC_NON_BOOL_constructor_closure == NULL)
      create_ST_ARITHMETIC_NON_BOOL_constructor_closure = 
        caml_named_value("create_ST_ARITHMETIC_NON_BOOL_constructor");
    xassert(create_ST_ARITHMETIC_NON_BOOL_constructor_closure);
    result = caml_callback(*create_ST_ARITHMETIC_NON_BOOL_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_ANY_OBJ_TYPE:
    if(create_ST_ANY_OBJ_TYPE_constructor_closure == NULL)
      create_ST_ANY_OBJ_TYPE_constructor_closure = 
        caml_named_value("create_ST_ANY_OBJ_TYPE_constructor");
    xassert(create_ST_ANY_OBJ_TYPE_constructor_closure);
    result = caml_callback(*create_ST_ANY_OBJ_TYPE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_ANY_NON_VOID:
    if(create_ST_ANY_NON_VOID_constructor_closure == NULL)
      create_ST_ANY_NON_VOID_constructor_closure = 
        caml_named_value("create_ST_ANY_NON_VOID_constructor");
    xassert(create_ST_ANY_NON_VOID_constructor_closure);
    result = caml_callback(*create_ST_ANY_NON_VOID_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_ANY_TYPE:
    if(create_ST_ANY_TYPE_constructor_closure == NULL)
      create_ST_ANY_TYPE_constructor_closure = 
        caml_named_value("create_ST_ANY_TYPE_constructor");
    xassert(create_ST_ANY_TYPE_constructor_closure);
    result = caml_callback(*create_ST_ANY_TYPE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_PRET_STRIP_REF:
    if(create_ST_PRET_STRIP_REF_constructor_closure == NULL)
      create_ST_PRET_STRIP_REF_constructor_closure = 
        caml_named_value("create_ST_PRET_STRIP_REF_constructor");
    xassert(create_ST_PRET_STRIP_REF_constructor_closure);
    result = caml_callback(*create_ST_PRET_STRIP_REF_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_PRET_PTM:
    if(create_ST_PRET_PTM_constructor_closure == NULL)
      create_ST_PRET_PTM_constructor_closure = 
        caml_named_value("create_ST_PRET_PTM_constructor");
    xassert(create_ST_PRET_PTM_constructor_closure);
    result = caml_callback(*create_ST_PRET_PTM_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_PRET_ARITH_CONV:
    if(create_ST_PRET_ARITH_CONV_constructor_closure == NULL)
      create_ST_PRET_ARITH_CONV_constructor_closure = 
        caml_named_value("create_ST_PRET_ARITH_CONV_constructor");
    xassert(create_ST_PRET_ARITH_CONV_constructor_closure);
    result = caml_callback(*create_ST_PRET_ARITH_CONV_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_PRET_FIRST:
    if(create_ST_PRET_FIRST_constructor_closure == NULL)
      create_ST_PRET_FIRST_constructor_closure = 
        caml_named_value("create_ST_PRET_FIRST_constructor");
    xassert(create_ST_PRET_FIRST_constructor_closure);
    result = caml_callback(*create_ST_PRET_FIRST_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_PRET_FIRST_PTR2REF:
    if(create_ST_PRET_FIRST_PTR2REF_constructor_closure == NULL)
      create_ST_PRET_FIRST_PTR2REF_constructor_closure = 
        caml_named_value("create_ST_PRET_FIRST_PTR2REF_constructor");
    xassert(create_ST_PRET_FIRST_PTR2REF_constructor_closure);
    result = caml_callback(*create_ST_PRET_FIRST_PTR2REF_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_PRET_SECOND:
    if(create_ST_PRET_SECOND_constructor_closure == NULL)
      create_ST_PRET_SECOND_constructor_closure = 
        caml_named_value("create_ST_PRET_SECOND_constructor");
    xassert(create_ST_PRET_SECOND_constructor_closure);
    result = caml_callback(*create_ST_PRET_SECOND_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case ST_PRET_SECOND_PTR2REF:
    if(create_ST_PRET_SECOND_PTR2REF_constructor_closure == NULL)
      create_ST_PRET_SECOND_PTR2REF_constructor_closure = 
        caml_named_value("create_ST_PRET_SECOND_PTR2REF_constructor");
    xassert(create_ST_PRET_SECOND_PTR2REF_constructor_closure);
    result = caml_callback(*create_ST_PRET_SECOND_PTR2REF_constructor_closure, 
			 Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}


// hand written ocaml serialization function
value ocaml_from_CVFlags(const CVFlags &f, ToOcamlData *d){
  CAMLparam0();
  CAMLlocal2(camlf, result);

  static value * cVFlag_from_int32_closure = NULL;
  if(cVFlag_from_int32_closure == NULL)
    cVFlag_from_int32_closure = caml_named_value("cVFlag_from_int32");
  xassert(cVFlag_from_int32_closure);

  camlf = caml_copy_int32(f);
  xassert(IS_OCAML_INT32(camlf));
  result = caml_callback(*cVFlag_from_int32_closure, camlf);
  xassert(IS_OCAML_AST_VALUE(result));

  CAMLreturn(result);
}


// hand written ocaml serialization function
value ocaml_from_function_flags(const FunctionFlags &f, ToOcamlData *d){
  CAMLparam0();
  CAMLlocal2(camlf, result);

  static value * function_flags_from_int32_closure = NULL;
  if(function_flags_from_int32_closure == NULL)
    function_flags_from_int32_closure = 
      caml_named_value("function_flags_from_int32");
  xassert(function_flags_from_int32_closure);

  camlf = caml_copy_int32(f);
  xassert(IS_OCAML_INT32(camlf));
  result = caml_callback(*function_flags_from_int32_closure, camlf);
  xassert(IS_OCAML_AST_VALUE(result));

  CAMLreturn(result);
}



// hand written ocaml serialization function
value ocaml_from_TypeIntr(const TypeIntr &id, ToOcamlData *d){
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_TI_STRUCT_constructor_closure = NULL;
  static value * create_TI_CLASS_constructor_closure = NULL;
  static value * create_TI_UNION_constructor_closure = NULL;
  static value * create_TI_ENUM_constructor_closure = NULL;

  value result;

  switch(id){

  case TI_STRUCT:
    if(create_TI_STRUCT_constructor_closure == NULL)
      create_TI_STRUCT_constructor_closure = 
        caml_named_value("create_TI_STRUCT_constructor");
    xassert(create_TI_STRUCT_constructor_closure);
    result = caml_callback(*create_TI_STRUCT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case TI_CLASS:
    if(create_TI_CLASS_constructor_closure == NULL)
      create_TI_CLASS_constructor_closure = 
        caml_named_value("create_TI_CLASS_constructor");
    xassert(create_TI_CLASS_constructor_closure);
    result = caml_callback(*create_TI_CLASS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case TI_UNION:
    if(create_TI_UNION_constructor_closure == NULL)
      create_TI_UNION_constructor_closure = 
        caml_named_value("create_TI_UNION_constructor");
    xassert(create_TI_UNION_constructor_closure);
    result = caml_callback(*create_TI_UNION_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case TI_ENUM:
    if(create_TI_ENUM_constructor_closure == NULL)
      create_TI_ENUM_constructor_closure = 
        caml_named_value("create_TI_ENUM_constructor");
    xassert(create_TI_ENUM_constructor_closure);
    result = caml_callback(*create_TI_ENUM_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}



// hand written ocaml serialization function
value ocaml_from_AccessKeyword(const AccessKeyword &id, ToOcamlData *d){
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_AK_PUBLIC_constructor_closure = NULL;
  static value * create_AK_PROTECTED_constructor_closure = NULL;
  static value * create_AK_PRIVATE_constructor_closure = NULL;
  static value * create_AK_UNSPECIFIED_constructor_closure = NULL;

  value result;

  switch(id){

  case AK_PUBLIC:
    if(create_AK_PUBLIC_constructor_closure == NULL)
      create_AK_PUBLIC_constructor_closure = 
        caml_named_value("create_AK_PUBLIC_constructor");
    xassert(create_AK_PUBLIC_constructor_closure);
    result = caml_callback(*create_AK_PUBLIC_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case AK_PROTECTED:
    if(create_AK_PROTECTED_constructor_closure == NULL)
      create_AK_PROTECTED_constructor_closure = 
        caml_named_value("create_AK_PROTECTED_constructor");
    xassert(create_AK_PROTECTED_constructor_closure);
    result = caml_callback(*create_AK_PROTECTED_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case AK_PRIVATE:
    if(create_AK_PRIVATE_constructor_closure == NULL)
      create_AK_PRIVATE_constructor_closure = 
        caml_named_value("create_AK_PRIVATE_constructor");
    xassert(create_AK_PRIVATE_constructor_closure);
    result = caml_callback(*create_AK_PRIVATE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case AK_UNSPECIFIED:
    if(create_AK_UNSPECIFIED_constructor_closure == NULL)
      create_AK_UNSPECIFIED_constructor_closure = 
        caml_named_value("create_AK_UNSPECIFIED_constructor");
    xassert(create_AK_UNSPECIFIED_constructor_closure);
    result = caml_callback(*create_AK_UNSPECIFIED_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    cerr << "AccessKeyword out of range: " << id << endl << flush;
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}


// hand written ocaml serialization function
value ocaml_from_OverloadableOp(const OverloadableOp &id, ToOcamlData *d){
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_OP_NOT_constructor_closure = NULL;
  static value * create_OP_BITNOT_constructor_closure = NULL;
  static value * create_OP_PLUSPLUS_constructor_closure = NULL;
  static value * create_OP_MINUSMINUS_constructor_closure = NULL;
  static value * create_OP_PLUS_constructor_closure = NULL;
  static value * create_OP_MINUS_constructor_closure = NULL;
  static value * create_OP_STAR_constructor_closure = NULL;
  static value * create_OP_AMPERSAND_constructor_closure = NULL;
  static value * create_OP_DIV_constructor_closure = NULL;
  static value * create_OP_MOD_constructor_closure = NULL;
  static value * create_OP_LSHIFT_constructor_closure = NULL;
  static value * create_OP_RSHIFT_constructor_closure = NULL;
  static value * create_OP_BITXOR_constructor_closure = NULL;
  static value * create_OP_BITOR_constructor_closure = NULL;
  static value * create_OP_ASSIGN_constructor_closure = NULL;
  static value * create_OP_PLUSEQ_constructor_closure = NULL;
  static value * create_OP_MINUSEQ_constructor_closure = NULL;
  static value * create_OP_MULTEQ_constructor_closure = NULL;
  static value * create_OP_DIVEQ_constructor_closure = NULL;
  static value * create_OP_MODEQ_constructor_closure = NULL;
  static value * create_OP_LSHIFTEQ_constructor_closure = NULL;
  static value * create_OP_RSHIFTEQ_constructor_closure = NULL;
  static value * create_OP_BITANDEQ_constructor_closure = NULL;
  static value * create_OP_BITXOREQ_constructor_closure = NULL;
  static value * create_OP_BITOREQ_constructor_closure = NULL;
  static value * create_OP_EQUAL_constructor_closure = NULL;
  static value * create_OP_NOTEQUAL_constructor_closure = NULL;
  static value * create_OP_LESS_constructor_closure = NULL;
  static value * create_OP_GREATER_constructor_closure = NULL;
  static value * create_OP_LESSEQ_constructor_closure = NULL;
  static value * create_OP_GREATEREQ_constructor_closure = NULL;
  static value * create_OP_AND_constructor_closure = NULL;
  static value * create_OP_OR_constructor_closure = NULL;
  static value * create_OP_ARROW_constructor_closure = NULL;
  static value * create_OP_ARROW_STAR_constructor_closure = NULL;
  static value * create_OP_BRACKETS_constructor_closure = NULL;
  static value * create_OP_PARENS_constructor_closure = NULL;
  static value * create_OP_COMMA_constructor_closure = NULL;
  static value * create_OP_QUESTION_constructor_closure = NULL;
  static value * create_OP_MINIMUM_constructor_closure = NULL;
  static value * create_OP_MAXIMUM_constructor_closure = NULL;

  value result;

  switch(id){

  case OP_NOT:
    if(create_OP_NOT_constructor_closure == NULL)
      create_OP_NOT_constructor_closure = 
        caml_named_value("create_OP_NOT_constructor");
    xassert(create_OP_NOT_constructor_closure);
    result = caml_callback(*create_OP_NOT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_BITNOT:
    if(create_OP_BITNOT_constructor_closure == NULL)
      create_OP_BITNOT_constructor_closure = 
        caml_named_value("create_OP_BITNOT_constructor");
    xassert(create_OP_BITNOT_constructor_closure);
    result = caml_callback(*create_OP_BITNOT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_PLUSPLUS:
    if(create_OP_PLUSPLUS_constructor_closure == NULL)
      create_OP_PLUSPLUS_constructor_closure = 
        caml_named_value("create_OP_PLUSPLUS_constructor");
    xassert(create_OP_PLUSPLUS_constructor_closure);
    result = caml_callback(*create_OP_PLUSPLUS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_MINUSMINUS:
    if(create_OP_MINUSMINUS_constructor_closure == NULL)
      create_OP_MINUSMINUS_constructor_closure = 
        caml_named_value("create_OP_MINUSMINUS_constructor");
    xassert(create_OP_MINUSMINUS_constructor_closure);
    result = caml_callback(*create_OP_MINUSMINUS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_PLUS:
    if(create_OP_PLUS_constructor_closure == NULL)
      create_OP_PLUS_constructor_closure = 
        caml_named_value("create_OP_PLUS_constructor");
    xassert(create_OP_PLUS_constructor_closure);
    result = caml_callback(*create_OP_PLUS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_MINUS:
    if(create_OP_MINUS_constructor_closure == NULL)
      create_OP_MINUS_constructor_closure = 
        caml_named_value("create_OP_MINUS_constructor");
    xassert(create_OP_MINUS_constructor_closure);
    result = caml_callback(*create_OP_MINUS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_STAR:
    if(create_OP_STAR_constructor_closure == NULL)
      create_OP_STAR_constructor_closure = 
        caml_named_value("create_OP_STAR_constructor");
    xassert(create_OP_STAR_constructor_closure);
    result = caml_callback(*create_OP_STAR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_AMPERSAND:
    if(create_OP_AMPERSAND_constructor_closure == NULL)
      create_OP_AMPERSAND_constructor_closure = 
        caml_named_value("create_OP_AMPERSAND_constructor");
    xassert(create_OP_AMPERSAND_constructor_closure);
    result = caml_callback(*create_OP_AMPERSAND_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_DIV:
    if(create_OP_DIV_constructor_closure == NULL)
      create_OP_DIV_constructor_closure = 
        caml_named_value("create_OP_DIV_constructor");
    xassert(create_OP_DIV_constructor_closure);
    result = caml_callback(*create_OP_DIV_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_MOD:
    if(create_OP_MOD_constructor_closure == NULL)
      create_OP_MOD_constructor_closure = 
        caml_named_value("create_OP_MOD_constructor");
    xassert(create_OP_MOD_constructor_closure);
    result = caml_callback(*create_OP_MOD_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_LSHIFT:
    if(create_OP_LSHIFT_constructor_closure == NULL)
      create_OP_LSHIFT_constructor_closure = 
        caml_named_value("create_OP_LSHIFT_constructor");
    xassert(create_OP_LSHIFT_constructor_closure);
    result = caml_callback(*create_OP_LSHIFT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_RSHIFT:
    if(create_OP_RSHIFT_constructor_closure == NULL)
      create_OP_RSHIFT_constructor_closure = 
        caml_named_value("create_OP_RSHIFT_constructor");
    xassert(create_OP_RSHIFT_constructor_closure);
    result = caml_callback(*create_OP_RSHIFT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_BITXOR:
    if(create_OP_BITXOR_constructor_closure == NULL)
      create_OP_BITXOR_constructor_closure = 
        caml_named_value("create_OP_BITXOR_constructor");
    xassert(create_OP_BITXOR_constructor_closure);
    result = caml_callback(*create_OP_BITXOR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_BITOR:
    if(create_OP_BITOR_constructor_closure == NULL)
      create_OP_BITOR_constructor_closure = 
        caml_named_value("create_OP_BITOR_constructor");
    xassert(create_OP_BITOR_constructor_closure);
    result = caml_callback(*create_OP_BITOR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_ASSIGN:
    if(create_OP_ASSIGN_constructor_closure == NULL)
      create_OP_ASSIGN_constructor_closure = 
        caml_named_value("create_OP_ASSIGN_constructor");
    xassert(create_OP_ASSIGN_constructor_closure);
    result = caml_callback(*create_OP_ASSIGN_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_PLUSEQ:
    if(create_OP_PLUSEQ_constructor_closure == NULL)
      create_OP_PLUSEQ_constructor_closure = 
        caml_named_value("create_OP_PLUSEQ_constructor");
    xassert(create_OP_PLUSEQ_constructor_closure);
    result = caml_callback(*create_OP_PLUSEQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_MINUSEQ:
    if(create_OP_MINUSEQ_constructor_closure == NULL)
      create_OP_MINUSEQ_constructor_closure = 
        caml_named_value("create_OP_MINUSEQ_constructor");
    xassert(create_OP_MINUSEQ_constructor_closure);
    result = caml_callback(*create_OP_MINUSEQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_MULTEQ:
    if(create_OP_MULTEQ_constructor_closure == NULL)
      create_OP_MULTEQ_constructor_closure = 
        caml_named_value("create_OP_MULTEQ_constructor");
    xassert(create_OP_MULTEQ_constructor_closure);
    result = caml_callback(*create_OP_MULTEQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_DIVEQ:
    if(create_OP_DIVEQ_constructor_closure == NULL)
      create_OP_DIVEQ_constructor_closure = 
        caml_named_value("create_OP_DIVEQ_constructor");
    xassert(create_OP_DIVEQ_constructor_closure);
    result = caml_callback(*create_OP_DIVEQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_MODEQ:
    if(create_OP_MODEQ_constructor_closure == NULL)
      create_OP_MODEQ_constructor_closure = 
        caml_named_value("create_OP_MODEQ_constructor");
    xassert(create_OP_MODEQ_constructor_closure);
    result = caml_callback(*create_OP_MODEQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_LSHIFTEQ:
    if(create_OP_LSHIFTEQ_constructor_closure == NULL)
      create_OP_LSHIFTEQ_constructor_closure = 
        caml_named_value("create_OP_LSHIFTEQ_constructor");
    xassert(create_OP_LSHIFTEQ_constructor_closure);
    result = caml_callback(*create_OP_LSHIFTEQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_RSHIFTEQ:
    if(create_OP_RSHIFTEQ_constructor_closure == NULL)
      create_OP_RSHIFTEQ_constructor_closure = 
        caml_named_value("create_OP_RSHIFTEQ_constructor");
    xassert(create_OP_RSHIFTEQ_constructor_closure);
    result = caml_callback(*create_OP_RSHIFTEQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_BITANDEQ:
    if(create_OP_BITANDEQ_constructor_closure == NULL)
      create_OP_BITANDEQ_constructor_closure = 
        caml_named_value("create_OP_BITANDEQ_constructor");
    xassert(create_OP_BITANDEQ_constructor_closure);
    result = caml_callback(*create_OP_BITANDEQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_BITXOREQ:
    if(create_OP_BITXOREQ_constructor_closure == NULL)
      create_OP_BITXOREQ_constructor_closure = 
        caml_named_value("create_OP_BITXOREQ_constructor");
    xassert(create_OP_BITXOREQ_constructor_closure);
    result = caml_callback(*create_OP_BITXOREQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_BITOREQ:
    if(create_OP_BITOREQ_constructor_closure == NULL)
      create_OP_BITOREQ_constructor_closure = 
        caml_named_value("create_OP_BITOREQ_constructor");
    xassert(create_OP_BITOREQ_constructor_closure);
    result = caml_callback(*create_OP_BITOREQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_EQUAL:
    if(create_OP_EQUAL_constructor_closure == NULL)
      create_OP_EQUAL_constructor_closure = 
        caml_named_value("create_OP_EQUAL_constructor");
    xassert(create_OP_EQUAL_constructor_closure);
    result = caml_callback(*create_OP_EQUAL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_NOTEQUAL:
    if(create_OP_NOTEQUAL_constructor_closure == NULL)
      create_OP_NOTEQUAL_constructor_closure = 
        caml_named_value("create_OP_NOTEQUAL_constructor");
    xassert(create_OP_NOTEQUAL_constructor_closure);
    result = caml_callback(*create_OP_NOTEQUAL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_LESS:
    if(create_OP_LESS_constructor_closure == NULL)
      create_OP_LESS_constructor_closure = 
        caml_named_value("create_OP_LESS_constructor");
    xassert(create_OP_LESS_constructor_closure);
    result = caml_callback(*create_OP_LESS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_GREATER:
    if(create_OP_GREATER_constructor_closure == NULL)
      create_OP_GREATER_constructor_closure = 
        caml_named_value("create_OP_GREATER_constructor");
    xassert(create_OP_GREATER_constructor_closure);
    result = caml_callback(*create_OP_GREATER_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_LESSEQ:
    if(create_OP_LESSEQ_constructor_closure == NULL)
      create_OP_LESSEQ_constructor_closure = 
        caml_named_value("create_OP_LESSEQ_constructor");
    xassert(create_OP_LESSEQ_constructor_closure);
    result = caml_callback(*create_OP_LESSEQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_GREATEREQ:
    if(create_OP_GREATEREQ_constructor_closure == NULL)
      create_OP_GREATEREQ_constructor_closure = 
        caml_named_value("create_OP_GREATEREQ_constructor");
    xassert(create_OP_GREATEREQ_constructor_closure);
    result = caml_callback(*create_OP_GREATEREQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_AND:
    if(create_OP_AND_constructor_closure == NULL)
      create_OP_AND_constructor_closure = 
        caml_named_value("create_OP_AND_constructor");
    xassert(create_OP_AND_constructor_closure);
    result = caml_callback(*create_OP_AND_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_OR:
    if(create_OP_OR_constructor_closure == NULL)
      create_OP_OR_constructor_closure = 
        caml_named_value("create_OP_OR_constructor");
    xassert(create_OP_OR_constructor_closure);
    result = caml_callback(*create_OP_OR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_ARROW:
    if(create_OP_ARROW_constructor_closure == NULL)
      create_OP_ARROW_constructor_closure = 
        caml_named_value("create_OP_ARROW_constructor");
    xassert(create_OP_ARROW_constructor_closure);
    result = caml_callback(*create_OP_ARROW_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_ARROW_STAR:
    if(create_OP_ARROW_STAR_constructor_closure == NULL)
      create_OP_ARROW_STAR_constructor_closure = 
        caml_named_value("create_OP_ARROW_STAR_constructor");
    xassert(create_OP_ARROW_STAR_constructor_closure);
    result = caml_callback(*create_OP_ARROW_STAR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_BRACKETS:
    if(create_OP_BRACKETS_constructor_closure == NULL)
      create_OP_BRACKETS_constructor_closure = 
        caml_named_value("create_OP_BRACKETS_constructor");
    xassert(create_OP_BRACKETS_constructor_closure);
    result = caml_callback(*create_OP_BRACKETS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_PARENS:
    if(create_OP_PARENS_constructor_closure == NULL)
      create_OP_PARENS_constructor_closure = 
        caml_named_value("create_OP_PARENS_constructor");
    xassert(create_OP_PARENS_constructor_closure);
    result = caml_callback(*create_OP_PARENS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_COMMA:
    if(create_OP_COMMA_constructor_closure == NULL)
      create_OP_COMMA_constructor_closure = 
        caml_named_value("create_OP_COMMA_constructor");
    xassert(create_OP_COMMA_constructor_closure);
    result = caml_callback(*create_OP_COMMA_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_QUESTION:
    if(create_OP_QUESTION_constructor_closure == NULL)
      create_OP_QUESTION_constructor_closure = 
        caml_named_value("create_OP_QUESTION_constructor");
    xassert(create_OP_QUESTION_constructor_closure);
    result = caml_callback(*create_OP_QUESTION_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_MINIMUM:
    if(create_OP_MINIMUM_constructor_closure == NULL)
      create_OP_MINIMUM_constructor_closure = 
        caml_named_value("create_OP_MINIMUM_constructor");
    xassert(create_OP_MINIMUM_constructor_closure);
    result = caml_callback(*create_OP_MINIMUM_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case OP_MAXIMUM:
    if(create_OP_MAXIMUM_constructor_closure == NULL)
      create_OP_MAXIMUM_constructor_closure = 
        caml_named_value("create_OP_MAXIMUM_constructor");
    xassert(create_OP_MAXIMUM_constructor_closure);
    result = caml_callback(*create_OP_MAXIMUM_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}


// hand written ocaml serialization function
value ocaml_from_UnaryOp(const UnaryOp &id, ToOcamlData *d){
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_UNY_PLUS_constructor_closure = NULL;
  static value * create_UNY_MINUS_constructor_closure = NULL;
  static value * create_UNY_NOT_constructor_closure = NULL;
  static value * create_UNY_BITNOT_constructor_closure = NULL;

  value result;

  switch(id){

  case UNY_PLUS:
    if(create_UNY_PLUS_constructor_closure == NULL)
      create_UNY_PLUS_constructor_closure = 
        caml_named_value("create_UNY_PLUS_constructor");
    xassert(create_UNY_PLUS_constructor_closure);
    result = caml_callback(*create_UNY_PLUS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case UNY_MINUS:
    if(create_UNY_MINUS_constructor_closure == NULL)
      create_UNY_MINUS_constructor_closure = 
        caml_named_value("create_UNY_MINUS_constructor");
    xassert(create_UNY_MINUS_constructor_closure);
    result = caml_callback(*create_UNY_MINUS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case UNY_NOT:
    if(create_UNY_NOT_constructor_closure == NULL)
      create_UNY_NOT_constructor_closure = 
        caml_named_value("create_UNY_NOT_constructor");
    xassert(create_UNY_NOT_constructor_closure);
    result = caml_callback(*create_UNY_NOT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case UNY_BITNOT:
    if(create_UNY_BITNOT_constructor_closure == NULL)
      create_UNY_BITNOT_constructor_closure = 
        caml_named_value("create_UNY_BITNOT_constructor");
    xassert(create_UNY_BITNOT_constructor_closure);
    result = caml_callback(*create_UNY_BITNOT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}


// hand written ocaml serialization function
value ocaml_from_EffectOp(const EffectOp &id, ToOcamlData *d){
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_EFF_POSTINC_constructor_closure = NULL;
  static value * create_EFF_POSTDEC_constructor_closure = NULL;
  static value * create_EFF_PREINC_constructor_closure = NULL;
  static value * create_EFF_PREDEC_constructor_closure = NULL;

  value result;

  switch(id){

  case EFF_POSTINC:
    if(create_EFF_POSTINC_constructor_closure == NULL)
      create_EFF_POSTINC_constructor_closure = 
        caml_named_value("create_EFF_POSTINC_constructor");
    xassert(create_EFF_POSTINC_constructor_closure);
    result = caml_callback(*create_EFF_POSTINC_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case EFF_POSTDEC:
    if(create_EFF_POSTDEC_constructor_closure == NULL)
      create_EFF_POSTDEC_constructor_closure = 
        caml_named_value("create_EFF_POSTDEC_constructor");
    xassert(create_EFF_POSTDEC_constructor_closure);
    result = caml_callback(*create_EFF_POSTDEC_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case EFF_PREINC:
    if(create_EFF_PREINC_constructor_closure == NULL)
      create_EFF_PREINC_constructor_closure = 
        caml_named_value("create_EFF_PREINC_constructor");
    xassert(create_EFF_PREINC_constructor_closure);
    result = caml_callback(*create_EFF_PREINC_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case EFF_PREDEC:
    if(create_EFF_PREDEC_constructor_closure == NULL)
      create_EFF_PREDEC_constructor_closure = 
        caml_named_value("create_EFF_PREDEC_constructor");
    xassert(create_EFF_PREDEC_constructor_closure);
    result = caml_callback(*create_EFF_PREDEC_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}


// hand written ocaml serialization function
value ocaml_from_BinaryOp(const BinaryOp &id, ToOcamlData *d){
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_BIN_EQUAL_constructor_closure = NULL;
  static value * create_BIN_NOTEQUAL_constructor_closure = NULL;
  static value * create_BIN_LESS_constructor_closure = NULL;
  static value * create_BIN_GREATER_constructor_closure = NULL;
  static value * create_BIN_LESSEQ_constructor_closure = NULL;
  static value * create_BIN_GREATEREQ_constructor_closure = NULL;
  static value * create_BIN_MULT_constructor_closure = NULL;
  static value * create_BIN_DIV_constructor_closure = NULL;
  static value * create_BIN_MOD_constructor_closure = NULL;
  static value * create_BIN_PLUS_constructor_closure = NULL;
  static value * create_BIN_MINUS_constructor_closure = NULL;
  static value * create_BIN_LSHIFT_constructor_closure = NULL;
  static value * create_BIN_RSHIFT_constructor_closure = NULL;
  static value * create_BIN_BITAND_constructor_closure = NULL;
  static value * create_BIN_BITXOR_constructor_closure = NULL;
  static value * create_BIN_BITOR_constructor_closure = NULL;
  static value * create_BIN_AND_constructor_closure = NULL;
  static value * create_BIN_OR_constructor_closure = NULL;
  static value * create_BIN_COMMA_constructor_closure = NULL;
  static value * create_BIN_MINIMUM_constructor_closure = NULL;
  static value * create_BIN_MAXIMUM_constructor_closure = NULL;
  static value * create_BIN_BRACKETS_constructor_closure = NULL;
  static value * create_BIN_ASSIGN_constructor_closure = NULL;
  static value * create_BIN_DOT_STAR_constructor_closure = NULL;
  static value * create_BIN_ARROW_STAR_constructor_closure = NULL;
  static value * create_BIN_IMPLIES_constructor_closure = NULL;
  static value * create_BIN_EQUIVALENT_constructor_closure = NULL;

  value result;

  switch(id){

  case BIN_EQUAL:
    if(create_BIN_EQUAL_constructor_closure == NULL)
      create_BIN_EQUAL_constructor_closure = 
        caml_named_value("create_BIN_EQUAL_constructor");
    xassert(create_BIN_EQUAL_constructor_closure);
    result = caml_callback(*create_BIN_EQUAL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_NOTEQUAL:
    if(create_BIN_NOTEQUAL_constructor_closure == NULL)
      create_BIN_NOTEQUAL_constructor_closure = 
        caml_named_value("create_BIN_NOTEQUAL_constructor");
    xassert(create_BIN_NOTEQUAL_constructor_closure);
    result = caml_callback(*create_BIN_NOTEQUAL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_LESS:
    if(create_BIN_LESS_constructor_closure == NULL)
      create_BIN_LESS_constructor_closure = 
        caml_named_value("create_BIN_LESS_constructor");
    xassert(create_BIN_LESS_constructor_closure);
    result = caml_callback(*create_BIN_LESS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_GREATER:
    if(create_BIN_GREATER_constructor_closure == NULL)
      create_BIN_GREATER_constructor_closure = 
        caml_named_value("create_BIN_GREATER_constructor");
    xassert(create_BIN_GREATER_constructor_closure);
    result = caml_callback(*create_BIN_GREATER_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_LESSEQ:
    if(create_BIN_LESSEQ_constructor_closure == NULL)
      create_BIN_LESSEQ_constructor_closure = 
        caml_named_value("create_BIN_LESSEQ_constructor");
    xassert(create_BIN_LESSEQ_constructor_closure);
    result = caml_callback(*create_BIN_LESSEQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_GREATEREQ:
    if(create_BIN_GREATEREQ_constructor_closure == NULL)
      create_BIN_GREATEREQ_constructor_closure = 
        caml_named_value("create_BIN_GREATEREQ_constructor");
    xassert(create_BIN_GREATEREQ_constructor_closure);
    result = caml_callback(*create_BIN_GREATEREQ_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_MULT:
    if(create_BIN_MULT_constructor_closure == NULL)
      create_BIN_MULT_constructor_closure = 
        caml_named_value("create_BIN_MULT_constructor");
    xassert(create_BIN_MULT_constructor_closure);
    result = caml_callback(*create_BIN_MULT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_DIV:
    if(create_BIN_DIV_constructor_closure == NULL)
      create_BIN_DIV_constructor_closure = 
        caml_named_value("create_BIN_DIV_constructor");
    xassert(create_BIN_DIV_constructor_closure);
    result = caml_callback(*create_BIN_DIV_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_MOD:
    if(create_BIN_MOD_constructor_closure == NULL)
      create_BIN_MOD_constructor_closure = 
        caml_named_value("create_BIN_MOD_constructor");
    xassert(create_BIN_MOD_constructor_closure);
    result = caml_callback(*create_BIN_MOD_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_PLUS:
    if(create_BIN_PLUS_constructor_closure == NULL)
      create_BIN_PLUS_constructor_closure = 
        caml_named_value("create_BIN_PLUS_constructor");
    xassert(create_BIN_PLUS_constructor_closure);
    result = caml_callback(*create_BIN_PLUS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_MINUS:
    if(create_BIN_MINUS_constructor_closure == NULL)
      create_BIN_MINUS_constructor_closure = 
        caml_named_value("create_BIN_MINUS_constructor");
    xassert(create_BIN_MINUS_constructor_closure);
    result = caml_callback(*create_BIN_MINUS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_LSHIFT:
    if(create_BIN_LSHIFT_constructor_closure == NULL)
      create_BIN_LSHIFT_constructor_closure = 
        caml_named_value("create_BIN_LSHIFT_constructor");
    xassert(create_BIN_LSHIFT_constructor_closure);
    result = caml_callback(*create_BIN_LSHIFT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_RSHIFT:
    if(create_BIN_RSHIFT_constructor_closure == NULL)
      create_BIN_RSHIFT_constructor_closure = 
        caml_named_value("create_BIN_RSHIFT_constructor");
    xassert(create_BIN_RSHIFT_constructor_closure);
    result = caml_callback(*create_BIN_RSHIFT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_BITAND:
    if(create_BIN_BITAND_constructor_closure == NULL)
      create_BIN_BITAND_constructor_closure = 
        caml_named_value("create_BIN_BITAND_constructor");
    xassert(create_BIN_BITAND_constructor_closure);
    result = caml_callback(*create_BIN_BITAND_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_BITXOR:
    if(create_BIN_BITXOR_constructor_closure == NULL)
      create_BIN_BITXOR_constructor_closure = 
        caml_named_value("create_BIN_BITXOR_constructor");
    xassert(create_BIN_BITXOR_constructor_closure);
    result = caml_callback(*create_BIN_BITXOR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_BITOR:
    if(create_BIN_BITOR_constructor_closure == NULL)
      create_BIN_BITOR_constructor_closure = 
        caml_named_value("create_BIN_BITOR_constructor");
    xassert(create_BIN_BITOR_constructor_closure);
    result = caml_callback(*create_BIN_BITOR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_AND:
    if(create_BIN_AND_constructor_closure == NULL)
      create_BIN_AND_constructor_closure = 
        caml_named_value("create_BIN_AND_constructor");
    xassert(create_BIN_AND_constructor_closure);
    result = caml_callback(*create_BIN_AND_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_OR:
    if(create_BIN_OR_constructor_closure == NULL)
      create_BIN_OR_constructor_closure = 
        caml_named_value("create_BIN_OR_constructor");
    xassert(create_BIN_OR_constructor_closure);
    result = caml_callback(*create_BIN_OR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_COMMA:
    if(create_BIN_COMMA_constructor_closure == NULL)
      create_BIN_COMMA_constructor_closure = 
        caml_named_value("create_BIN_COMMA_constructor");
    xassert(create_BIN_COMMA_constructor_closure);
    result = caml_callback(*create_BIN_COMMA_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_MINIMUM:
    if(create_BIN_MINIMUM_constructor_closure == NULL)
      create_BIN_MINIMUM_constructor_closure = 
        caml_named_value("create_BIN_MINIMUM_constructor");
    xassert(create_BIN_MINIMUM_constructor_closure);
    result = caml_callback(*create_BIN_MINIMUM_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_MAXIMUM:
    if(create_BIN_MAXIMUM_constructor_closure == NULL)
      create_BIN_MAXIMUM_constructor_closure = 
        caml_named_value("create_BIN_MAXIMUM_constructor");
    xassert(create_BIN_MAXIMUM_constructor_closure);
    result = caml_callback(*create_BIN_MAXIMUM_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_BRACKETS:
    if(create_BIN_BRACKETS_constructor_closure == NULL)
      create_BIN_BRACKETS_constructor_closure = 
        caml_named_value("create_BIN_BRACKETS_constructor");
    xassert(create_BIN_BRACKETS_constructor_closure);
    result = caml_callback(*create_BIN_BRACKETS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_ASSIGN:
    if(create_BIN_ASSIGN_constructor_closure == NULL)
      create_BIN_ASSIGN_constructor_closure = 
        caml_named_value("create_BIN_ASSIGN_constructor");
    xassert(create_BIN_ASSIGN_constructor_closure);
    result = caml_callback(*create_BIN_ASSIGN_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_DOT_STAR:
    if(create_BIN_DOT_STAR_constructor_closure == NULL)
      create_BIN_DOT_STAR_constructor_closure = 
        caml_named_value("create_BIN_DOT_STAR_constructor");
    xassert(create_BIN_DOT_STAR_constructor_closure);
    result = caml_callback(*create_BIN_DOT_STAR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_ARROW_STAR:
    if(create_BIN_ARROW_STAR_constructor_closure == NULL)
      create_BIN_ARROW_STAR_constructor_closure = 
        caml_named_value("create_BIN_ARROW_STAR_constructor");
    xassert(create_BIN_ARROW_STAR_constructor_closure);
    result = caml_callback(*create_BIN_ARROW_STAR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_IMPLIES:
    if(create_BIN_IMPLIES_constructor_closure == NULL)
      create_BIN_IMPLIES_constructor_closure = 
        caml_named_value("create_BIN_IMPLIES_constructor");
    xassert(create_BIN_IMPLIES_constructor_closure);
    result = caml_callback(*create_BIN_IMPLIES_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case BIN_EQUIVALENT:
    if(create_BIN_EQUIVALENT_constructor_closure == NULL)
      create_BIN_EQUIVALENT_constructor_closure = 
        caml_named_value("create_BIN_EQUIVALENT_constructor");
    xassert(create_BIN_EQUIVALENT_constructor_closure);
    result = caml_callback(*create_BIN_EQUIVALENT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}


// hand written ocaml serialization function
value ocaml_from_CastKeyword(const CastKeyword &id, ToOcamlData *d){
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_CK_DYNAMIC_constructor_closure = NULL;
  static value * create_CK_STATIC_constructor_closure = NULL;
  static value * create_CK_REINTERPRET_constructor_closure = NULL;
  static value * create_CK_CONST_constructor_closure = NULL;

  value result;

  switch(id){

  case CK_DYNAMIC:
    if(create_CK_DYNAMIC_constructor_closure == NULL)
      create_CK_DYNAMIC_constructor_closure = 
        caml_named_value("create_CK_DYNAMIC_constructor");
    xassert(create_CK_DYNAMIC_constructor_closure);
    result = caml_callback(*create_CK_DYNAMIC_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case CK_STATIC:
    if(create_CK_STATIC_constructor_closure == NULL)
      create_CK_STATIC_constructor_closure = 
        caml_named_value("create_CK_STATIC_constructor");
    xassert(create_CK_STATIC_constructor_closure);
    result = caml_callback(*create_CK_STATIC_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case CK_REINTERPRET:
    if(create_CK_REINTERPRET_constructor_closure == NULL)
      create_CK_REINTERPRET_constructor_closure = 
        caml_named_value("create_CK_REINTERPRET_constructor");
    xassert(create_CK_REINTERPRET_constructor_closure);
    result = caml_callback(*create_CK_REINTERPRET_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case CK_CONST:
    if(create_CK_CONST_constructor_closure == NULL)
      create_CK_CONST_constructor_closure = 
        caml_named_value("create_CK_CONST_constructor");
    xassert(create_CK_CONST_constructor_closure);
    result = caml_callback(*create_CK_CONST_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}



// hand written ocaml serialization function
value ocaml_from_CompoundType_Keyword(const CompoundType::Keyword &id, 
				      ToOcamlData *d){
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_K_STRUCT_constructor_closure = NULL;
  static value * create_K_CLASS_constructor_closure = NULL;
  static value * create_K_UNION_constructor_closure = NULL;

  switch(id){

  case CompoundType::K_STRUCT:
    if(create_K_STRUCT_constructor_closure == NULL)
      create_K_STRUCT_constructor_closure = 
        caml_named_value("create_K_STRUCT_constructor");
    xassert(create_K_STRUCT_constructor_closure);
    return caml_callback(*create_K_STRUCT_constructor_closure, Val_unit);

  case CompoundType::K_CLASS:
    if(create_K_CLASS_constructor_closure == NULL)
      create_K_CLASS_constructor_closure = 
        caml_named_value("create_K_CLASS_constructor");
    xassert(create_K_CLASS_constructor_closure);
    return caml_callback(*create_K_CLASS_constructor_closure, Val_unit);

  case CompoundType::K_UNION:
    if(create_K_UNION_constructor_closure == NULL)
      create_K_UNION_constructor_closure = 
        caml_named_value("create_K_UNION_constructor");
    xassert(create_K_UNION_constructor_closure);
    return caml_callback(*create_K_UNION_constructor_closure, Val_unit);

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}


// hand written ocaml serialization function
value ocaml_from_DeclaratorContext(const DeclaratorContext &id, ToOcamlData *d){
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_DC_UNKNOWN_constructor_closure = NULL;
  static value * create_DC_FUNCTION_constructor_closure = NULL;
  static value * create_DC_TF_DECL_constructor_closure = NULL;
  static value * create_DC_TF_EXPLICITINST_constructor_closure = NULL;
  static value * create_DC_MR_DECL_constructor_closure = NULL;
  static value * create_DC_S_DECL_constructor_closure = NULL;
  static value * create_DC_TD_DECL_constructor_closure = NULL;
  static value * create_DC_FEA_constructor_closure = NULL;
  static value * create_DC_D_FUNC_constructor_closure = NULL;
  static value * create_DC_EXCEPTIONSPEC_constructor_closure = NULL;
  static value * create_DC_ON_CONVERSION_constructor_closure = NULL;
  static value * create_DC_CN_DECL_constructor_closure = NULL;
  static value * create_DC_HANDLER_constructor_closure = NULL;
  static value * create_DC_E_CAST_constructor_closure = NULL;
  static value * create_DC_E_SIZEOFTYPE_constructor_closure = NULL;
  static value * create_DC_E_NEW_constructor_closure = NULL;
  static value * create_DC_E_KEYWORDCAST_constructor_closure = NULL;
  static value * create_DC_E_TYPEIDTYPE_constructor_closure = NULL;
  static value * create_DC_TP_TYPE_constructor_closure = NULL;
  static value * create_DC_TP_NONTYPE_constructor_closure = NULL;
  static value * create_DC_TA_TYPE_constructor_closure = NULL;
  static value * create_DC_TS_TYPEOF_TYPE_constructor_closure = NULL;
  static value * create_DC_E_COMPOUNDLIT_constructor_closure = NULL;
  static value * create_DC_E_ALIGNOFTYPE_constructor_closure = NULL;
  static value * create_DC_E_BUILTIN_VA_ARG_constructor_closure = NULL;

  value result;

  switch(id){

  case DC_UNKNOWN:
    if(create_DC_UNKNOWN_constructor_closure == NULL)
      create_DC_UNKNOWN_constructor_closure = 
        caml_named_value("create_DC_UNKNOWN_constructor");
    xassert(create_DC_UNKNOWN_constructor_closure);
    result = caml_callback(*create_DC_UNKNOWN_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_FUNCTION:
    if(create_DC_FUNCTION_constructor_closure == NULL)
      create_DC_FUNCTION_constructor_closure = 
        caml_named_value("create_DC_FUNCTION_constructor");
    xassert(create_DC_FUNCTION_constructor_closure);
    result = caml_callback(*create_DC_FUNCTION_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_TF_DECL:
    if(create_DC_TF_DECL_constructor_closure == NULL)
      create_DC_TF_DECL_constructor_closure = 
        caml_named_value("create_DC_TF_DECL_constructor");
    xassert(create_DC_TF_DECL_constructor_closure);
    result = caml_callback(*create_DC_TF_DECL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_TF_EXPLICITINST:
    if(create_DC_TF_EXPLICITINST_constructor_closure == NULL)
      create_DC_TF_EXPLICITINST_constructor_closure = 
        caml_named_value("create_DC_TF_EXPLICITINST_constructor");
    xassert(create_DC_TF_EXPLICITINST_constructor_closure);
    result = caml_callback(*create_DC_TF_EXPLICITINST_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_MR_DECL:
    if(create_DC_MR_DECL_constructor_closure == NULL)
      create_DC_MR_DECL_constructor_closure = 
        caml_named_value("create_DC_MR_DECL_constructor");
    xassert(create_DC_MR_DECL_constructor_closure);
    result = caml_callback(*create_DC_MR_DECL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_S_DECL:
    if(create_DC_S_DECL_constructor_closure == NULL)
      create_DC_S_DECL_constructor_closure = 
        caml_named_value("create_DC_S_DECL_constructor");
    xassert(create_DC_S_DECL_constructor_closure);
    result = caml_callback(*create_DC_S_DECL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_TD_DECL:
    if(create_DC_TD_DECL_constructor_closure == NULL)
      create_DC_TD_DECL_constructor_closure = 
        caml_named_value("create_DC_TD_DECL_constructor");
    xassert(create_DC_TD_DECL_constructor_closure);
    result = caml_callback(*create_DC_TD_DECL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_FEA:
    if(create_DC_FEA_constructor_closure == NULL)
      create_DC_FEA_constructor_closure = 
        caml_named_value("create_DC_FEA_constructor");
    xassert(create_DC_FEA_constructor_closure);
    result = caml_callback(*create_DC_FEA_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_D_FUNC:
    if(create_DC_D_FUNC_constructor_closure == NULL)
      create_DC_D_FUNC_constructor_closure = 
        caml_named_value("create_DC_D_FUNC_constructor");
    xassert(create_DC_D_FUNC_constructor_closure);
    result = caml_callback(*create_DC_D_FUNC_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_EXCEPTIONSPEC:
    if(create_DC_EXCEPTIONSPEC_constructor_closure == NULL)
      create_DC_EXCEPTIONSPEC_constructor_closure = 
        caml_named_value("create_DC_EXCEPTIONSPEC_constructor");
    xassert(create_DC_EXCEPTIONSPEC_constructor_closure);
    result = caml_callback(*create_DC_EXCEPTIONSPEC_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_ON_CONVERSION:
    if(create_DC_ON_CONVERSION_constructor_closure == NULL)
      create_DC_ON_CONVERSION_constructor_closure = 
        caml_named_value("create_DC_ON_CONVERSION_constructor");
    xassert(create_DC_ON_CONVERSION_constructor_closure);
    result = caml_callback(*create_DC_ON_CONVERSION_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_CN_DECL:
    if(create_DC_CN_DECL_constructor_closure == NULL)
      create_DC_CN_DECL_constructor_closure = 
        caml_named_value("create_DC_CN_DECL_constructor");
    xassert(create_DC_CN_DECL_constructor_closure);
    result = caml_callback(*create_DC_CN_DECL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_HANDLER:
    if(create_DC_HANDLER_constructor_closure == NULL)
      create_DC_HANDLER_constructor_closure = 
        caml_named_value("create_DC_HANDLER_constructor");
    xassert(create_DC_HANDLER_constructor_closure);
    result = caml_callback(*create_DC_HANDLER_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_E_CAST:
    if(create_DC_E_CAST_constructor_closure == NULL)
      create_DC_E_CAST_constructor_closure = 
        caml_named_value("create_DC_E_CAST_constructor");
    xassert(create_DC_E_CAST_constructor_closure);
    result = caml_callback(*create_DC_E_CAST_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_E_SIZEOFTYPE:
    if(create_DC_E_SIZEOFTYPE_constructor_closure == NULL)
      create_DC_E_SIZEOFTYPE_constructor_closure = 
        caml_named_value("create_DC_E_SIZEOFTYPE_constructor");
    xassert(create_DC_E_SIZEOFTYPE_constructor_closure);
    result = caml_callback(*create_DC_E_SIZEOFTYPE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_E_NEW:
    if(create_DC_E_NEW_constructor_closure == NULL)
      create_DC_E_NEW_constructor_closure = 
        caml_named_value("create_DC_E_NEW_constructor");
    xassert(create_DC_E_NEW_constructor_closure);
    result = caml_callback(*create_DC_E_NEW_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_E_KEYWORDCAST:
    if(create_DC_E_KEYWORDCAST_constructor_closure == NULL)
      create_DC_E_KEYWORDCAST_constructor_closure = 
        caml_named_value("create_DC_E_KEYWORDCAST_constructor");
    xassert(create_DC_E_KEYWORDCAST_constructor_closure);
    result = caml_callback(*create_DC_E_KEYWORDCAST_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_E_TYPEIDTYPE:
    if(create_DC_E_TYPEIDTYPE_constructor_closure == NULL)
      create_DC_E_TYPEIDTYPE_constructor_closure = 
        caml_named_value("create_DC_E_TYPEIDTYPE_constructor");
    xassert(create_DC_E_TYPEIDTYPE_constructor_closure);
    result = caml_callback(*create_DC_E_TYPEIDTYPE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_TP_TYPE:
    if(create_DC_TP_TYPE_constructor_closure == NULL)
      create_DC_TP_TYPE_constructor_closure = 
        caml_named_value("create_DC_TP_TYPE_constructor");
    xassert(create_DC_TP_TYPE_constructor_closure);
    result = caml_callback(*create_DC_TP_TYPE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_TP_NONTYPE:
    if(create_DC_TP_NONTYPE_constructor_closure == NULL)
      create_DC_TP_NONTYPE_constructor_closure = 
        caml_named_value("create_DC_TP_NONTYPE_constructor");
    xassert(create_DC_TP_NONTYPE_constructor_closure);
    result = caml_callback(*create_DC_TP_NONTYPE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_TA_TYPE:
    if(create_DC_TA_TYPE_constructor_closure == NULL)
      create_DC_TA_TYPE_constructor_closure = 
        caml_named_value("create_DC_TA_TYPE_constructor");
    xassert(create_DC_TA_TYPE_constructor_closure);
    result = caml_callback(*create_DC_TA_TYPE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_TS_TYPEOF_TYPE:
    if(create_DC_TS_TYPEOF_TYPE_constructor_closure == NULL)
      create_DC_TS_TYPEOF_TYPE_constructor_closure = 
        caml_named_value("create_DC_TS_TYPEOF_TYPE_constructor");
    xassert(create_DC_TS_TYPEOF_TYPE_constructor_closure);
    result = caml_callback(*create_DC_TS_TYPEOF_TYPE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_E_COMPOUNDLIT:
    if(create_DC_E_COMPOUNDLIT_constructor_closure == NULL)
      create_DC_E_COMPOUNDLIT_constructor_closure = 
        caml_named_value("create_DC_E_COMPOUNDLIT_constructor");
    xassert(create_DC_E_COMPOUNDLIT_constructor_closure);
    result = caml_callback(*create_DC_E_COMPOUNDLIT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_E_ALIGNOFTYPE:
    if(create_DC_E_ALIGNOFTYPE_constructor_closure == NULL)
      create_DC_E_ALIGNOFTYPE_constructor_closure = 
        caml_named_value("create_DC_E_ALIGNOFTYPE_constructor");
    xassert(create_DC_E_ALIGNOFTYPE_constructor_closure);
    result = caml_callback(*create_DC_E_ALIGNOFTYPE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case DC_E_BUILTIN_VA_ARG:
    if(create_DC_E_BUILTIN_VA_ARG_constructor_closure == NULL)
      create_DC_E_BUILTIN_VA_ARG_constructor_closure = 
        caml_named_value("create_DC_E_BUILTIN_VA_ARG_constructor");
    xassert(create_DC_E_BUILTIN_VA_ARG_constructor_closure);
    result = caml_callback(*create_DC_E_BUILTIN_VA_ARG_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}


// hand written ocaml serialization function
value ocaml_from_unsigned_long(const unsigned long & ul, ToOcamlData *d) {
  value r = caml_copy_int32(ul);
  xassert(IS_OCAML_INT32(r));
  return r;
}

// hand written ocaml serialization function
value ocaml_from_double(const double &f, ToOcamlData *) {
  value r = caml_copy_double(f);
  xassert(IS_OCAML_FLOAT(r));
  return r;
}


// hand written ocaml serialization function
value ocaml_from_ScopeKind(const ScopeKind &id, ToOcamlData *d){
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_SK_UNKNOWN_constructor_closure = NULL;
  static value * create_SK_GLOBAL_constructor_closure = NULL;
  static value * create_SK_PARAMETER_constructor_closure = NULL;
  static value * create_SK_FUNCTION_constructor_closure = NULL;
  static value * create_SK_CLASS_constructor_closure = NULL;
  static value * create_SK_TEMPLATE_PARAMS_constructor_closure = NULL;
  static value * create_SK_TEMPLATE_ARGS_constructor_closure = NULL;
  static value * create_SK_NAMESPACE_constructor_closure = NULL;

  value result;

  switch(id){

  case SK_UNKNOWN:
    xassert(false);
    if(create_SK_UNKNOWN_constructor_closure == NULL)
      create_SK_UNKNOWN_constructor_closure = 
        caml_named_value("create_SK_UNKNOWN_constructor");
    xassert(create_SK_UNKNOWN_constructor_closure);
    result = caml_callback(*create_SK_UNKNOWN_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case SK_GLOBAL:
    if(create_SK_GLOBAL_constructor_closure == NULL)
      create_SK_GLOBAL_constructor_closure = 
        caml_named_value("create_SK_GLOBAL_constructor");
    xassert(create_SK_GLOBAL_constructor_closure);
    result = caml_callback(*create_SK_GLOBAL_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case SK_PARAMETER:
    if(create_SK_PARAMETER_constructor_closure == NULL)
      create_SK_PARAMETER_constructor_closure = 
        caml_named_value("create_SK_PARAMETER_constructor");
    xassert(create_SK_PARAMETER_constructor_closure);
    result = caml_callback(*create_SK_PARAMETER_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case SK_FUNCTION:
    if(create_SK_FUNCTION_constructor_closure == NULL)
      create_SK_FUNCTION_constructor_closure = 
        caml_named_value("create_SK_FUNCTION_constructor");
    xassert(create_SK_FUNCTION_constructor_closure);
    result = caml_callback(*create_SK_FUNCTION_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case SK_CLASS:
    if(create_SK_CLASS_constructor_closure == NULL)
      create_SK_CLASS_constructor_closure = 
        caml_named_value("create_SK_CLASS_constructor");
    xassert(create_SK_CLASS_constructor_closure);
    result = caml_callback(*create_SK_CLASS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case SK_TEMPLATE_PARAMS:
    if(create_SK_TEMPLATE_PARAMS_constructor_closure == NULL)
      create_SK_TEMPLATE_PARAMS_constructor_closure = 
        caml_named_value("create_SK_TEMPLATE_PARAMS_constructor");
    xassert(create_SK_TEMPLATE_PARAMS_constructor_closure);
    result = caml_callback(*create_SK_TEMPLATE_PARAMS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case SK_TEMPLATE_ARGS:
    if(create_SK_TEMPLATE_ARGS_constructor_closure == NULL)
      create_SK_TEMPLATE_ARGS_constructor_closure = 
        caml_named_value("create_SK_TEMPLATE_ARGS_constructor");
    xassert(create_SK_TEMPLATE_ARGS_constructor_closure);
    result = caml_callback(*create_SK_TEMPLATE_ARGS_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case SK_NAMESPACE:
    if(create_SK_NAMESPACE_constructor_closure == NULL)
      create_SK_NAMESPACE_constructor_closure = 
        caml_named_value("create_SK_NAMESPACE_constructor");
    xassert(create_SK_NAMESPACE_constructor_closure);
    result = caml_callback(*create_SK_NAMESPACE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}


// hand written ocaml serialization function
value ocaml_from_TemplateThingKind(const TemplateThingKind &id, 
				   ToOcamlData *d) {
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_TTK_PRIMARY_constructor_closure = NULL;
  static value * create_TTK_SPECIALIZATION_constructor_closure = NULL;
  static value * create_TTK_INSTANTIATION_constructor_closure = NULL;

  value result;

  switch(id){

  case TTK_PRIMARY:
    if(create_TTK_PRIMARY_constructor_closure == NULL)
      create_TTK_PRIMARY_constructor_closure = 
        caml_named_value("create_TTK_PRIMARY_constructor");
    xassert(create_TTK_PRIMARY_constructor_closure);
    result = caml_callback(*create_TTK_PRIMARY_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case TTK_SPECIALIZATION:
    if(create_TTK_SPECIALIZATION_constructor_closure == NULL)
      create_TTK_SPECIALIZATION_constructor_closure = 
        caml_named_value("create_TTK_SPECIALIZATION_constructor");
    xassert(create_TTK_SPECIALIZATION_constructor_closure);
    result = caml_callback(*create_TTK_SPECIALIZATION_constructor_closure, 
			   Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case TTK_INSTANTIATION:
    if(create_TTK_INSTANTIATION_constructor_closure == NULL)
      create_TTK_INSTANTIATION_constructor_closure = 
        caml_named_value("create_TTK_INSTANTIATION_constructor");
    xassert(create_TTK_INSTANTIATION_constructor_closure);
    result = caml_callback(*create_TTK_INSTANTIATION_constructor_closure, 
			   Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}
  

//**************************************************************************
//******************************* debug caml roots *************************

#ifdef DEBUG_CAML_GLOBAL_ROOTS

#undef caml_register_global_root
#undef caml_remove_global_root

static SObjSet<value const *> debug_caml_roots;
static int debug_caml_root_max;

// #include <fstream.h>       // ofstream
// static ofstream rootlog("rootlog");

void debug_caml_register_global_root (value * val) {
  // rootlog << "register root " << hex << val << dec << endl;
  if(debug_caml_roots.contains(val)) {
    cerr << "second registration of caml root " << hex << val << dec << endl;
    xassert(false);
  }
  debug_caml_roots.add(val);
  if(debug_caml_roots.size() > debug_caml_root_max)
    debug_caml_root_max = debug_caml_roots.size();
  caml_register_global_root(val);
}

void debug_caml_remove_global_root (value * val) {
  if(!debug_caml_roots.contains(val)) {
    cerr << "remove nonexisting caml root " << hex << val << dec << endl;
    xassert(false);
  }
  debug_caml_roots.remove(val);
  caml_remove_global_root(val);
}

void check_caml_root_status() {
  if(debug_caml_roots.size() == 0) 
    return;
  cerr << "active caml roots: " << debug_caml_roots.size()
       << " (max was " << debug_caml_root_max << " )\n";
  cerr << "active roots: " << hex;
  for(SObjSetIter<value const *> iter(debug_caml_roots); !iter.isDone();
      iter.adv()) {
    cerr << iter.data() << " ";
    cerr << dec << "\n";
  }
  xassert(false);
}
#endif // DEBUG_CAML_GLOBAL_ROOTS




/*

to create the bodies for the enums:
 - take the ocaml variant type def
 - delete the type name (only constructors remain)
 - strip constructor args off if any (these funs only treat pure enums)
 - apply enum-constructor
 - there should be just the constructors, each on their own line
 - apply declare-statics

(defun enum-constructors ()
  (interactive)
  (narrow-to-region (point) (mark))
  (beginning-of-buffer)
  (replace-regexp "(\\*.+" "")
  (end-of-buffer)
  (forward-line -1)
  (forward-char 4)
  (kill-rectangle (point-min) (point))
  (beginning-of-buffer)
  (replace-regexp " +" "")
  (beginning-of-buffer)
  (flush-lines "^$")
  (widen))

(defun declare-statics ()
  (interactive)
  (narrow-to-region (point) (mark))
  (beginning-of-buffer)
  (kill-ring-save (point-min) (point-max))
  (replace-regexp "\\(.*\\)" "  static value * create_\\1_constructor_closure = NULL;" nil nil nil)
  (insert "\n\n  value result;\n\n  switch(id){\n\n")
  (yank)
  (goto-char (mark))
  (replace-regexp "\\(.+\\)" "  case \\1:
    if(create_\\1_constructor_closure == NULL)
      create_\\1_constructor_closure = 
        caml_named_value(\"create_\\1_constructor\");
    xassert(create_\\1_constructor_closure);
    result = caml_callback(*create_\\1_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;
" )
  (widen))


*/
