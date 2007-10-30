
open Elsa_reflect_type
open Ast_annotation

type oast_type = annotated compilationUnit_type

let header_comment = 
  format_of_string "Marshaled elsa abstract syntax tree, version %d"

let oast_header_version = 
