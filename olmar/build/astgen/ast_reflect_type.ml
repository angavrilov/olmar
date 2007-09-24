(* ** DO NOT EDIT ***** DO NOT EDIT ***** DO NOT EDIT ***** DO NOT EDIT ***
 * 
 * automatically generated by gen_reflection from ../build/astgen/ast.ast.oast
 * 
 * ************************************************************************
 * ********************** Ast type definition *****************************
 * ************************************************************************
 *)


(* header from control file
 * 
 *)
open Ast_ml_types


(* syntax tree type definition
 * 
 *)
type 'a aSTSpecFile_type = {
  aSTSpecFile_annotation : 'a;
  forms : 'a toplevelForm_type list;
}


and 'a toplevelForm_type = 
  | TF_verbatim of 'a * string
  | TF_impl_verbatim of 'a * string
  | TF_ocaml_type_verbatim of 'a * string
  | TF_xml_verbatim of 'a * string
  | TF_class of 'a * 'a aSTClass_type * 'a aSTClass_type list
  | TF_option of 'a * string * string list
  | TF_custom of 'a * 'a annotation_type (* = CustomCode *)
  | TF_enum of 'a * string * string list


and 'a aSTClass_type = {
  aSTClass_annotation : 'a;
  cl_name : string;
  args : 'a fieldOrCtorArg_type list;
  lastArgs : 'a fieldOrCtorArg_type list;
  bases : 'a baseClass_type list;
  decls : 'a annotation_type list;
}


and 'a accessMod_type = {
  accessMod_annotation : 'a;
  acc : accessCtl;
  mods : string list;
}


and 'a annotation_type = 
  | UserDecl of 'a * 'a accessMod_type * string * string
  | CustomCode of 'a * string * string


and 'a fieldOrCtorArg_type = {
  fieldOrCtorArg_annotation : 'a;
  flags : fieldFlags;
  field_type : string;
  field_name : string;
  defaultValue : string;
}


and 'a baseClass_type = {
  baseClass_annotation : 'a;
  access : accessCtl;
  base_name : string;
}


