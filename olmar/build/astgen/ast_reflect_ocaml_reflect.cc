/* ** DO NOT EDIT ***** DO NOT EDIT ***** DO NOT EDIT ***** DO NOT EDIT ***
 * 
 * automatically generated by gen_reflection from ../build/astgen/ast.ast.oast
 * 
 * **********************************************************************
 * *************** Ocaml reflection tree traversal **********************
 * **********************************************************************
 */


#include "ast_reflect_ocaml_reflect.h"


/* **********************************************************************
 * Reflection functions declarations
 */


value ocaml_reflect_ASTSpecFile(ASTSpecFile const * x, Ocaml_reflection_data * data);
value ocaml_reflect_ToplevelForm_ast_list(ASTList<ToplevelForm> const * x, Ocaml_reflection_data * data);
value ocaml_reflect_ToplevelForm(ToplevelForm const * x, Ocaml_reflection_data * data);
value ocaml_reflect_TF_verbatim(TF_verbatim const * x, Ocaml_reflection_data * data);
value ocaml_reflect_TF_impl_verbatim(TF_impl_verbatim const * x, Ocaml_reflection_data * data);
value ocaml_reflect_TF_ocaml_type_verbatim(TF_ocaml_type_verbatim const * x, Ocaml_reflection_data * data);
value ocaml_reflect_TF_xml_verbatim(TF_xml_verbatim const * x, Ocaml_reflection_data * data);
value ocaml_reflect_TF_class(TF_class const * x, Ocaml_reflection_data * data);
value ocaml_reflect_TF_option(TF_option const * x, Ocaml_reflection_data * data);
value ocaml_reflect_TF_custom(TF_custom const * x, Ocaml_reflection_data * data);
value ocaml_reflect_TF_enum(TF_enum const * x, Ocaml_reflection_data * data);
/* relying on extern
 * value ocaml_reflect_string(string const * x, Ocaml_reflection_data * data)
 */
value ocaml_reflect_ASTClass(ASTClass const * x, Ocaml_reflection_data * data);
value ocaml_reflect_ASTClass_ast_list(ASTList<ASTClass> const * x, Ocaml_reflection_data * data);
value ocaml_reflect_string_ast_list(ASTList<string> const * x, Ocaml_reflection_data * data);
value ocaml_reflect_CustomCode(CustomCode const * x, Ocaml_reflection_data * data);
value ocaml_reflect_Annotation(Annotation const * x, Ocaml_reflection_data * data);
value ocaml_reflect_UserDecl(UserDecl const * x, Ocaml_reflection_data * data);
value ocaml_reflect_FieldOrCtorArg_ast_list(ASTList<FieldOrCtorArg> const * x, Ocaml_reflection_data * data);
value ocaml_reflect_FieldOrCtorArg(FieldOrCtorArg const * x, Ocaml_reflection_data * data);
value ocaml_reflect_BaseClass_ast_list(ASTList<BaseClass> const * x, Ocaml_reflection_data * data);
value ocaml_reflect_BaseClass(BaseClass const * x, Ocaml_reflection_data * data);
value ocaml_reflect_Annotation_ast_list(ASTList<Annotation> const * x, Ocaml_reflection_data * data);
/* relying on extern
 * value ocaml_reflect_AccessCtl(AccessCtl const * x, Ocaml_reflection_data * data)
 */
value ocaml_reflect_AccessMod(AccessMod const * x, Ocaml_reflection_data * data);
/* relying on extern
 * value ocaml_reflect_FieldFlags(FieldFlags const * x, Ocaml_reflection_data * data)
 */


/* **********************************************************************
 * Reflection functions definitions
 */


value ocaml_reflect_Annotation(Annotation const * x, Ocaml_reflection_data * data) {
  switch(x->kind()) {
  case Annotation::USERDECL:
    return ocaml_reflect_UserDecl(x->asUserDeclC(), data);

  case Annotation::CUSTOMCODE:
    return ocaml_reflect_CustomCode(x->asCustomCodeC(), data);

  case Annotation::NUM_KINDS:
    xassert(false);
    break;
  }
  xassert(false);
}


value ocaml_reflect_FieldOrCtorArg_ast_list(ASTList<FieldOrCtorArg> const * x, Ocaml_reflection_data * data) {
  CAMLparam0();
  CAMLlocal4(res, tmp, elem, previous);
  res = find_ocaml_shared_list_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  previous = 0;

  FOREACH_ASTLIST(FieldOrCtorArg, *x, iter) {
    elem = ocaml_reflect_FieldOrCtorArg(iter.data(), data);
    tmp = caml_alloc(2, Tag_cons);
    Store_field(tmp, 0, elem);
    Store_field(tmp, 1, Val_emptylist);
    if(previous == 0) {
      res = tmp;
    } else {
      Store_field(previous, 1, tmp);
    }
    previous = tmp;
  }

  register_ocaml_shared_list_value(x, res);
  CAMLreturn(res);
}


value ocaml_reflect_ToplevelForm_ast_list(ASTList<ToplevelForm> const * x, Ocaml_reflection_data * data) {
  CAMLparam0();
  CAMLlocal4(res, tmp, elem, previous);
  res = find_ocaml_shared_list_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  previous = 0;

  FOREACH_ASTLIST(ToplevelForm, *x, iter) {
    elem = ocaml_reflect_ToplevelForm(iter.data(), data);
    tmp = caml_alloc(2, Tag_cons);
    Store_field(tmp, 0, elem);
    Store_field(tmp, 1, Val_emptylist);
    if(previous == 0) {
      res = tmp;
    } else {
      Store_field(previous, 1, tmp);
    }
    previous = tmp;
  }

  register_ocaml_shared_list_value(x, res);
  CAMLreturn(res);
}


value ocaml_reflect_FieldOrCtorArg(FieldOrCtorArg const * x, Ocaml_reflection_data * data) {
  /* reflect FieldOrCtorArg into a record */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(5, 0);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_FieldFlags(&x->flags, data);
  Store_field(res, 1, child);

  child = ocaml_reflect_string(&x->type, data);
  Store_field(res, 2, child);

  child = ocaml_reflect_string(&x->name, data);
  Store_field(res, 3, child);

  child = ocaml_reflect_string(&x->defaultValue, data);
  Store_field(res, 4, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_TF_enum(TF_enum const * x, Ocaml_reflection_data * data) {
  /* reflect TF_enum into a variant */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(3, 7);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_string(&x->name, data);
  Store_field(res, 1, child);

  child = ocaml_reflect_string_ast_list(&x->enumerators, data);
  Store_field(res, 2, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_TF_custom(TF_custom const * x, Ocaml_reflection_data * data) {
  /* reflect TF_custom into a variant */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(2, 6);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_CustomCode(x->cust, data);
  Store_field(res, 1, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_TF_option(TF_option const * x, Ocaml_reflection_data * data) {
  /* reflect TF_option into a variant */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(3, 5);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_string(&x->name, data);
  Store_field(res, 1, child);

  child = ocaml_reflect_string_ast_list(&x->args, data);
  Store_field(res, 2, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_TF_class(TF_class const * x, Ocaml_reflection_data * data) {
  /* reflect TF_class into a variant */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(3, 4);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_ASTClass(x->super, data);
  Store_field(res, 1, child);

  child = ocaml_reflect_ASTClass_ast_list(&x->ctors, data);
  Store_field(res, 2, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_TF_xml_verbatim(TF_xml_verbatim const * x, Ocaml_reflection_data * data) {
  /* reflect TF_xml_verbatim into a variant */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(2, 3);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_string(&x->code, data);
  Store_field(res, 1, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_TF_ocaml_type_verbatim(TF_ocaml_type_verbatim const * x, Ocaml_reflection_data * data) {
  /* reflect TF_ocaml_type_verbatim into a variant */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(2, 2);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_string(&x->code, data);
  Store_field(res, 1, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_TF_impl_verbatim(TF_impl_verbatim const * x, Ocaml_reflection_data * data) {
  /* reflect TF_impl_verbatim into a variant */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(2, 1);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_string(&x->code, data);
  Store_field(res, 1, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_TF_verbatim(TF_verbatim const * x, Ocaml_reflection_data * data) {
  /* reflect TF_verbatim into a variant */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(2, 0);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_string(&x->code, data);
  Store_field(res, 1, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_ToplevelForm(ToplevelForm const * x, Ocaml_reflection_data * data) {
  switch(x->kind()) {
  case ToplevelForm::TF_VERBATIM:
    return ocaml_reflect_TF_verbatim(x->asTF_verbatimC(), data);

  case ToplevelForm::TF_IMPL_VERBATIM:
    return ocaml_reflect_TF_impl_verbatim(x->asTF_impl_verbatimC(), data);

  case ToplevelForm::TF_OCAML_TYPE_VERBATIM:
    return ocaml_reflect_TF_ocaml_type_verbatim(x->asTF_ocaml_type_verbatimC(), data);

  case ToplevelForm::TF_XML_VERBATIM:
    return ocaml_reflect_TF_xml_verbatim(x->asTF_xml_verbatimC(), data);

  case ToplevelForm::TF_CLASS:
    return ocaml_reflect_TF_class(x->asTF_classC(), data);

  case ToplevelForm::TF_OPTION:
    return ocaml_reflect_TF_option(x->asTF_optionC(), data);

  case ToplevelForm::TF_CUSTOM:
    return ocaml_reflect_TF_custom(x->asTF_customC(), data);

  case ToplevelForm::TF_ENUM:
    return ocaml_reflect_TF_enum(x->asTF_enumC(), data);

  case ToplevelForm::NUM_KINDS:
    xassert(false);
    break;
  }
  xassert(false);
}


value ocaml_reflect_AccessMod(AccessMod const * x, Ocaml_reflection_data * data) {
  /* reflect AccessMod into a record */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(3, 0);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_AccessCtl(&x->acc, data);
  Store_field(res, 1, child);

  child = ocaml_reflect_string_ast_list(&x->mods, data);
  Store_field(res, 2, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_BaseClass(BaseClass const * x, Ocaml_reflection_data * data) {
  /* reflect BaseClass into a record */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(3, 0);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_AccessCtl(&x->access, data);
  Store_field(res, 1, child);

  child = ocaml_reflect_string(&x->name, data);
  Store_field(res, 2, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_ASTSpecFile(ASTSpecFile const * x, Ocaml_reflection_data * data) {
  /* reflect ASTSpecFile into a record */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(2, 0);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_ToplevelForm_ast_list(&x->forms, data);
  Store_field(res, 1, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_Annotation_ast_list(ASTList<Annotation> const * x, Ocaml_reflection_data * data) {
  CAMLparam0();
  CAMLlocal4(res, tmp, elem, previous);
  res = find_ocaml_shared_list_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  previous = 0;

  FOREACH_ASTLIST(Annotation, *x, iter) {
    elem = ocaml_reflect_Annotation(iter.data(), data);
    tmp = caml_alloc(2, Tag_cons);
    Store_field(tmp, 0, elem);
    Store_field(tmp, 1, Val_emptylist);
    if(previous == 0) {
      res = tmp;
    } else {
      Store_field(previous, 1, tmp);
    }
    previous = tmp;
  }

  register_ocaml_shared_list_value(x, res);
  CAMLreturn(res);
}


value ocaml_reflect_ASTClass(ASTClass const * x, Ocaml_reflection_data * data) {
  /* reflect ASTClass into a record */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(6, 0);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_string(&x->name, data);
  Store_field(res, 1, child);

  child = ocaml_reflect_FieldOrCtorArg_ast_list(&x->args, data);
  Store_field(res, 2, child);

  child = ocaml_reflect_FieldOrCtorArg_ast_list(&x->lastArgs, data);
  Store_field(res, 3, child);

  child = ocaml_reflect_BaseClass_ast_list(&x->bases, data);
  Store_field(res, 4, child);

  child = ocaml_reflect_Annotation_ast_list(&x->decls, data);
  Store_field(res, 5, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_UserDecl(UserDecl const * x, Ocaml_reflection_data * data) {
  /* reflect UserDecl into a variant */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(4, 0);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_AccessMod(x->amod, data);
  Store_field(res, 1, child);

  child = ocaml_reflect_string(&x->code, data);
  Store_field(res, 2, child);

  child = ocaml_reflect_string(&x->init, data);
  Store_field(res, 3, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_CustomCode(CustomCode const * x, Ocaml_reflection_data * data) {
  /* reflect CustomCode into a variant */ 
  CAMLparam0();
  CAMLlocal2(res, child);
  res = find_ocaml_shared_node_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  if(data->stack.contains(x)) {
    cerr << "cyclic ast detected during ocaml reflection\n";
    xassert(false);
  } else {
    data->stack.add(x);
  }
  res = caml_alloc(3, 1);

  child = ocaml_ast_annotation(x, data);
  Store_field(res, 0, child);

  child = ocaml_reflect_string(&x->qualifier, data);
  Store_field(res, 1, child);

  child = ocaml_reflect_string(&x->code, data);
  Store_field(res, 2, child);

  register_ocaml_shared_node_value(x, res);
  data->stack.remove(x);
  CAMLreturn(res);
}


value ocaml_reflect_BaseClass_ast_list(ASTList<BaseClass> const * x, Ocaml_reflection_data * data) {
  CAMLparam0();
  CAMLlocal4(res, tmp, elem, previous);
  res = find_ocaml_shared_list_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  previous = 0;

  FOREACH_ASTLIST(BaseClass, *x, iter) {
    elem = ocaml_reflect_BaseClass(iter.data(), data);
    tmp = caml_alloc(2, Tag_cons);
    Store_field(tmp, 0, elem);
    Store_field(tmp, 1, Val_emptylist);
    if(previous == 0) {
      res = tmp;
    } else {
      Store_field(previous, 1, tmp);
    }
    previous = tmp;
  }

  register_ocaml_shared_list_value(x, res);
  CAMLreturn(res);
}


value ocaml_reflect_ASTClass_ast_list(ASTList<ASTClass> const * x, Ocaml_reflection_data * data) {
  CAMLparam0();
  CAMLlocal4(res, tmp, elem, previous);
  res = find_ocaml_shared_list_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  previous = 0;

  FOREACH_ASTLIST(ASTClass, *x, iter) {
    elem = ocaml_reflect_ASTClass(iter.data(), data);
    tmp = caml_alloc(2, Tag_cons);
    Store_field(tmp, 0, elem);
    Store_field(tmp, 1, Val_emptylist);
    if(previous == 0) {
      res = tmp;
    } else {
      Store_field(previous, 1, tmp);
    }
    previous = tmp;
  }

  register_ocaml_shared_list_value(x, res);
  CAMLreturn(res);
}


value ocaml_reflect_string_ast_list(ASTList<string> const * x, Ocaml_reflection_data * data) {
  CAMLparam0();
  CAMLlocal4(res, tmp, elem, previous);
  res = find_ocaml_shared_list_value(x);
  if( res != Val_None ) {
    xassert(Is_block(res) && Tag_val(res) == 0);
    CAMLreturn(Field(res, 0));
  }

  previous = 0;

  FOREACH_ASTLIST(string, *x, iter) {
    elem = ocaml_reflect_string(iter.data(), data);
    tmp = caml_alloc(2, Tag_cons);
    Store_field(tmp, 0, elem);
    Store_field(tmp, 1, Val_emptylist);
    if(previous == 0) {
      res = tmp;
    } else {
      Store_field(previous, 1, tmp);
    }
    previous = tmp;
  }

  register_ocaml_shared_list_value(x, res);
  CAMLreturn(res);
}



