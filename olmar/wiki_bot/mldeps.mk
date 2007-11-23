form_values.cmo: form_values.cmi 
form_values.cmx: form_values.cmi 
wiki_bot.cmo: wiki_http.cmi ../util/stdlib_missing.cmi \
    ../util/more_string.cmi ../meta/meta_ast.cmi ../meta/ast_config.cmi 
wiki_bot.cmx: wiki_http.cmx ../util/stdlib_missing.cmx \
    ../util/more_string.cmx ../meta/meta_ast.cmx ../meta/ast_config.cmx 
wiki_http.cmo: netstring_ext.cmx form_values.cmi wiki_http.cmi 
wiki_http.cmx: netstring_ext.cmx form_values.cmx wiki_http.cmi 
