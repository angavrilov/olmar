  /* gnu.lex
   * extension to cc.lex, for GNU extensions
   */

"__builtin_constant_p" return tok(TOK_BUILTIN_CONSTANT_P);
"__alignof"            return tok(TOK___ALIGNOF__);
"__alignof__"          return tok(TOK___ALIGNOF__);

"__attribute"          return tok(TOK___ATTRIBUTE__);
"__attribute__"        return tok(TOK___ATTRIBUTE__);
"__label__"            return tok(TOK___LABEL__);
"typeof"               return tok(TOK___TYPEOF__);
"__typeof"             return tok(TOK___TYPEOF__);
"__typeof__"           return tok(TOK___TYPEOF__);
"__builtin_expect"     return tok(TOK___BUILTIN_EXPECT);
"__builtin_va_arg"     return tok(TOK___BUILTIN_VA_ARG);

"__null" {
  // gcc only recognizes __null as 0 in C++ mode, but I prefer the
  // simplicity of always doing it; This is Scott's inlined and
  // modified Lexer::svalTok()
  checkForNonsep(TOK_INT_LITERAL);
  updLoc();
  sval = (SemanticValue)addString("0", 1);
  return TOK_INT_LITERAL;
}

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

  /* GNU alternate spellings; the versions with both leading and trailling
     underscores are explained here:
       http://gcc.gnu.org/onlinedocs/gcc-3.1/gcc/Alternate-Keywords.html
     But, I see no explanation for the ones with only leading underscores,
     though they occur in code in the wild so we support them...
   */
"__asm"                return tok(TOK_ASM);
"__asm__"              return tok(TOK_ASM);
"__const"              return tok(TOK_CONST);
"__const__"            return tok(TOK_CONST);
"__restrict"           return tok(TOK_RESTRICT);
"__restrict__"         return tok(TOK_RESTRICT);
"__inline"             return tok(TOK_INLINE);
"__inline__"           return tok(TOK_INLINE);
"__signed__"           return tok(TOK_SIGNED);
"__volatile"           return tok(TOK_VOLATILE);
"__volatile__"         return tok(TOK_VOLATILE);

  /* C99 stuff */
"restrict"             return tok(TOK_RESTRICT);

  /* EOF */
