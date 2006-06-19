// cc_flags.h            see license.txt for copyright and terms of use
// implementation of cc_ocaml.h -- ocaml serialization utility functions


#include "cc_ocaml.h"


// hand written ocaml serialization function
value ocaml_from_SourceLoc(const SourceLoc &loc, ToOcamlData *d){
  CAMLparam0();
  CAMLlocal2(val_s, result);
  
  static value * create_SourceLoc_tuple_closure = NULL;
  if(create_SourceLoc_tuple_closure == NULL)
    create_SourceLoc_tuple_closure = caml_named_value("create_SourceLoc_tuple");
  xassert(create_SourceLoc_tuple_closure);

  char const *name;
  int line, col;
  sourceLocManager->decodeLineCol(loc, name, line, col);

  val_s = caml_copy_string(name);
  result = caml_callback3(*create_SourceLoc_tuple_closure, 
			  val_s, Val_int(line), Val_int(col));

  CAMLreturn(result);
}



// hand written ocaml serialization function
value ocaml_from_DeclFlags(const DeclFlags &f, ToOcamlData *d){
  CAMLparam0();
  CAMLlocal1(result);

  static value * declFlag_from_int32_closure = NULL;
  if(declFlag_from_int32_closure == NULL)
    declFlag_from_int32_closure = caml_named_value("declFlag_from_int32");
  xassert(declFlag_from_int32_closure);

  result = caml_callback(*declFlag_from_int32_closure, caml_copy_int32(f));

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

  switch(id){

  case ST_CHAR:
    if(create_ST_CHAR_constructor_closure == NULL)
      create_ST_CHAR_constructor_closure = 
        caml_named_value("create_ST_CHAR_constructor");
    xassert(create_ST_CHAR_constructor_closure);
    return caml_callback(*create_ST_CHAR_constructor_closure, Val_unit);

  case ST_UNSIGNED_CHAR:
    if(create_ST_UNSIGNED_CHAR_constructor_closure == NULL)
      create_ST_UNSIGNED_CHAR_constructor_closure = 
        caml_named_value("create_ST_UNSIGNED_CHAR_constructor");
    xassert(create_ST_UNSIGNED_CHAR_constructor_closure);
    return caml_callback(*create_ST_UNSIGNED_CHAR_constructor_closure, 
			 Val_unit);

  case ST_SIGNED_CHAR:
    if(create_ST_SIGNED_CHAR_constructor_closure == NULL)
      create_ST_SIGNED_CHAR_constructor_closure = 
        caml_named_value("create_ST_SIGNED_CHAR_constructor");
    xassert(create_ST_SIGNED_CHAR_constructor_closure);
    return caml_callback(*create_ST_SIGNED_CHAR_constructor_closure, Val_unit);

  case ST_BOOL:
    if(create_ST_BOOL_constructor_closure == NULL)
      create_ST_BOOL_constructor_closure = 
        caml_named_value("create_ST_BOOL_constructor");
    xassert(create_ST_BOOL_constructor_closure);
    return caml_callback(*create_ST_BOOL_constructor_closure, Val_unit);

  case ST_INT:
    if(create_ST_INT_constructor_closure == NULL)
      create_ST_INT_constructor_closure = 
        caml_named_value("create_ST_INT_constructor");
    xassert(create_ST_INT_constructor_closure);
    return caml_callback(*create_ST_INT_constructor_closure, Val_unit);

  case ST_UNSIGNED_INT:
    if(create_ST_UNSIGNED_INT_constructor_closure == NULL)
      create_ST_UNSIGNED_INT_constructor_closure = 
        caml_named_value("create_ST_UNSIGNED_INT_constructor");
    xassert(create_ST_UNSIGNED_INT_constructor_closure);
    return caml_callback(*create_ST_UNSIGNED_INT_constructor_closure, Val_unit);

  case ST_LONG_INT:
    if(create_ST_LONG_INT_constructor_closure == NULL)
      create_ST_LONG_INT_constructor_closure = 
        caml_named_value("create_ST_LONG_INT_constructor");
    xassert(create_ST_LONG_INT_constructor_closure);
    return caml_callback(*create_ST_LONG_INT_constructor_closure, Val_unit);

  case ST_UNSIGNED_LONG_INT:
    if(create_ST_UNSIGNED_LONG_INT_constructor_closure == NULL)
      create_ST_UNSIGNED_LONG_INT_constructor_closure = 
        caml_named_value("create_ST_UNSIGNED_LONG_INT_constructor");
    xassert(create_ST_UNSIGNED_LONG_INT_constructor_closure);
    return caml_callback(*create_ST_UNSIGNED_LONG_INT_constructor_closure, 
			 Val_unit);

  case ST_LONG_LONG:
    if(create_ST_LONG_LONG_constructor_closure == NULL)
      create_ST_LONG_LONG_constructor_closure = 
        caml_named_value("create_ST_LONG_LONG_constructor");
    xassert(create_ST_LONG_LONG_constructor_closure);
    return caml_callback(*create_ST_LONG_LONG_constructor_closure, Val_unit);

  case ST_UNSIGNED_LONG_LONG:
    if(create_ST_UNSIGNED_LONG_LONG_constructor_closure == NULL)
      create_ST_UNSIGNED_LONG_LONG_constructor_closure = 
        caml_named_value("create_ST_UNSIGNED_LONG_LONG_constructor");
    xassert(create_ST_UNSIGNED_LONG_LONG_constructor_closure);
    return caml_callback(*create_ST_UNSIGNED_LONG_LONG_constructor_closure, 
			 Val_unit);

  case ST_SHORT_INT:
    if(create_ST_SHORT_INT_constructor_closure == NULL)
      create_ST_SHORT_INT_constructor_closure = 
        caml_named_value("create_ST_SHORT_INT_constructor");
    xassert(create_ST_SHORT_INT_constructor_closure);
    return caml_callback(*create_ST_SHORT_INT_constructor_closure, Val_unit);

  case ST_UNSIGNED_SHORT_INT:
    if(create_ST_UNSIGNED_SHORT_INT_constructor_closure == NULL)
      create_ST_UNSIGNED_SHORT_INT_constructor_closure = 
        caml_named_value("create_ST_UNSIGNED_SHORT_INT_constructor");
    xassert(create_ST_UNSIGNED_SHORT_INT_constructor_closure);
    return caml_callback(*create_ST_UNSIGNED_SHORT_INT_constructor_closure, 
			 Val_unit);

  case ST_WCHAR_T:
    if(create_ST_WCHAR_T_constructor_closure == NULL)
      create_ST_WCHAR_T_constructor_closure = 
        caml_named_value("create_ST_WCHAR_T_constructor");
    xassert(create_ST_WCHAR_T_constructor_closure);
    return caml_callback(*create_ST_WCHAR_T_constructor_closure, Val_unit);

  case ST_FLOAT:
    if(create_ST_FLOAT_constructor_closure == NULL)
      create_ST_FLOAT_constructor_closure = 
        caml_named_value("create_ST_FLOAT_constructor");
    xassert(create_ST_FLOAT_constructor_closure);
    return caml_callback(*create_ST_FLOAT_constructor_closure, Val_unit);

  case ST_DOUBLE:
    if(create_ST_DOUBLE_constructor_closure == NULL)
      create_ST_DOUBLE_constructor_closure = 
        caml_named_value("create_ST_DOUBLE_constructor");
    xassert(create_ST_DOUBLE_constructor_closure);
    return caml_callback(*create_ST_DOUBLE_constructor_closure, Val_unit);

  case ST_LONG_DOUBLE:
    if(create_ST_LONG_DOUBLE_constructor_closure == NULL)
      create_ST_LONG_DOUBLE_constructor_closure = 
        caml_named_value("create_ST_LONG_DOUBLE_constructor");
    xassert(create_ST_LONG_DOUBLE_constructor_closure);
    return caml_callback(*create_ST_LONG_DOUBLE_constructor_closure, Val_unit);

  case ST_FLOAT_COMPLEX:
    if(create_ST_FLOAT_COMPLEX_constructor_closure == NULL)
      create_ST_FLOAT_COMPLEX_constructor_closure = 
        caml_named_value("create_ST_FLOAT_COMPLEX_constructor");
    xassert(create_ST_FLOAT_COMPLEX_constructor_closure);
    return caml_callback(*create_ST_FLOAT_COMPLEX_constructor_closure, 
			 Val_unit);

  case ST_DOUBLE_COMPLEX:
    if(create_ST_DOUBLE_COMPLEX_constructor_closure == NULL)
      create_ST_DOUBLE_COMPLEX_constructor_closure = 
        caml_named_value("create_ST_DOUBLE_COMPLEX_constructor");
    xassert(create_ST_DOUBLE_COMPLEX_constructor_closure);
    return caml_callback(*create_ST_DOUBLE_COMPLEX_constructor_closure, 
			 Val_unit);

  case ST_LONG_DOUBLE_COMPLEX:
    if(create_ST_LONG_DOUBLE_COMPLEX_constructor_closure == NULL)
      create_ST_LONG_DOUBLE_COMPLEX_constructor_closure = 
        caml_named_value("create_ST_LONG_DOUBLE_COMPLEX_constructor");
    xassert(create_ST_LONG_DOUBLE_COMPLEX_constructor_closure);
    return caml_callback(*create_ST_LONG_DOUBLE_COMPLEX_constructor_closure, 
			 Val_unit);

  case ST_FLOAT_IMAGINARY:
    if(create_ST_FLOAT_IMAGINARY_constructor_closure == NULL)
      create_ST_FLOAT_IMAGINARY_constructor_closure = 
        caml_named_value("create_ST_FLOAT_IMAGINARY_constructor");
    xassert(create_ST_FLOAT_IMAGINARY_constructor_closure);
    return caml_callback(*create_ST_FLOAT_IMAGINARY_constructor_closure, 
			 Val_unit);

  case ST_DOUBLE_IMAGINARY:
    if(create_ST_DOUBLE_IMAGINARY_constructor_closure == NULL)
      create_ST_DOUBLE_IMAGINARY_constructor_closure = 
        caml_named_value("create_ST_DOUBLE_IMAGINARY_constructor");
    xassert(create_ST_DOUBLE_IMAGINARY_constructor_closure);
    return caml_callback(*create_ST_DOUBLE_IMAGINARY_constructor_closure, 
			 Val_unit);

  case ST_LONG_DOUBLE_IMAGINARY:
    if(create_ST_LONG_DOUBLE_IMAGINARY_constructor_closure == NULL)
      create_ST_LONG_DOUBLE_IMAGINARY_constructor_closure = 
        caml_named_value("create_ST_LONG_DOUBLE_IMAGINARY_constructor");
    xassert(create_ST_LONG_DOUBLE_IMAGINARY_constructor_closure);
    return caml_callback(*create_ST_LONG_DOUBLE_IMAGINARY_constructor_closure, 
			 Val_unit);

  case ST_VOID:
    if(create_ST_VOID_constructor_closure == NULL)
      create_ST_VOID_constructor_closure = 
        caml_named_value("create_ST_VOID_constructor");
    xassert(create_ST_VOID_constructor_closure);
    return caml_callback(*create_ST_VOID_constructor_closure, Val_unit);

  case ST_ELLIPSIS:
    if(create_ST_ELLIPSIS_constructor_closure == NULL)
      create_ST_ELLIPSIS_constructor_closure = 
        caml_named_value("create_ST_ELLIPSIS_constructor");
    xassert(create_ST_ELLIPSIS_constructor_closure);
    return caml_callback(*create_ST_ELLIPSIS_constructor_closure, Val_unit);

  case ST_CDTOR:
    if(create_ST_CDTOR_constructor_closure == NULL)
      create_ST_CDTOR_constructor_closure = 
        caml_named_value("create_ST_CDTOR_constructor");
    xassert(create_ST_CDTOR_constructor_closure);
    return caml_callback(*create_ST_CDTOR_constructor_closure, Val_unit);

  case ST_ERROR:
    if(create_ST_ERROR_constructor_closure == NULL)
      create_ST_ERROR_constructor_closure = 
        caml_named_value("create_ST_ERROR_constructor");
    xassert(create_ST_ERROR_constructor_closure);
    return caml_callback(*create_ST_ERROR_constructor_closure, Val_unit);

  case ST_DEPENDENT:
    if(create_ST_DEPENDENT_constructor_closure == NULL)
      create_ST_DEPENDENT_constructor_closure = 
        caml_named_value("create_ST_DEPENDENT_constructor");
    xassert(create_ST_DEPENDENT_constructor_closure);
    return caml_callback(*create_ST_DEPENDENT_constructor_closure, Val_unit);

  case ST_IMPLINT:
    if(create_ST_IMPLINT_constructor_closure == NULL)
      create_ST_IMPLINT_constructor_closure = 
        caml_named_value("create_ST_IMPLINT_constructor");
    xassert(create_ST_IMPLINT_constructor_closure);
    return caml_callback(*create_ST_IMPLINT_constructor_closure, Val_unit);

  case ST_NOTFOUND:
    if(create_ST_NOTFOUND_constructor_closure == NULL)
      create_ST_NOTFOUND_constructor_closure = 
        caml_named_value("create_ST_NOTFOUND_constructor");
    xassert(create_ST_NOTFOUND_constructor_closure);
    return caml_callback(*create_ST_NOTFOUND_constructor_closure, Val_unit);

  case ST_PROMOTED_INTEGRAL:
    if(create_ST_PROMOTED_INTEGRAL_constructor_closure == NULL)
      create_ST_PROMOTED_INTEGRAL_constructor_closure = 
        caml_named_value("create_ST_PROMOTED_INTEGRAL_constructor");
    xassert(create_ST_PROMOTED_INTEGRAL_constructor_closure);
    return caml_callback(*create_ST_PROMOTED_INTEGRAL_constructor_closure, 
			 Val_unit);

  case ST_PROMOTED_ARITHMETIC:
    if(create_ST_PROMOTED_ARITHMETIC_constructor_closure == NULL)
      create_ST_PROMOTED_ARITHMETIC_constructor_closure = 
        caml_named_value("create_ST_PROMOTED_ARITHMETIC_constructor");
    xassert(create_ST_PROMOTED_ARITHMETIC_constructor_closure);
    return caml_callback(*create_ST_PROMOTED_ARITHMETIC_constructor_closure, 
			 Val_unit);

  case ST_INTEGRAL:
    if(create_ST_INTEGRAL_constructor_closure == NULL)
      create_ST_INTEGRAL_constructor_closure = 
        caml_named_value("create_ST_INTEGRAL_constructor");
    xassert(create_ST_INTEGRAL_constructor_closure);
    return caml_callback(*create_ST_INTEGRAL_constructor_closure, Val_unit);

  case ST_ARITHMETIC:
    if(create_ST_ARITHMETIC_constructor_closure == NULL)
      create_ST_ARITHMETIC_constructor_closure = 
        caml_named_value("create_ST_ARITHMETIC_constructor");
    xassert(create_ST_ARITHMETIC_constructor_closure);
    return caml_callback(*create_ST_ARITHMETIC_constructor_closure, Val_unit);

  case ST_ARITHMETIC_NON_BOOL:
    if(create_ST_ARITHMETIC_NON_BOOL_constructor_closure == NULL)
      create_ST_ARITHMETIC_NON_BOOL_constructor_closure = 
        caml_named_value("create_ST_ARITHMETIC_NON_BOOL_constructor");
    xassert(create_ST_ARITHMETIC_NON_BOOL_constructor_closure);
    return caml_callback(*create_ST_ARITHMETIC_NON_BOOL_constructor_closure, 
			 Val_unit);

  case ST_ANY_OBJ_TYPE:
    if(create_ST_ANY_OBJ_TYPE_constructor_closure == NULL)
      create_ST_ANY_OBJ_TYPE_constructor_closure = 
        caml_named_value("create_ST_ANY_OBJ_TYPE_constructor");
    xassert(create_ST_ANY_OBJ_TYPE_constructor_closure);
    return caml_callback(*create_ST_ANY_OBJ_TYPE_constructor_closure, Val_unit);

  case ST_ANY_NON_VOID:
    if(create_ST_ANY_NON_VOID_constructor_closure == NULL)
      create_ST_ANY_NON_VOID_constructor_closure = 
        caml_named_value("create_ST_ANY_NON_VOID_constructor");
    xassert(create_ST_ANY_NON_VOID_constructor_closure);
    return caml_callback(*create_ST_ANY_NON_VOID_constructor_closure, Val_unit);

  case ST_ANY_TYPE:
    if(create_ST_ANY_TYPE_constructor_closure == NULL)
      create_ST_ANY_TYPE_constructor_closure = 
        caml_named_value("create_ST_ANY_TYPE_constructor");
    xassert(create_ST_ANY_TYPE_constructor_closure);
    return caml_callback(*create_ST_ANY_TYPE_constructor_closure, Val_unit);

  case ST_PRET_STRIP_REF:
    if(create_ST_PRET_STRIP_REF_constructor_closure == NULL)
      create_ST_PRET_STRIP_REF_constructor_closure = 
        caml_named_value("create_ST_PRET_STRIP_REF_constructor");
    xassert(create_ST_PRET_STRIP_REF_constructor_closure);
    return caml_callback(*create_ST_PRET_STRIP_REF_constructor_closure, 
			 Val_unit);

  case ST_PRET_PTM:
    if(create_ST_PRET_PTM_constructor_closure == NULL)
      create_ST_PRET_PTM_constructor_closure = 
        caml_named_value("create_ST_PRET_PTM_constructor");
    xassert(create_ST_PRET_PTM_constructor_closure);
    return caml_callback(*create_ST_PRET_PTM_constructor_closure, Val_unit);

  case ST_PRET_ARITH_CONV:
    if(create_ST_PRET_ARITH_CONV_constructor_closure == NULL)
      create_ST_PRET_ARITH_CONV_constructor_closure = 
        caml_named_value("create_ST_PRET_ARITH_CONV_constructor");
    xassert(create_ST_PRET_ARITH_CONV_constructor_closure);
    return caml_callback(*create_ST_PRET_ARITH_CONV_constructor_closure, 
			 Val_unit);

  case ST_PRET_FIRST:
    if(create_ST_PRET_FIRST_constructor_closure == NULL)
      create_ST_PRET_FIRST_constructor_closure = 
        caml_named_value("create_ST_PRET_FIRST_constructor");
    xassert(create_ST_PRET_FIRST_constructor_closure);
    return caml_callback(*create_ST_PRET_FIRST_constructor_closure, Val_unit);

  case ST_PRET_FIRST_PTR2REF:
    if(create_ST_PRET_FIRST_PTR2REF_constructor_closure == NULL)
      create_ST_PRET_FIRST_PTR2REF_constructor_closure = 
        caml_named_value("create_ST_PRET_FIRST_PTR2REF_constructor");
    xassert(create_ST_PRET_FIRST_PTR2REF_constructor_closure);
    return caml_callback(*create_ST_PRET_FIRST_PTR2REF_constructor_closure, 
			 Val_unit);

  case ST_PRET_SECOND:
    if(create_ST_PRET_SECOND_constructor_closure == NULL)
      create_ST_PRET_SECOND_constructor_closure = 
        caml_named_value("create_ST_PRET_SECOND_constructor");
    xassert(create_ST_PRET_SECOND_constructor_closure);
    return caml_callback(*create_ST_PRET_SECOND_constructor_closure, Val_unit);

  case ST_PRET_SECOND_PTR2REF:
    if(create_ST_PRET_SECOND_PTR2REF_constructor_closure == NULL)
      create_ST_PRET_SECOND_PTR2REF_constructor_closure = 
        caml_named_value("create_ST_PRET_SECOND_PTR2REF_constructor");
    xassert(create_ST_PRET_SECOND_PTR2REF_constructor_closure);
    return caml_callback(*create_ST_PRET_SECOND_PTR2REF_constructor_closure, 
			 Val_unit);

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us before
  return(Val_unit);
}


// hand written ocaml serialization function
value ocaml_from_CVFlags(const CVFlags &f, ToOcamlData *d){
  CAMLparam0();
  CAMLlocal1(result);

  static value * cVFlag_from_int32_closure = NULL;
  if(cVFlag_from_int32_closure == NULL)
    cVFlag_from_int32_closure = caml_named_value("cVFlag_from_int32");
  xassert(cVFlag_from_int32_closure);

  result = caml_callback(*cVFlag_from_int32_closure, caml_copy_int32(f));

  CAMLreturn(result);
}




// hand written ocaml serialization function
value ocaml_from_TypeIntr(const TypeIntr &id, ToOcamlData *d){
  // don't allocate here, so don;t need the CAMLparam stuff

  static value * create_TI_STRUCT_constructor_closure = NULL;
  static value * create_TI_CLASS_constructor_closure = NULL;
  static value * create_TI_UNION_constructor_closure = NULL;
  static value * create_TI_ENUM_constructor_closure = NULL;

  switch(id){

  case TI_STRUCT:
    if(create_TI_STRUCT_constructor_closure == NULL)
      create_TI_STRUCT_constructor_closure = 
        caml_named_value("create_TI_STRUCT_constructor");
    xassert(create_TI_STRUCT_constructor_closure);
    return caml_callback(*create_TI_STRUCT_constructor_closure, Val_unit);

  case TI_CLASS:
    if(create_TI_CLASS_constructor_closure == NULL)
      create_TI_CLASS_constructor_closure = 
        caml_named_value("create_TI_CLASS_constructor");
    xassert(create_TI_CLASS_constructor_closure);
    return caml_callback(*create_TI_CLASS_constructor_closure, Val_unit);

  case TI_UNION:
    if(create_TI_UNION_constructor_closure == NULL)
      create_TI_UNION_constructor_closure = 
        caml_named_value("create_TI_UNION_constructor");
    xassert(create_TI_UNION_constructor_closure);
    return caml_callback(*create_TI_UNION_constructor_closure, Val_unit);

  case TI_ENUM:
    if(create_TI_ENUM_constructor_closure == NULL)
      create_TI_ENUM_constructor_closure = 
        caml_named_value("create_TI_ENUM_constructor");
    xassert(create_TI_ENUM_constructor_closure);
    return caml_callback(*create_TI_ENUM_constructor_closure, Val_unit);

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us before
  return(Val_unit);
}



// hand written ocaml serialization function
value ocaml_from_AccessKeyword(const AccessKeyword &id, ToOcamlData *d){
  // don't allocate here, so don;t need the CAMLparam stuff

  static value * create_AK_PUBLIC_constructor_closure = NULL;
  static value * create_AK_PROTECTED_constructor_closure = NULL;
  static value * create_AK_PRIVATE_constructor_closure = NULL;
  static value * create_AK_UNSPECIFIED_constructor_closure = NULL;

  switch(id){

  case AK_PUBLIC:
    if(create_AK_PUBLIC_constructor_closure == NULL)
      create_AK_PUBLIC_constructor_closure = 
        caml_named_value("create_AK_PUBLIC_constructor");
    xassert(create_AK_PUBLIC_constructor_closure);
    return caml_callback(*create_AK_PUBLIC_constructor_closure, Val_unit);

  case AK_PROTECTED:
    if(create_AK_PROTECTED_constructor_closure == NULL)
      create_AK_PROTECTED_constructor_closure = 
        caml_named_value("create_AK_PROTECTED_constructor");
    xassert(create_AK_PROTECTED_constructor_closure);
    return caml_callback(*create_AK_PROTECTED_constructor_closure, Val_unit);

  case AK_PRIVATE:
    if(create_AK_PRIVATE_constructor_closure == NULL)
      create_AK_PRIVATE_constructor_closure = 
        caml_named_value("create_AK_PRIVATE_constructor");
    xassert(create_AK_PRIVATE_constructor_closure);
    return caml_callback(*create_AK_PRIVATE_constructor_closure, Val_unit);

  case AK_UNSPECIFIED:
    if(create_AK_UNSPECIFIED_constructor_closure == NULL)
      create_AK_UNSPECIFIED_constructor_closure = 
        caml_named_value("create_AK_UNSPECIFIED_constructor");
    xassert(create_AK_UNSPECIFIED_constructor_closure);
    return caml_callback(*create_AK_UNSPECIFIED_constructor_closure, Val_unit);

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us before
  return(Val_unit);
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

  switch(id){

  case OP_NOT:
    if(create_OP_NOT_constructor_closure == NULL)
      create_OP_NOT_constructor_closure = 
        caml_named_value("create_OP_NOT_constructor");
    xassert(create_OP_NOT_constructor_closure);
    return caml_callback(*create_OP_NOT_constructor_closure, Val_unit);

  case OP_BITNOT:
    if(create_OP_BITNOT_constructor_closure == NULL)
      create_OP_BITNOT_constructor_closure = 
        caml_named_value("create_OP_BITNOT_constructor");
    xassert(create_OP_BITNOT_constructor_closure);
    return caml_callback(*create_OP_BITNOT_constructor_closure, Val_unit);

  case OP_PLUSPLUS:
    if(create_OP_PLUSPLUS_constructor_closure == NULL)
      create_OP_PLUSPLUS_constructor_closure = 
        caml_named_value("create_OP_PLUSPLUS_constructor");
    xassert(create_OP_PLUSPLUS_constructor_closure);
    return caml_callback(*create_OP_PLUSPLUS_constructor_closure, Val_unit);

  case OP_MINUSMINUS:
    if(create_OP_MINUSMINUS_constructor_closure == NULL)
      create_OP_MINUSMINUS_constructor_closure = 
        caml_named_value("create_OP_MINUSMINUS_constructor");
    xassert(create_OP_MINUSMINUS_constructor_closure);
    return caml_callback(*create_OP_MINUSMINUS_constructor_closure, Val_unit);

  case OP_PLUS:
    if(create_OP_PLUS_constructor_closure == NULL)
      create_OP_PLUS_constructor_closure = 
        caml_named_value("create_OP_PLUS_constructor");
    xassert(create_OP_PLUS_constructor_closure);
    return caml_callback(*create_OP_PLUS_constructor_closure, Val_unit);

  case OP_MINUS:
    if(create_OP_MINUS_constructor_closure == NULL)
      create_OP_MINUS_constructor_closure = 
        caml_named_value("create_OP_MINUS_constructor");
    xassert(create_OP_MINUS_constructor_closure);
    return caml_callback(*create_OP_MINUS_constructor_closure, Val_unit);

  case OP_STAR:
    if(create_OP_STAR_constructor_closure == NULL)
      create_OP_STAR_constructor_closure = 
        caml_named_value("create_OP_STAR_constructor");
    xassert(create_OP_STAR_constructor_closure);
    return caml_callback(*create_OP_STAR_constructor_closure, Val_unit);

  case OP_AMPERSAND:
    if(create_OP_AMPERSAND_constructor_closure == NULL)
      create_OP_AMPERSAND_constructor_closure = 
        caml_named_value("create_OP_AMPERSAND_constructor");
    xassert(create_OP_AMPERSAND_constructor_closure);
    return caml_callback(*create_OP_AMPERSAND_constructor_closure, Val_unit);

  case OP_DIV:
    if(create_OP_DIV_constructor_closure == NULL)
      create_OP_DIV_constructor_closure = 
        caml_named_value("create_OP_DIV_constructor");
    xassert(create_OP_DIV_constructor_closure);
    return caml_callback(*create_OP_DIV_constructor_closure, Val_unit);

  case OP_MOD:
    if(create_OP_MOD_constructor_closure == NULL)
      create_OP_MOD_constructor_closure = 
        caml_named_value("create_OP_MOD_constructor");
    xassert(create_OP_MOD_constructor_closure);
    return caml_callback(*create_OP_MOD_constructor_closure, Val_unit);

  case OP_LSHIFT:
    if(create_OP_LSHIFT_constructor_closure == NULL)
      create_OP_LSHIFT_constructor_closure = 
        caml_named_value("create_OP_LSHIFT_constructor");
    xassert(create_OP_LSHIFT_constructor_closure);
    return caml_callback(*create_OP_LSHIFT_constructor_closure, Val_unit);

  case OP_RSHIFT:
    if(create_OP_RSHIFT_constructor_closure == NULL)
      create_OP_RSHIFT_constructor_closure = 
        caml_named_value("create_OP_RSHIFT_constructor");
    xassert(create_OP_RSHIFT_constructor_closure);
    return caml_callback(*create_OP_RSHIFT_constructor_closure, Val_unit);

  case OP_BITXOR:
    if(create_OP_BITXOR_constructor_closure == NULL)
      create_OP_BITXOR_constructor_closure = 
        caml_named_value("create_OP_BITXOR_constructor");
    xassert(create_OP_BITXOR_constructor_closure);
    return caml_callback(*create_OP_BITXOR_constructor_closure, Val_unit);

  case OP_BITOR:
    if(create_OP_BITOR_constructor_closure == NULL)
      create_OP_BITOR_constructor_closure = 
        caml_named_value("create_OP_BITOR_constructor");
    xassert(create_OP_BITOR_constructor_closure);
    return caml_callback(*create_OP_BITOR_constructor_closure, Val_unit);

  case OP_ASSIGN:
    if(create_OP_ASSIGN_constructor_closure == NULL)
      create_OP_ASSIGN_constructor_closure = 
        caml_named_value("create_OP_ASSIGN_constructor");
    xassert(create_OP_ASSIGN_constructor_closure);
    return caml_callback(*create_OP_ASSIGN_constructor_closure, Val_unit);

  case OP_PLUSEQ:
    if(create_OP_PLUSEQ_constructor_closure == NULL)
      create_OP_PLUSEQ_constructor_closure = 
        caml_named_value("create_OP_PLUSEQ_constructor");
    xassert(create_OP_PLUSEQ_constructor_closure);
    return caml_callback(*create_OP_PLUSEQ_constructor_closure, Val_unit);

  case OP_MINUSEQ:
    if(create_OP_MINUSEQ_constructor_closure == NULL)
      create_OP_MINUSEQ_constructor_closure = 
        caml_named_value("create_OP_MINUSEQ_constructor");
    xassert(create_OP_MINUSEQ_constructor_closure);
    return caml_callback(*create_OP_MINUSEQ_constructor_closure, Val_unit);

  case OP_MULTEQ:
    if(create_OP_MULTEQ_constructor_closure == NULL)
      create_OP_MULTEQ_constructor_closure = 
        caml_named_value("create_OP_MULTEQ_constructor");
    xassert(create_OP_MULTEQ_constructor_closure);
    return caml_callback(*create_OP_MULTEQ_constructor_closure, Val_unit);

  case OP_DIVEQ:
    if(create_OP_DIVEQ_constructor_closure == NULL)
      create_OP_DIVEQ_constructor_closure = 
        caml_named_value("create_OP_DIVEQ_constructor");
    xassert(create_OP_DIVEQ_constructor_closure);
    return caml_callback(*create_OP_DIVEQ_constructor_closure, Val_unit);

  case OP_MODEQ:
    if(create_OP_MODEQ_constructor_closure == NULL)
      create_OP_MODEQ_constructor_closure = 
        caml_named_value("create_OP_MODEQ_constructor");
    xassert(create_OP_MODEQ_constructor_closure);
    return caml_callback(*create_OP_MODEQ_constructor_closure, Val_unit);

  case OP_LSHIFTEQ:
    if(create_OP_LSHIFTEQ_constructor_closure == NULL)
      create_OP_LSHIFTEQ_constructor_closure = 
        caml_named_value("create_OP_LSHIFTEQ_constructor");
    xassert(create_OP_LSHIFTEQ_constructor_closure);
    return caml_callback(*create_OP_LSHIFTEQ_constructor_closure, Val_unit);

  case OP_RSHIFTEQ:
    if(create_OP_RSHIFTEQ_constructor_closure == NULL)
      create_OP_RSHIFTEQ_constructor_closure = 
        caml_named_value("create_OP_RSHIFTEQ_constructor");
    xassert(create_OP_RSHIFTEQ_constructor_closure);
    return caml_callback(*create_OP_RSHIFTEQ_constructor_closure, Val_unit);

  case OP_BITANDEQ:
    if(create_OP_BITANDEQ_constructor_closure == NULL)
      create_OP_BITANDEQ_constructor_closure = 
        caml_named_value("create_OP_BITANDEQ_constructor");
    xassert(create_OP_BITANDEQ_constructor_closure);
    return caml_callback(*create_OP_BITANDEQ_constructor_closure, Val_unit);

  case OP_BITXOREQ:
    if(create_OP_BITXOREQ_constructor_closure == NULL)
      create_OP_BITXOREQ_constructor_closure = 
        caml_named_value("create_OP_BITXOREQ_constructor");
    xassert(create_OP_BITXOREQ_constructor_closure);
    return caml_callback(*create_OP_BITXOREQ_constructor_closure, Val_unit);

  case OP_BITOREQ:
    if(create_OP_BITOREQ_constructor_closure == NULL)
      create_OP_BITOREQ_constructor_closure = 
        caml_named_value("create_OP_BITOREQ_constructor");
    xassert(create_OP_BITOREQ_constructor_closure);
    return caml_callback(*create_OP_BITOREQ_constructor_closure, Val_unit);

  case OP_EQUAL:
    if(create_OP_EQUAL_constructor_closure == NULL)
      create_OP_EQUAL_constructor_closure = 
        caml_named_value("create_OP_EQUAL_constructor");
    xassert(create_OP_EQUAL_constructor_closure);
    return caml_callback(*create_OP_EQUAL_constructor_closure, Val_unit);

  case OP_NOTEQUAL:
    if(create_OP_NOTEQUAL_constructor_closure == NULL)
      create_OP_NOTEQUAL_constructor_closure = 
        caml_named_value("create_OP_NOTEQUAL_constructor");
    xassert(create_OP_NOTEQUAL_constructor_closure);
    return caml_callback(*create_OP_NOTEQUAL_constructor_closure, Val_unit);

  case OP_LESS:
    if(create_OP_LESS_constructor_closure == NULL)
      create_OP_LESS_constructor_closure = 
        caml_named_value("create_OP_LESS_constructor");
    xassert(create_OP_LESS_constructor_closure);
    return caml_callback(*create_OP_LESS_constructor_closure, Val_unit);

  case OP_GREATER:
    if(create_OP_GREATER_constructor_closure == NULL)
      create_OP_GREATER_constructor_closure = 
        caml_named_value("create_OP_GREATER_constructor");
    xassert(create_OP_GREATER_constructor_closure);
    return caml_callback(*create_OP_GREATER_constructor_closure, Val_unit);

  case OP_LESSEQ:
    if(create_OP_LESSEQ_constructor_closure == NULL)
      create_OP_LESSEQ_constructor_closure = 
        caml_named_value("create_OP_LESSEQ_constructor");
    xassert(create_OP_LESSEQ_constructor_closure);
    return caml_callback(*create_OP_LESSEQ_constructor_closure, Val_unit);

  case OP_GREATEREQ:
    if(create_OP_GREATEREQ_constructor_closure == NULL)
      create_OP_GREATEREQ_constructor_closure = 
        caml_named_value("create_OP_GREATEREQ_constructor");
    xassert(create_OP_GREATEREQ_constructor_closure);
    return caml_callback(*create_OP_GREATEREQ_constructor_closure, Val_unit);

  case OP_AND:
    if(create_OP_AND_constructor_closure == NULL)
      create_OP_AND_constructor_closure = 
        caml_named_value("create_OP_AND_constructor");
    xassert(create_OP_AND_constructor_closure);
    return caml_callback(*create_OP_AND_constructor_closure, Val_unit);

  case OP_OR:
    if(create_OP_OR_constructor_closure == NULL)
      create_OP_OR_constructor_closure = 
        caml_named_value("create_OP_OR_constructor");
    xassert(create_OP_OR_constructor_closure);
    return caml_callback(*create_OP_OR_constructor_closure, Val_unit);

  case OP_ARROW:
    if(create_OP_ARROW_constructor_closure == NULL)
      create_OP_ARROW_constructor_closure = 
        caml_named_value("create_OP_ARROW_constructor");
    xassert(create_OP_ARROW_constructor_closure);
    return caml_callback(*create_OP_ARROW_constructor_closure, Val_unit);

  case OP_ARROW_STAR:
    if(create_OP_ARROW_STAR_constructor_closure == NULL)
      create_OP_ARROW_STAR_constructor_closure = 
        caml_named_value("create_OP_ARROW_STAR_constructor");
    xassert(create_OP_ARROW_STAR_constructor_closure);
    return caml_callback(*create_OP_ARROW_STAR_constructor_closure, Val_unit);

  case OP_BRACKETS:
    if(create_OP_BRACKETS_constructor_closure == NULL)
      create_OP_BRACKETS_constructor_closure = 
        caml_named_value("create_OP_BRACKETS_constructor");
    xassert(create_OP_BRACKETS_constructor_closure);
    return caml_callback(*create_OP_BRACKETS_constructor_closure, Val_unit);

  case OP_PARENS:
    if(create_OP_PARENS_constructor_closure == NULL)
      create_OP_PARENS_constructor_closure = 
        caml_named_value("create_OP_PARENS_constructor");
    xassert(create_OP_PARENS_constructor_closure);
    return caml_callback(*create_OP_PARENS_constructor_closure, Val_unit);

  case OP_COMMA:
    if(create_OP_COMMA_constructor_closure == NULL)
      create_OP_COMMA_constructor_closure = 
        caml_named_value("create_OP_COMMA_constructor");
    xassert(create_OP_COMMA_constructor_closure);
    return caml_callback(*create_OP_COMMA_constructor_closure, Val_unit);

  case OP_QUESTION:
    if(create_OP_QUESTION_constructor_closure == NULL)
      create_OP_QUESTION_constructor_closure = 
        caml_named_value("create_OP_QUESTION_constructor");
    xassert(create_OP_QUESTION_constructor_closure);
    return caml_callback(*create_OP_QUESTION_constructor_closure, Val_unit);

  case OP_MINIMUM:
    if(create_OP_MINIMUM_constructor_closure == NULL)
      create_OP_MINIMUM_constructor_closure = 
        caml_named_value("create_OP_MINIMUM_constructor");
    xassert(create_OP_MINIMUM_constructor_closure);
    return caml_callback(*create_OP_MINIMUM_constructor_closure, Val_unit);

  case OP_MAXIMUM:
    if(create_OP_MAXIMUM_constructor_closure == NULL)
      create_OP_MAXIMUM_constructor_closure = 
        caml_named_value("create_OP_MAXIMUM_constructor");
    xassert(create_OP_MAXIMUM_constructor_closure);
    return caml_callback(*create_OP_MAXIMUM_constructor_closure, Val_unit);

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us before
  return(Val_unit);
}


// hand written ocaml serialization function
value ocaml_from_UnaryOp(const UnaryOp &id, ToOcamlData *d){
  // don't allocate here, so don;t need the CAMLparam stuff

  static value * create_UNY_PLUS_constructor_closure = NULL;
  static value * create_UNY_MINUS_constructor_closure = NULL;
  static value * create_UNY_NOT_constructor_closure = NULL;
  static value * create_UNY_BITNOT_constructor_closure = NULL;

  switch(id){

  case UNY_PLUS:
    if(create_UNY_PLUS_constructor_closure == NULL)
      create_UNY_PLUS_constructor_closure = 
        caml_named_value("create_UNY_PLUS_constructor");
    xassert(create_UNY_PLUS_constructor_closure);
    return caml_callback(*create_UNY_PLUS_constructor_closure, Val_unit);

  case UNY_MINUS:
    if(create_UNY_MINUS_constructor_closure == NULL)
      create_UNY_MINUS_constructor_closure = 
        caml_named_value("create_UNY_MINUS_constructor");
    xassert(create_UNY_MINUS_constructor_closure);
    return caml_callback(*create_UNY_MINUS_constructor_closure, Val_unit);

  case UNY_NOT:
    if(create_UNY_NOT_constructor_closure == NULL)
      create_UNY_NOT_constructor_closure = 
        caml_named_value("create_UNY_NOT_constructor");
    xassert(create_UNY_NOT_constructor_closure);
    return caml_callback(*create_UNY_NOT_constructor_closure, Val_unit);

  case UNY_BITNOT:
    if(create_UNY_BITNOT_constructor_closure == NULL)
      create_UNY_BITNOT_constructor_closure = 
        caml_named_value("create_UNY_BITNOT_constructor");
    xassert(create_UNY_BITNOT_constructor_closure);
    return caml_callback(*create_UNY_BITNOT_constructor_closure, Val_unit);

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us before
  return(Val_unit);
}


// hand written ocaml serialization function
value ocaml_from_EffectOp(const EffectOp &id, ToOcamlData *d){
  // don't allocate here, so don;t need the CAMLparam stuff

  static value * create_EFF_POSTINC_constructor_closure = NULL;
  static value * create_EFF_POSTDEC_constructor_closure = NULL;
  static value * create_EFF_PREINC_constructor_closure = NULL;
  static value * create_EFF_PREDEC_constructor_closure = NULL;

  switch(id){

  case EFF_POSTINC:
    if(create_EFF_POSTINC_constructor_closure == NULL)
      create_EFF_POSTINC_constructor_closure = 
        caml_named_value("create_EFF_POSTINC_constructor");
    xassert(create_EFF_POSTINC_constructor_closure);
    return caml_callback(*create_EFF_POSTINC_constructor_closure, Val_unit);

  case EFF_POSTDEC:
    if(create_EFF_POSTDEC_constructor_closure == NULL)
      create_EFF_POSTDEC_constructor_closure = 
        caml_named_value("create_EFF_POSTDEC_constructor");
    xassert(create_EFF_POSTDEC_constructor_closure);
    return caml_callback(*create_EFF_POSTDEC_constructor_closure, Val_unit);

  case EFF_PREINC:
    if(create_EFF_PREINC_constructor_closure == NULL)
      create_EFF_PREINC_constructor_closure = 
        caml_named_value("create_EFF_PREINC_constructor");
    xassert(create_EFF_PREINC_constructor_closure);
    return caml_callback(*create_EFF_PREINC_constructor_closure, Val_unit);

  case EFF_PREDEC:
    if(create_EFF_PREDEC_constructor_closure == NULL)
      create_EFF_PREDEC_constructor_closure = 
        caml_named_value("create_EFF_PREDEC_constructor");
    xassert(create_EFF_PREDEC_constructor_closure);
    return caml_callback(*create_EFF_PREDEC_constructor_closure, Val_unit);

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us before
  return(Val_unit);
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

  switch(id){

  case BIN_EQUAL:
    if(create_BIN_EQUAL_constructor_closure == NULL)
      create_BIN_EQUAL_constructor_closure = 
        caml_named_value("create_BIN_EQUAL_constructor");
    xassert(create_BIN_EQUAL_constructor_closure);
    return caml_callback(*create_BIN_EQUAL_constructor_closure, Val_unit);

  case BIN_NOTEQUAL:
    if(create_BIN_NOTEQUAL_constructor_closure == NULL)
      create_BIN_NOTEQUAL_constructor_closure = 
        caml_named_value("create_BIN_NOTEQUAL_constructor");
    xassert(create_BIN_NOTEQUAL_constructor_closure);
    return caml_callback(*create_BIN_NOTEQUAL_constructor_closure, Val_unit);

  case BIN_LESS:
    if(create_BIN_LESS_constructor_closure == NULL)
      create_BIN_LESS_constructor_closure = 
        caml_named_value("create_BIN_LESS_constructor");
    xassert(create_BIN_LESS_constructor_closure);
    return caml_callback(*create_BIN_LESS_constructor_closure, Val_unit);

  case BIN_GREATER:
    if(create_BIN_GREATER_constructor_closure == NULL)
      create_BIN_GREATER_constructor_closure = 
        caml_named_value("create_BIN_GREATER_constructor");
    xassert(create_BIN_GREATER_constructor_closure);
    return caml_callback(*create_BIN_GREATER_constructor_closure, Val_unit);

  case BIN_LESSEQ:
    if(create_BIN_LESSEQ_constructor_closure == NULL)
      create_BIN_LESSEQ_constructor_closure = 
        caml_named_value("create_BIN_LESSEQ_constructor");
    xassert(create_BIN_LESSEQ_constructor_closure);
    return caml_callback(*create_BIN_LESSEQ_constructor_closure, Val_unit);

  case BIN_GREATEREQ:
    if(create_BIN_GREATEREQ_constructor_closure == NULL)
      create_BIN_GREATEREQ_constructor_closure = 
        caml_named_value("create_BIN_GREATEREQ_constructor");
    xassert(create_BIN_GREATEREQ_constructor_closure);
    return caml_callback(*create_BIN_GREATEREQ_constructor_closure, Val_unit);

  case BIN_MULT:
    if(create_BIN_MULT_constructor_closure == NULL)
      create_BIN_MULT_constructor_closure = 
        caml_named_value("create_BIN_MULT_constructor");
    xassert(create_BIN_MULT_constructor_closure);
    return caml_callback(*create_BIN_MULT_constructor_closure, Val_unit);

  case BIN_DIV:
    if(create_BIN_DIV_constructor_closure == NULL)
      create_BIN_DIV_constructor_closure = 
        caml_named_value("create_BIN_DIV_constructor");
    xassert(create_BIN_DIV_constructor_closure);
    return caml_callback(*create_BIN_DIV_constructor_closure, Val_unit);

  case BIN_MOD:
    if(create_BIN_MOD_constructor_closure == NULL)
      create_BIN_MOD_constructor_closure = 
        caml_named_value("create_BIN_MOD_constructor");
    xassert(create_BIN_MOD_constructor_closure);
    return caml_callback(*create_BIN_MOD_constructor_closure, Val_unit);

  case BIN_PLUS:
    if(create_BIN_PLUS_constructor_closure == NULL)
      create_BIN_PLUS_constructor_closure = 
        caml_named_value("create_BIN_PLUS_constructor");
    xassert(create_BIN_PLUS_constructor_closure);
    return caml_callback(*create_BIN_PLUS_constructor_closure, Val_unit);

  case BIN_MINUS:
    if(create_BIN_MINUS_constructor_closure == NULL)
      create_BIN_MINUS_constructor_closure = 
        caml_named_value("create_BIN_MINUS_constructor");
    xassert(create_BIN_MINUS_constructor_closure);
    return caml_callback(*create_BIN_MINUS_constructor_closure, Val_unit);

  case BIN_LSHIFT:
    if(create_BIN_LSHIFT_constructor_closure == NULL)
      create_BIN_LSHIFT_constructor_closure = 
        caml_named_value("create_BIN_LSHIFT_constructor");
    xassert(create_BIN_LSHIFT_constructor_closure);
    return caml_callback(*create_BIN_LSHIFT_constructor_closure, Val_unit);

  case BIN_RSHIFT:
    if(create_BIN_RSHIFT_constructor_closure == NULL)
      create_BIN_RSHIFT_constructor_closure = 
        caml_named_value("create_BIN_RSHIFT_constructor");
    xassert(create_BIN_RSHIFT_constructor_closure);
    return caml_callback(*create_BIN_RSHIFT_constructor_closure, Val_unit);

  case BIN_BITAND:
    if(create_BIN_BITAND_constructor_closure == NULL)
      create_BIN_BITAND_constructor_closure = 
        caml_named_value("create_BIN_BITAND_constructor");
    xassert(create_BIN_BITAND_constructor_closure);
    return caml_callback(*create_BIN_BITAND_constructor_closure, Val_unit);

  case BIN_BITXOR:
    if(create_BIN_BITXOR_constructor_closure == NULL)
      create_BIN_BITXOR_constructor_closure = 
        caml_named_value("create_BIN_BITXOR_constructor");
    xassert(create_BIN_BITXOR_constructor_closure);
    return caml_callback(*create_BIN_BITXOR_constructor_closure, Val_unit);

  case BIN_BITOR:
    if(create_BIN_BITOR_constructor_closure == NULL)
      create_BIN_BITOR_constructor_closure = 
        caml_named_value("create_BIN_BITOR_constructor");
    xassert(create_BIN_BITOR_constructor_closure);
    return caml_callback(*create_BIN_BITOR_constructor_closure, Val_unit);

  case BIN_AND:
    if(create_BIN_AND_constructor_closure == NULL)
      create_BIN_AND_constructor_closure = 
        caml_named_value("create_BIN_AND_constructor");
    xassert(create_BIN_AND_constructor_closure);
    return caml_callback(*create_BIN_AND_constructor_closure, Val_unit);

  case BIN_OR:
    if(create_BIN_OR_constructor_closure == NULL)
      create_BIN_OR_constructor_closure = 
        caml_named_value("create_BIN_OR_constructor");
    xassert(create_BIN_OR_constructor_closure);
    return caml_callback(*create_BIN_OR_constructor_closure, Val_unit);

  case BIN_COMMA:
    if(create_BIN_COMMA_constructor_closure == NULL)
      create_BIN_COMMA_constructor_closure = 
        caml_named_value("create_BIN_COMMA_constructor");
    xassert(create_BIN_COMMA_constructor_closure);
    return caml_callback(*create_BIN_COMMA_constructor_closure, Val_unit);

  case BIN_MINIMUM:
    if(create_BIN_MINIMUM_constructor_closure == NULL)
      create_BIN_MINIMUM_constructor_closure = 
        caml_named_value("create_BIN_MINIMUM_constructor");
    xassert(create_BIN_MINIMUM_constructor_closure);
    return caml_callback(*create_BIN_MINIMUM_constructor_closure, Val_unit);

  case BIN_MAXIMUM:
    if(create_BIN_MAXIMUM_constructor_closure == NULL)
      create_BIN_MAXIMUM_constructor_closure = 
        caml_named_value("create_BIN_MAXIMUM_constructor");
    xassert(create_BIN_MAXIMUM_constructor_closure);
    return caml_callback(*create_BIN_MAXIMUM_constructor_closure, Val_unit);

  case BIN_BRACKETS:
    if(create_BIN_BRACKETS_constructor_closure == NULL)
      create_BIN_BRACKETS_constructor_closure = 
        caml_named_value("create_BIN_BRACKETS_constructor");
    xassert(create_BIN_BRACKETS_constructor_closure);
    return caml_callback(*create_BIN_BRACKETS_constructor_closure, Val_unit);

  case BIN_ASSIGN:
    if(create_BIN_ASSIGN_constructor_closure == NULL)
      create_BIN_ASSIGN_constructor_closure = 
        caml_named_value("create_BIN_ASSIGN_constructor");
    xassert(create_BIN_ASSIGN_constructor_closure);
    return caml_callback(*create_BIN_ASSIGN_constructor_closure, Val_unit);

  case BIN_DOT_STAR:
    if(create_BIN_DOT_STAR_constructor_closure == NULL)
      create_BIN_DOT_STAR_constructor_closure = 
        caml_named_value("create_BIN_DOT_STAR_constructor");
    xassert(create_BIN_DOT_STAR_constructor_closure);
    return caml_callback(*create_BIN_DOT_STAR_constructor_closure, Val_unit);

  case BIN_ARROW_STAR:
    if(create_BIN_ARROW_STAR_constructor_closure == NULL)
      create_BIN_ARROW_STAR_constructor_closure = 
        caml_named_value("create_BIN_ARROW_STAR_constructor");
    xassert(create_BIN_ARROW_STAR_constructor_closure);
    return caml_callback(*create_BIN_ARROW_STAR_constructor_closure, Val_unit);

  case BIN_IMPLIES:
    if(create_BIN_IMPLIES_constructor_closure == NULL)
      create_BIN_IMPLIES_constructor_closure = 
        caml_named_value("create_BIN_IMPLIES_constructor");
    xassert(create_BIN_IMPLIES_constructor_closure);
    return caml_callback(*create_BIN_IMPLIES_constructor_closure, Val_unit);

  case BIN_EQUIVALENT:
    if(create_BIN_EQUIVALENT_constructor_closure == NULL)
      create_BIN_EQUIVALENT_constructor_closure = 
        caml_named_value("create_BIN_EQUIVALENT_constructor");
    xassert(create_BIN_EQUIVALENT_constructor_closure);
    return caml_callback(*create_BIN_EQUIVALENT_constructor_closure, Val_unit);

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us before
  return(Val_unit);
}


// hand written ocaml serialization function
value ocaml_from_CastKeyword(const CastKeyword &id, ToOcamlData *d){
  // don't allocate here, so don;t need the CAMLparam stuff

  static value * create_CK_DYNAMIC_constructor_closure = NULL;
  static value * create_CK_STATIC_constructor_closure = NULL;
  static value * create_CK_REINTERPRET_constructor_closure = NULL;
  static value * create_CK_CONST_constructor_closure = NULL;

  switch(id){

  case CK_DYNAMIC:
    if(create_CK_DYNAMIC_constructor_closure == NULL)
      create_CK_DYNAMIC_constructor_closure = 
        caml_named_value("create_CK_DYNAMIC_constructor");
    xassert(create_CK_DYNAMIC_constructor_closure);
    return caml_callback(*create_CK_DYNAMIC_constructor_closure, Val_unit);

  case CK_STATIC:
    if(create_CK_STATIC_constructor_closure == NULL)
      create_CK_STATIC_constructor_closure = 
        caml_named_value("create_CK_STATIC_constructor");
    xassert(create_CK_STATIC_constructor_closure);
    return caml_callback(*create_CK_STATIC_constructor_closure, Val_unit);

  case CK_REINTERPRET:
    if(create_CK_REINTERPRET_constructor_closure == NULL)
      create_CK_REINTERPRET_constructor_closure = 
        caml_named_value("create_CK_REINTERPRET_constructor");
    xassert(create_CK_REINTERPRET_constructor_closure);
    return caml_callback(*create_CK_REINTERPRET_constructor_closure, Val_unit);

  case CK_CONST:
    if(create_CK_CONST_constructor_closure == NULL)
      create_CK_CONST_constructor_closure = 
        caml_named_value("create_CK_CONST_constructor");
    xassert(create_CK_CONST_constructor_closure);
    return caml_callback(*create_CK_CONST_constructor_closure, Val_unit);

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us before
  return(Val_unit);
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
 
