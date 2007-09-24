ast_accessors_header.cmo: ../../build/astgen/ast_reflect_type.cmx 
ast_accessors_header.cmx: ../../build/astgen/ast_reflect_type.cmx 
ast_accessors.cmo: ../../build/astgen/ast_reflect_type.cmx 
ast_accessors.cmx: ../../build/astgen/ast_reflect_type.cmx 
superast_generated.cmo: superast_generated.cmi 
superast_generated.cmx: superast_generated.cmi 
superast_header.cmo: ../../util/dense_set.cmi \
    ../../build/astgen/ast_reflect_type.cmx \
    ../../build/general/ast_annotation.cmi ast_accessors.cmx \
    superast_header.cmi 
superast_header.cmx: ../../util/dense_set.cmx \
    ../../build/astgen/ast_reflect_type.cmx \
    ../../build/general/ast_annotation.cmx ast_accessors.cmx \
    superast_header.cmi 
superast.cmo: ../../util/dense_set.cmi \
    ../../build/astgen/ast_reflect_type.cmx \
    ../../build/general/ast_annotation.cmi ast_accessors.cmx superast.cmi 
superast.cmx: ../../util/dense_set.cmx \
    ../../build/astgen/ast_reflect_type.cmx \
    ../../build/general/ast_annotation.cmx ast_accessors.cmx superast.cmi 
superast_trailer.cmo: ../../util/dense_set.cmi superast_trailer.cmi 
superast_trailer.cmx: ../../util/dense_set.cmx superast_trailer.cmi 
uplinks_header.cmo: superast.cmi ../../build/astgen/ast_reflect_type.cmx \
    ../../build/general/ast_annotation.cmi ast_accessors.cmx 
uplinks_header.cmx: superast.cmx ../../build/astgen/ast_reflect_type.cmx \
    ../../build/general/ast_annotation.cmx ast_accessors.cmx 
uplinks_trailer.cmo: superast.cmi 
uplinks_trailer.cmx: superast.cmx 
superast_header.cmi: ../../build/astgen/ast_reflect_type.cmx \
    ../../build/general/ast_annotation.cmi 
superast.cmi: ../../build/astgen/ast_reflect_type.cmx \
    ../../build/general/ast_annotation.cmi 
uplinks.cmi: superast.cmi ../../build/general/ast_annotation.cmi 
