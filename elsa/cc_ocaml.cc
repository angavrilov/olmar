// cc_flags.h            see license.txt for copyright and terms of use
// implementation of cc_ocaml.h -- ocaml serialization utility functions


#include "cc_ocaml.h"
extern "C" {
#include <caml/fail.h>
}
#include <iomanip.h>



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
value ref_None_constr(ToOcamlData * data){
  CAMLparam0();
  CAMLlocal1(result);
  result = caml_alloc(1, 0);  // the reference cell
  Store_field(result, 0, Val_None);
  xassert(IS_OCAML_AST_VALUE(result));
  CAMLreturn(result);
}


// hand written ocaml serialization function
// not relly a serialization function, but handwritten ;-)
void postpone_circular_type(ToOcamlData * data, value val, 
				unsigned field, CType * type) {

  // no need to register val as long as we don't allocate here

  CircularAstPart * part = new CircularAstPart;
  part->ca_type = CA_Type;
  part->ast.type = type;
  part->val = val;
  part->field = field;
  part->next = data->postponed_circles;
  data->postponed_circles = part;
  data->postponed_count++;
  cerr << "postpone (" << data->postponed_count
       << ") type " << type << " in field " << field
       << " of " << val << "\n";
}


// hand written ocaml serialization function
// not relly a serialization function, but handwritten ;-)
void finish_circular_pointers(ToOcamlData * data) {
  CAMLparam0();
  CAMLlocal3(val, some_val, cell);
  CircularAstPart * part;

  while(data->postponed_circles != NULL) {
    part = data->postponed_circles;
    data->postponed_circles = data->postponed_circles->next;
    cerr << "dispatch (" << data->postponed_count 
	 << ") in field " << part->field
	 << " of " << part->val;
    data->postponed_count--;

    xassert(data->stack.size() == 0);
  
    switch(part->ca_type) {
    case CA_Type:
      cerr << " (CType)\n";
      val = part->ast.type->toOcaml(data);
      break;

    case CA_Empty:
    default:
      xassert(false);
    }

    // check that the field in part->val is ref None
    xassert(Is_block(part->val));
    cell = Field(part->val, part->field);
    xassert(Is_block(cell) && 	// it's a block
	    (Tag_val(cell) == 0) && // it's an array/tuple/record
	    (Wosize_val(cell) == 1) && // with one cell
	    Is_long(Field(cell, 0)) && // containing None
	    (Val_long(Field(cell, 0)) == 0));

    // construct Some val
    some_val = option_some_constr(val);
    // assign into reference cell
    Store_field(cell, 0, some_val);

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
  // don't allocate here, so don;t need the CAMLparam stuff

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
  // don't allocate here, so don;t need the CAMLparam stuff

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
  // don't allocate here, so don;t need the CAMLparam stuff

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
  // don't allocate here, so don;t need the CAMLparam stuff

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
  // don't allocate here, so don;t need the CAMLparam stuff

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
  // don't allocate here, so don;t need the CAMLparam stuff

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
  // don't allocate here, so don;t need the CAMLparam stuff

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
  // don't allocate here, so don;t need the CAMLparam stuff

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
  // don't allocate here, so don;t need the CAMLparam stuff

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





/*

to create the bodies for the enums:

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
  (insert "\n\n  switch(id){\n\n")
  (yank)
  (goto-char (mark))
  (replace-regexp "\\(.+\\)" "  case \\1:
    if(create_\\1_constructor_closure == NULL)
      create_\\1_constructor_closure = 
        caml_named_value(\"create_\\1_constructor\");
    xassert(create_\\1_constructor_closure);
    return caml_callback(*create_\\1_constructor_closure, Val_unit);
" )
  (widen))


*/
