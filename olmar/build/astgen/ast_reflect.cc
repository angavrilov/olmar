//  Copyright 2006 Hendrik Tews, All rights reserved.                  *
//  See file license.txt for terms of use                              *
//**********************************************************************

// astgen specific ocaml reflection stuff

#include "str.h"
#include "ast.hand.h"
#include "ast_reflect.h"


void reflect_into_ocaml(char ** argv, string ofile, ASTSpecFile const * ast){
  CAMLparam0();
  CAMLlocal2(ocaml_ast, of);

  cout << "generating ocaml ast ...\n";

  caml_startup(argv);

  static value * register_closure = NULL;
  if(register_closure == NULL)
    register_closure = caml_named_value("register_caml_callbacks");
  xassert(register_closure);
  caml_callback(*register_closure, Val_unit);
  
  Ocaml_reflection_data ocaml_data;

  ocaml_ast = ocaml_reflect_ASTSpecFile(ast, &ocaml_data);
  xassert(ocaml_data.stack.size() == 0);

  static value * marshal_callback = NULL;
  if(marshal_callback == NULL)
    marshal_callback = caml_named_value("marshal_oast_catch_all");
  xassert(marshal_callback);

  string ocamlAstFname(stringc << ofile << ".oast");
  of = caml_copy_string(ocamlAstFname.c_str());
  cout << "writing " << ocamlAstFname << "...\n";

  caml_callback2(*marshal_callback, ocaml_ast, of);

  CAMLreturn0;
}



// hand written ocaml serialization function
value ocaml_reflect_FieldFlags(FieldFlags const * f, Ocaml_reflection_data *d){
  CAMLparam0();
  CAMLlocal2(camlf, result);

  static value * fieldFlags_from_int_closure = NULL;
  if(fieldFlags_from_int_closure == NULL)
    fieldFlags_from_int_closure = caml_named_value("fieldFlags_from_int");
  xassert(fieldFlags_from_int_closure);

  xassert(*f <= Max_long && Min_long <= *f);
  camlf = Val_int(*f);
  xassert(IS_OCAML_AST_VALUE(camlf));
  result = caml_callback(*fieldFlags_from_int_closure, camlf);
  xassert(IS_OCAML_AST_VALUE(result));

  CAMLreturn(result);
}


// hand written ocaml serialization function
value ocaml_reflect_AccessCtl(AccessCtl const * id, Ocaml_reflection_data *d) {
  // don't allocate here, so don't need the CAMLparam stuff

  static value * create_AC_PUBLIC_constructor_closure = NULL;
  static value * create_AC_PRIVATE_constructor_closure = NULL;
  static value * create_AC_PROTECTED_constructor_closure = NULL;
  static value * create_AC_CTOR_constructor_closure = NULL;
  static value * create_AC_DTOR_constructor_closure = NULL;
  static value * create_AC_PUREVIRT_constructor_closure = NULL;

  value result;

  switch(*id){

  case AC_PUBLIC:
    if(create_AC_PUBLIC_constructor_closure == NULL)
      create_AC_PUBLIC_constructor_closure = 
        caml_named_value("create_AC_PUBLIC_constructor");
    xassert(create_AC_PUBLIC_constructor_closure);
    result = caml_callback(*create_AC_PUBLIC_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case AC_PRIVATE:
    if(create_AC_PRIVATE_constructor_closure == NULL)
      create_AC_PRIVATE_constructor_closure = 
        caml_named_value("create_AC_PRIVATE_constructor");
    xassert(create_AC_PRIVATE_constructor_closure);
    result = caml_callback(*create_AC_PRIVATE_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case AC_PROTECTED:
    if(create_AC_PROTECTED_constructor_closure == NULL)
      create_AC_PROTECTED_constructor_closure = 
        caml_named_value("create_AC_PROTECTED_constructor");
    xassert(create_AC_PROTECTED_constructor_closure);
    result = caml_callback(*create_AC_PROTECTED_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case AC_CTOR:
    if(create_AC_CTOR_constructor_closure == NULL)
      create_AC_CTOR_constructor_closure = 
        caml_named_value("create_AC_CTOR_constructor");
    xassert(create_AC_CTOR_constructor_closure);
    result = caml_callback(*create_AC_CTOR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case AC_DTOR:
    if(create_AC_DTOR_constructor_closure == NULL)
      create_AC_DTOR_constructor_closure = 
        caml_named_value("create_AC_DTOR_constructor");
    xassert(create_AC_DTOR_constructor_closure);
    result = caml_callback(*create_AC_DTOR_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  case AC_PUREVIRT:
    if(create_AC_PUREVIRT_constructor_closure == NULL)
      create_AC_PUREVIRT_constructor_closure = 
        caml_named_value("create_AC_PUREVIRT_constructor");
    xassert(create_AC_PUREVIRT_constructor_closure);
    result = caml_callback(*create_AC_PUREVIRT_constructor_closure, Val_unit);
    xassert(IS_OCAML_AST_VALUE(result));
    return result;

  default:
    xassert(false);
    break;
  }

  // not reached, the above assertion takes us out before
  xassert(false);
}

