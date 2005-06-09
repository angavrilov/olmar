  /* astxml_lexer1_bottom.lex            see license.txt for copyright and terms of use
   * flex description of scanner for C and C++ souce
   */

  /* This file is the bottom part of the generated .lex file. */

  /* identifier: e.g. foo */
{LETTERDOT}{ALNUMDOT}* {
  return svalTok(XTOK_NAME);
}

  /* integer literal; dec, oct, or hex */
[1-9][0-9]*{INT_SUFFIX}?           |
[0][0-7]*{INT_SUFFIX}?             |
[0][xX][0-9A-Fa-f]+{INT_SUFFIX}?   {
  return svalTok(XTOK_INT_LITERAL);
}

  /* hex literal with nothing after the 'x' */
[0][xX] {
  err("hexadecimal literal with nothing after the 'x'");
  return svalTok(XTOK_INT_LITERAL);
}

  /* string literal */
"L"?{QUOTE}({STRCHAR}|{ESCAPE})*{QUOTE} {
  return svalTok(XTOK_STRING_LITERAL);
}

  /* string literal missing final quote */
"L"?{QUOTE}({STRCHAR}|{ESCAPE})*{EOL}   {
    err("string literal missing final `\"'");
    return svalTok(XTOK_STRING_LITERAL);     // error recovery
}

  /* unterminated string literal; maximal munch causes us to prefer
   * either of the above two rules when possible; the trailing
   * optional backslash is needed so the scanner won't back up in that
   * case; NOTE: this can only happen if the file ends in the string
   * and there is no newline before the EOF */
"L"?{QUOTE}({STRCHAR}|{ESCAPE})*{BACKSL}? {
  err("unterminated string literal");
  yyterminate();
}


  /* whitespace */
  /* 10/20/02: added '\r' to accomodate files coming from Windows; this
   * could be seen as part of the mapping from physical source file
   * characters to the basic character set (cppstd 2.1 para 1 phase 1),
   * except that it doesn't happen for chars in string/char literals... */
[ \t\n\f\v\r]+  {
  /*    whitespace(); */
}

  /* C++ comment */
  /* we don't match the \n because that way this works at EOF */
"//"{NOTNL}*    {
  /*    whitespace(); */
}

  /* illegal */
.  {
  /* updLoc(); */
  err(stringc << "illegal character: `" << yytext[0] << "'");
}

<<EOF>> {
  /*    srcFile->doneAdding(); */
  yyterminate();
}


%%
