ast_config.cmo: ../util/more_string.cmi ast_config.cmi 
ast_config.cmx: ../util/more_string.cmx ast_config.cmi 
check_oast.cmo: ../build/astgen/ast_oast_header.cmx \
    ../build/general/ast_annotation.cmi 
check_oast.cmx: ../build/astgen/ast_oast_header.cmx \
    ../build/general/ast_annotation.cmx 
gen_accessors.cmo: ../util/more_string.cmi meta_ast.cmi ast_config.cmi 
gen_accessors.cmx: ../util/more_string.cmx meta_ast.cmx ast_config.cmx 
gen_reflection.cmo: ../util/more_string.cmi meta_ast.cmi ast_config.cmi 
gen_reflection.cmx: ../util/more_string.cmx meta_ast.cmx ast_config.cmx 
gen_superast.cmo: ../util/more_string.cmi meta_ast.cmi ast_config.cmi 
gen_superast.cmx: ../util/more_string.cmx meta_ast.cmx ast_config.cmx 
gen_uplinks.cmo: ../util/more_string.cmi meta_ast.cmi ast_config.cmi 
gen_uplinks.cmx: ../util/more_string.cmx meta_ast.cmx ast_config.cmx 
meta_ast.cmo: ../util/more_string.cmi ../build/astgen/ast_reflect_type.cmx \
    ../build/astgen/ast_oast_header.cmx ../build/astgen/ast_ml_types.cmx \
    ast_config.cmi ../build/general/ast_annotation.cmi meta_ast.cmi 
meta_ast.cmx: ../util/more_string.cmx ../build/astgen/ast_reflect_type.cmx \
    ../build/astgen/ast_oast_header.cmx ../build/astgen/ast_ml_types.cmx \
    ast_config.cmx ../build/general/ast_annotation.cmx meta_ast.cmi 
meta_ast.cmi: ../build/astgen/ast_reflect_type.cmx \
    ../build/astgen/ast_ml_types.cmx ../build/general/ast_annotation.cmi 
