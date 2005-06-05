/* astxml_lexer.lex            see license.txt for copyright and terms of use
 * flex description of scanner for C and C++ souce
 */

/* ----------------------- C definitions ---------------------- */
%{

#include "astxml_lexer.h"       // Lexer class

// this works around a problem with cygwin & fileno
#define YY_NEVER_INTERACTIVE 1

%}


/* -------------------- flex options ------------------ */
/* no wrapping is needed; setting this means we don't have to link with libfl.a */
%option noyywrap

/* don't use the default-echo rules */
%option nodefault

/* I don't call unput */
%option nounput

/* generate a c++ lexer */
%option c++

/* use the "fast" algorithm with no table compression */
%option full

/* utilize character equivalence classes */
%option ecs

/* the scanner is never interactive */
%option never-interactive

/* and I will define the class (lexer.h) */
%option yyclass="AstXmlLexer"

/* output file name */
  /* dsw: Arg!  Don't do this, since it is not overrideable from the
     command line with -o, which I consider to be a flex bug. */
  /* sm: fair enough; agreed */
  /* %option outfile="lexer.yy.cc" */

/* start conditions */
%x BUGGY_STRING_LIT

/* ------------------- definitions -------------------- */
/* newline */
NL            "\n"

/* anything but newline */
NOTNL         .

/* any of 256 source characters */
ANY           ({NOTNL}|{NL})

/* backslash */
BACKSL        "\\"

/* beginnging of line (must be start of a pattern) */
BOL           ^

/* end of line (would like EOF to qualify also, but flex doesn't allow it */
EOL           {NL}

/* letter or underscore */
LETTER        [A-Za-z_]

/* letter or underscore or digit */
ALNUM         [A-Za-z_0-9]

/* decimal digit */
DIGIT         [0-9]
HEXDIGIT      [0-9A-Fa-f]

/* sequence of decimal digits */
DIGITS        ({DIGIT}+)
/* sequence of hex digits */
HEXDIGITS     ({HEXDIGIT}+)

/* sign of a number */
SIGN          ("+"|"-")

/* integer suffix */
/* added 'LL' option for gcc/c99 long long compatibility */
ELL_SUFFIX    [lL]([lL]?)
INT_SUFFIX    ([uU]{ELL_SUFFIX}?|{ELL_SUFFIX}[uU]?)

/* floating-point suffix letter */
FLOAT_SUFFIX  [flFL]

/* normal string character: any but quote, newline, or backslash */
STRCHAR       [^\"\n\\]

/* (start of) an escape sequence */
ESCAPE        ({BACKSL}{ANY})

/* double quote */
QUOTE         [\"]

/* normal character literal character: any but single-quote, newline, or backslash */
CCCHAR        [^\'\n\\]

/* single quote */
TICK          [\']

/* space or tab */
SPTAB         [ \t]

/* preprocessor "character" -- any but escaped newline */
PPCHAR        ([^\\\n]|{BACKSL}{NOTNL})


/* ------------- token definition rules --------------- */
%%

  /* operators, punctuators and keywords: tokens with one spelling */
"<"                return tok(XTOK_LESSTHAN);
">"                return tok(XTOK_GREATERTHAN);
"/"                return tok(XTOK_SLASH);

"TranslationUnit"  return tok(XTOK_TRANSLATION_UNIT);
"Foo"              return tok(XTOK_FOO);
  /* This doesn't work!  Makes lexer jam?? */
  /* ".id"              return tok(XTOK_DOT_ID); */
"topForms"         return tok(XTOK_TOPFORMS);


  /* identifier: e.g. foo */
{LETTER}{ALNUM}* {
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


  /* This scanner reads in a string literal that contains unescaped
   * newlines, to support a gcc-2 bug.  The strategy is to emit a
   * sequence of XTOK_STRING_LITERALs, as if the string had been
   * properly broken into multiple literals.  However, these literals
   * aren't consistently surrounded by quotes... */
<BUGGY_STRING_LIT>{
  ({STRCHAR}|{ESCAPE})*{QUOTE} {
    // found the end
    BEGIN(INITIAL);
    return svalTok(XTOK_STRING_LITERAL);
  }
  ({STRCHAR}|{ESCAPE})*{EOL} {
    // another line
    return svalTok(XTOK_STRING_LITERAL);
  }
  <<EOF>> |
  ({STRCHAR}|{ESCAPE})*{BACKSL}? {
    // unterminated (this only matches at EOF)
    err("at EOF, unterminated string literal; support for newlines in string "
        "literals is presently turned on, maybe the missing quote should have "
        "been much earlier in the file?");
    yyterminate();
  }
}


  /* whitespace */
  /* 10/20/02: added '\r' to accomodate files coming from Windows; this
   * could be seen as part of the mapping from physical source file
   * characters to the basic character set (cppstd 2.1 para 1 phase 1),
   * except that it doesn't happen for chars in string/char literals... */
[ \t\n\f\v\r]+  {
  whitespace();
}

  /* C++ comment */
  /* we don't match the \n because that way this works at EOF */
"//"{NOTNL}*    {
  whitespace();
}

  /* illegal */
.  {
  /* updLoc(); */
  err(stringc << "illegal character: `" << yytext[0] << "'");
}

<<EOF>> {
  srcFile->doneAdding();
  yyterminate();
}


%%
