  /* gnu.lex
   * extension to cc.lex, for GNU extensions
   */

"__builtin_constant_p" return tok(TOK_BUILTIN_CONSTANT_P);
"__alignof__"          return tok(TOK___ALIGNOF__);

"__attribute"          return tok(TOK___ATTRIBUTE__);
"__attribute__"        return tok(TOK___ATTRIBUTE__);
"__FUNCTION__"         return tok(TOK___FUNCTION__);
"__label__"            return tok(TOK___LABEL__);
"__PRETTY_FUNCTION__"  return tok(TOK___PRETTY_FUNCTION__);
"typeof"               return tok(TOK___TYPEOF__);
"__typeof"             return tok(TOK___TYPEOF__);
"__typeof__"           return tok(TOK___TYPEOF__);

"__extension__" {
  /* treat this like a token, in that nonseparating checks are done,
   * but don't yield it to the parser */
  (void)tok(TOK___EXTENSION__);
}
