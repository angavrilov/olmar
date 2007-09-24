
open Ast_reflect_type
open Ast_annotation

type oast_type = annotated aSTSpecFile_type

let header_comment = 
  format_of_string "Marshaled astgen abstract syntax tree, version %d"

let oast_header_version = 
