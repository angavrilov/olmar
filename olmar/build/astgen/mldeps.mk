ast_ml_types.cmo: ../general/astgen_util.cmx 
ast_ml_types.cmx: ../general/astgen_util.cmx 
ast_ml_constructors.cmo: ast_ml_types.cmx 
ast_ml_constructors.cmx: ast_ml_types.cmx 
ast_reflect_type.cmo: ast_ml_types.cmx 
ast_reflect_type.cmx: ast_ml_types.cmx 
ast_oast_header.cmo: ast_reflect_type.cmx ../general/ast_annotation.cmi 
ast_oast_header.cmx: ast_reflect_type.cmx ../general/ast_annotation.cmx 
ast_caml_callbacks.cmo: ../general/astgen_util.cmx ast_oast_header.cmx \
    ast_ml_constructors.cmx ../general/ast_annotation.cmi 
ast_caml_callbacks.cmx: ../general/astgen_util.cmx ast_oast_header.cmx \
    ast_ml_constructors.cmx ../general/ast_annotation.cmx 
