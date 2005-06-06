  /* astxml_lexer0_mid.lex            see license.txt for copyright and terms of use
   * flex description of scanner for C and C++ souce
   */

  /* This file is the mid part of the generated .lex file.  This
     will be REPLACED with a generated file. */

  /* make one of these for every AST node */
"TranslationUnit"  return tok(XTOK_TRANSLATION_UNIT);
"Foo"              return tok(XTOK_FOO);

  /* make one of these for every child name */
".id"              return tok(XTOK_DOT_ID);
"topForms"         return tok(XTOK_TOPFORMS);
