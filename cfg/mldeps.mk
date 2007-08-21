build.cmo: ../elsa/oast_header.cmo ../elsa/ml_ctype.cmo cfg_util.cmi \
    cfg_type.cmo ../elsa/cc_ml_types.cmo ../elsa/cc_ast_gen_type.cmo \
    ../elsa/ast_annotation.cmi ../asttools/ast_accessors.cmo build.cmi 
build.cmx: ../elsa/oast_header.cmx ../elsa/ml_ctype.cmx cfg_util.cmx \
    cfg_type.cmx ../elsa/cc_ml_types.cmx ../elsa/cc_ast_gen_type.cmx \
    ../elsa/ast_annotation.cmx ../asttools/ast_accessors.cmx build.cmi 
cfg_type.cmo: ../elsa/cc_ml_types.cmo 
cfg_type.cmx: ../elsa/cc_ml_types.cmx 
cfg_util.cmo: ../asttools/superast.cmi cfg_type.cmo \
    ../elsa/cc_ast_gen_type.cmo ../elsa/ast_annotation.cmi cfg_util.cmi 
cfg_util.cmx: ../asttools/superast.cmx cfg_type.cmx \
    ../elsa/cc_ast_gen_type.cmx ../elsa/ast_annotation.cmx cfg_util.cmi 
dot.cmo: cfg_type.cmo dot.cmi 
dot.cmx: cfg_type.cmx dot.cmi 
main.cmo: dot.cmi build.cmi 
main.cmx: dot.cmx build.cmx 
build.cmi: cfg_type.cmo 
cfg_util.cmi: cfg_type.cmo ../elsa/cc_ast_gen_type.cmo \
    ../elsa/ast_annotation.cmi 
dot.cmi: cfg_type.cmo 
