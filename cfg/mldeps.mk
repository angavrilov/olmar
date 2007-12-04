build.cmo: ../olmar/util/more_string.cmi \
    ../olmar/build/elsa/elsa_reflect_type.cmo \
    ../olmar/build/elsa/elsa_oast_header.cmo \
    ../olmar/build/elsa/elsa_ml_flag_types.cmo cfg_util.cmi cfg_type.cmo \
    ../olmar/build/general/ast_annotation.cmi \
    ../olmar/meta/generated_elsa/ast_accessors.cmo build.cmi 
build.cmx: ../olmar/util/more_string.cmx \
    ../olmar/build/elsa/elsa_reflect_type.cmx \
    ../olmar/build/elsa/elsa_oast_header.cmx \
    ../olmar/build/elsa/elsa_ml_flag_types.cmx cfg_util.cmx cfg_type.cmx \
    ../olmar/build/general/ast_annotation.cmx \
    ../olmar/meta/generated_elsa/ast_accessors.cmx build.cmi 
cfg_type.cmo: ../olmar/build/elsa/elsa_ml_base_types.cmo 
cfg_type.cmx: ../olmar/build/elsa/elsa_ml_base_types.cmx 
cfg_util.cmo: ../olmar/meta/generated_elsa/superast.cmi \
    ../olmar/build/elsa/elsa_reflect_type.cmo cfg_type.cmo \
    ../olmar/build/general/ast_annotation.cmi cfg_util.cmi 
cfg_util.cmx: ../olmar/meta/generated_elsa/superast.cmx \
    ../olmar/build/elsa/elsa_reflect_type.cmx cfg_type.cmx \
    ../olmar/build/general/ast_annotation.cmx cfg_util.cmi 
dot.cmo: ../olmar/util/dot_graph.cmi cfg_util.cmi cfg_type.cmo dot.cmi 
dot.cmx: ../olmar/util/dot_graph.cmx cfg_util.cmx cfg_type.cmx dot.cmi 
main.cmo: dot.cmi cfg_util.cmi build.cmi 
main.cmx: dot.cmx cfg_util.cmx build.cmx 
build.cmi: cfg_type.cmo 
cfg_util.cmi: ../olmar/meta/generated_elsa/superast.cmi \
    ../olmar/build/elsa/elsa_reflect_type.cmo \
    ../olmar/build/elsa/elsa_ml_base_types.cmo cfg_type.cmo \
    ../olmar/build/general/ast_annotation.cmi 
dot.cmi: cfg_type.cmo 
