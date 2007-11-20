ast_config.cmo: ../util/more_string.cmi ast_config.cmi 
ast_config.cmx: ../util/more_string.cmx ast_config.cmi 
ast_type_graph.cmo: meta_ast.cmi ../util/dot_graph.cmi ast_config.cmi 
ast_type_graph.cmx: meta_ast.cmx ../util/dot_graph.cmx ast_config.cmx 
check_oast.cmo: ../build/elsa/elsa_oast_header.cmx elsa_ast_type_descr.cmx \
    ../build/astgen/ast_oast_header.cmx ast_ast_type_descr.cmx \
    ../build/general/ast_annotation.cmi 
check_oast.cmx: ../build/elsa/elsa_oast_header.cmx elsa_ast_type_descr.cmx \
    ../build/astgen/ast_oast_header.cmx ast_ast_type_descr.cmx \
    ../build/general/ast_annotation.cmx 
gen_accessors.cmo: ../util/more_string.cmi meta_ast.cmi ast_config.cmi 
gen_accessors.cmx: ../util/more_string.cmx meta_ast.cmx ast_config.cmx 
gen_graph.cmo: ../util/more_string.cmi meta_ast.cmi ast_config.cmi 
gen_graph.cmx: ../util/more_string.cmx meta_ast.cmx ast_config.cmx 
gen_reflection.cmo: ../util/more_string.cmi meta_ast.cmi ast_config.cmi 
gen_reflection.cmx: ../util/more_string.cmx meta_ast.cmx ast_config.cmx 
gen_superast.cmo: ../util/more_string.cmi meta_ast.cmi ast_config.cmi 
gen_superast.cmx: ../util/more_string.cmx meta_ast.cmx ast_config.cmx 
gen_uplinks.cmo: ../util/more_string.cmi meta_ast.cmi ast_config.cmi 
gen_uplinks.cmx: ../util/more_string.cmx meta_ast.cmx ast_config.cmx 
meta_ast.cmo: ../util/stdlib_missing.cmi ../util/more_string.cmi \
    ../build/astgen/ast_reflect_type.cmx ../build/astgen/ast_oast_header.cmx \
    ../build/astgen/ast_ml_types.cmx ast_config.cmi \
    ../build/general/ast_annotation.cmi meta_ast.cmi 
meta_ast.cmx: ../util/stdlib_missing.cmx ../util/more_string.cmx \
    ../build/astgen/ast_reflect_type.cmx ../build/astgen/ast_oast_header.cmx \
    ../build/astgen/ast_ml_types.cmx ast_config.cmx \
    ../build/general/ast_annotation.cmx meta_ast.cmi 
meta_ast.cmi: ../build/astgen/ast_reflect_type.cmx \
    ../build/astgen/ast_ml_types.cmx ../build/general/ast_annotation.cmi 
