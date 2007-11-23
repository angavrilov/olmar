./elsa_ast_util.cmo: elsa_reflect_type.cmx ./elsa_ast_util.cmi 
./elsa_ast_util.cmx: elsa_reflect_type.cmx ./elsa_ast_util.cmi 
./elsa_ml_base_reflection.cmo: elsa_ml_base_types.cmx 
./elsa_ml_base_reflection.cmx: elsa_ml_base_types.cmx 
./elsa_ast_util.cmi: elsa_reflect_type.cmx 
./elsa_oast_header.cmo: elsa_reflect_type.cmx ../general/ast_annotation.cmi 
./elsa_oast_header.cmx: elsa_reflect_type.cmx ../general/ast_annotation.cmx 
./elsa_caml_callbacks.cmo: elsa_oast_header.cmx elsa_ml_flag_constructors.cmx \
    elsa_ml_base_reflection.cmx ../general/astgen_util.cmx \
    ../general/ast_annotation.cmi 
./elsa_caml_callbacks.cmx: elsa_oast_header.cmx elsa_ml_flag_constructors.cmx \
    elsa_ml_base_reflection.cmx ../general/astgen_util.cmx \
    ../general/ast_annotation.cmx 
./elsa_ml_flag_constructors.cmo: elsa_ml_flag_types.cmx 
./elsa_ml_flag_constructors.cmx: elsa_ml_flag_types.cmx 
./elsa_ml_flag_types.cmo: ../general/astgen_util.cmx 
./elsa_ml_flag_types.cmx: ../general/astgen_util.cmx 
./elsa_reflect_type.cmo: elsa_ml_flag_types.cmx elsa_ml_base_types.cmx 
./elsa_reflect_type.cmx: elsa_ml_flag_types.cmx elsa_ml_base_types.cmx 
