cc_ml_constructors.cmo: cc_ml_types.cmo 
cc_ml_constructors.cmx: cc_ml_types.cmx 
cc_ast_gen_type.cmo: cc_ml_types.cmo 
cc_ast_gen_type.cmx: cc_ml_types.cmx 
cc_ast_gen.cmo: cc_ast_gen_type.cmo 
cc_ast_gen.cmx: cc_ast_gen_type.cmx 
ast_marshal.cmo: cc_ast_gen_type.cmo 
ast_marshal.cmx: cc_ast_gen_type.cmx 
caml_callbacks.cmo: cc_ml_constructors.cmo cc_ast_gen.cmo ast_marshal.cmo 
caml_callbacks.cmx: cc_ml_constructors.cmx cc_ast_gen.cmx ast_marshal.cmx 
