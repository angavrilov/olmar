  /* gnu.lex
   * extension to cc.lex, for GNU extensions
   */

"__builtin_constant_p" return tok(TOK_BUILTIN_CONSTANT_P);
"__alignof__"          return tok(TOK___ALIGNOF__);

"__attribute"          return tok(TOK___ATTRIBUTE__);
"__attribute__"        return tok(TOK___ATTRIBUTE__);
"__label__"            return tok(TOK___LABEL__);
"typeof"               return tok(TOK___TYPEOF__);
"__typeof"             return tok(TOK___TYPEOF__);
"__typeof__"           return tok(TOK___TYPEOF__);
"__restrict__"         return tok(TOK___RESTRICT__);
"__builtin_expect"     return tok(TOK___BUILTIN_EXPECT);

  /* behavior of these depends on CCLang settings */
"__FUNCTION__"|"__PRETTY_FUNCTION__" {
  if (lang.gccFuncBehavior == CCLang::GFB_string) {
    // yield with special token codes
    return tok(yytext[2]=='F'? TOK___FUNCTION__ : TOK___PRETTY_FUNCTION__);
  }
  else {
    // ordinary identifier, possibly predefined (but that's not the
    // lexer's concern)
    return svalTok(TOK_NAME);
  }
}

"__extension__" {
  /* treat this like a token, in that nonseparating checks are done,
   * but don't yield it to the parser */
  (void)tok(TOK___EXTENSION__);
}
