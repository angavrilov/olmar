cc_ml_types.cmo: elsa_util.cmo 
cc_ml_types.cmx: elsa_util.cmx 
cc_ml_constructors.cmo: cc_ml_types.cmo 
cc_ml_constructors.cmx: cc_ml_types.cmx 
ml_ctype.cmo: cc_ml_types.cmo 
ml_ctype.cmx: cc_ml_types.cmx 
cc_ast_gen_type.cmo: ml_ctype.cmo cc_ml_types.cmo 
cc_ast_gen_type.cmx: ml_ctype.cmx cc_ml_types.cmx 
cc_ast_gen.cmo: cc_ast_gen_type.cmo 
cc_ast_gen.cmx: cc_ast_gen_type.cmx 
ml_ctype_constructors.cmo: ml_ctype.cmo cc_ast_gen_type.cmo 
ml_ctype_constructors.cmx: ml_ctype.cmx cc_ast_gen_type.cmx 
ast_marshal.cmo: ml_ctype.cmo cc_ast_gen_type.cmo ast_annotation.cmo 
ast_marshal.cmx: ml_ctype.cmx cc_ast_gen_type.cmx ast_annotation.cmx 
caml_callbacks.cmo: ml_ctype_constructors.cmo cc_ml_constructors.cmo \
    cc_ast_gen.cmo ast_marshal.cmo ast_annotation.cmo 
caml_callbacks.cmx: ml_ctype_constructors.cmx cc_ml_constructors.cmx \
    cc_ast_gen.cmx ast_marshal.cmx ast_annotation.cmx 
