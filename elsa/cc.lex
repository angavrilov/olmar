/* cc.lex
 * flex description of scanner for C and C++ souce
 */
 
/* ----------------------- C definitions ---------------------- */
%{

#include "lexer.h"       // Lexer class

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

/* and I will define the class */
%option yyclass="Lexer"

/* output file name */
  /* dsw: Arg!  Don't do this, since it is not overrideable from the
     command line with -o, which I consider to be a flex bug. */
  /* %option outfile="lexer.yy.cc" */


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

/* sequence of decimal digits */
DIGITS        ({DIGIT}+)

/* sign of a number */
SIGN          ("+"|"-")

/* integer suffix */
/* added 'LL' option for GNU long long compatibility.. */
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

  /* this comment is replaced, by an external script, with whatever
   * additional rules a language extension author desires */
  /* EXTENSION RULES GO HERE */


  /* operators, punctuators and keywords: tokens with one spelling */
"asm"              return tok(TOK_ASM);
"__asm__"          return tok(TOK_ASM);
"auto"             return tok(TOK_AUTO);
"break"            return tok(TOK_BREAK);
"bool"             return tok(TOK_BOOL);
"case"             return tok(TOK_CASE);
"catch"            return tok(TOK_CATCH);
"cdecl"            return tok(TOK_CDECL);
"char"             return tok(TOK_CHAR);
"class"            return tok(TOK_CLASS);
"const"            return tok(TOK_CONST);
"__const"          return tok(TOK_CONST);
"__const__"        return tok(TOK_CONST);
"const_cast"       return tok(TOK_CONST_CAST);
"continue"         return tok(TOK_CONTINUE);
"default"          return tok(TOK_DEFAULT);
"delete"           return tok(TOK_DELETE);
"do"               return tok(TOK_DO);
"double"           return tok(TOK_DOUBLE);
"dynamic_cast"     return tok(TOK_DYNAMIC_CAST);
"else"             return tok(TOK_ELSE);
"enum"             return tok(TOK_ENUM);
"explicit"         return tok(TOK_EXPLICIT);
"export"           return tok(TOK_EXPORT);
"extern"           return tok(TOK_EXTERN);
"false"            return tok(TOK_FALSE);
"float"            return tok(TOK_FLOAT);
"for"              return tok(TOK_FOR);
"friend"           return tok(TOK_FRIEND);
"goto"             return tok(TOK_GOTO);
"if"               return tok(TOK_IF);
"inline"           return tok(TOK_INLINE);
"__inline"         return tok(TOK_INLINE);
"__inline__"       return tok(TOK_INLINE);
"int"              return tok(TOK_INT);
"long"             return tok(TOK_LONG);
"mutable"          return tok(TOK_MUTABLE);
"namespace"        return tok(TOK_NAMESPACE);
"new"              return tok(TOK_NEW);
"operator"         return tok(TOK_OPERATOR);
"pascal"           return tok(TOK_PASCAL);
"private"          return tok(TOK_PRIVATE);
"protected"        return tok(TOK_PROTECTED);
"public"           return tok(TOK_PUBLIC);
"register"         return tok(TOK_REGISTER);
"reinterpret_cast" return tok(TOK_REINTERPRET_CAST);
"return"           return tok(TOK_RETURN);
"short"            return tok(TOK_SHORT);
"signed"           return tok(TOK_SIGNED);
"__signed__"       return tok(TOK_SIGNED);
"sizeof"           return tok(TOK_SIZEOF);
"static"           return tok(TOK_STATIC);
"static_cast"      return tok(TOK_STATIC_CAST);
"struct"           return tok(TOK_STRUCT);
"switch"           return tok(TOK_SWITCH);
"template"         return tok(TOK_TEMPLATE);
"this"             return tok(TOK_THIS);
"throw"            return tok(TOK_THROW);
"true"             return tok(TOK_TRUE);
"try"              return tok(TOK_TRY);
"typedef"          return tok(TOK_TYPEDEF);
"typeid"           return tok(TOK_TYPEID);
"typename"         return tok(TOK_TYPENAME);
"union"            return tok(TOK_UNION);
"unsigned"         return tok(TOK_UNSIGNED);
"using"            return tok(TOK_USING);
"virtual"          return tok(TOK_VIRTUAL);
"void"             return tok(TOK_VOID);
"volatile"         return tok(TOK_VOLATILE);
"__volatile__"     return tok(TOK_VOLATILE);
"wchar_t"          return tok(TOK_WCHAR_T);
"while"            return tok(TOK_WHILE);

"("                return tok(TOK_LPAREN);
")"                return tok(TOK_RPAREN);
"["                return tok(TOK_LBRACKET);
"]"                return tok(TOK_RBRACKET);
"->"               return tok(TOK_ARROW);
"::"               return tok(TOK_COLONCOLON);
"."                return tok(TOK_DOT);
"!"                return tok(TOK_BANG);
"~"                return tok(TOK_TILDE);
"+"                return tok(TOK_PLUS);
"-"                return tok(TOK_MINUS);
"++"               return tok(TOK_PLUSPLUS);
"--"               return tok(TOK_MINUSMINUS);
"&"                return tok(TOK_AND);
"*"                return tok(TOK_STAR);
".*"               return tok(TOK_DOTSTAR);
"->*"              return tok(TOK_ARROWSTAR);
"/"                return tok(TOK_SLASH);
"%"                return tok(TOK_PERCENT);
"<<"               return tok(TOK_LEFTSHIFT);
">>"               return tok(TOK_RIGHTSHIFT);
"<"                return tok(TOK_LESSTHAN);
"<="               return tok(TOK_LESSEQ);
">"                return tok(TOK_GREATERTHAN);
">="               return tok(TOK_GREATEREQ);
"=="               return tok(TOK_EQUALEQUAL);
"!="               return tok(TOK_NOTEQUAL);
"^"                return tok(TOK_XOR);
"|"                return tok(TOK_OR);
"&&"               return tok(TOK_ANDAND);
"||"               return tok(TOK_OROR);
"?"                return tok(TOK_QUESTION);
":"                return tok(TOK_COLON);
"="                return tok(TOK_EQUAL);
"*="               return tok(TOK_STAREQUAL);
"/="               return tok(TOK_SLASHEQUAL);
"%="               return tok(TOK_PERCENTEQUAL);
"+="               return tok(TOK_PLUSEQUAL);
"-="               return tok(TOK_MINUSEQUAL);
"&="               return tok(TOK_ANDEQUAL);
"^="               return tok(TOK_XOREQUAL);
"|="               return tok(TOK_OREQUAL);
"<<="              return tok(TOK_LEFTSHIFTEQUAL);
">>="              return tok(TOK_RIGHTSHIFTEQUAL);
","                return tok(TOK_COMMA);
"..."              return tok(TOK_ELLIPSIS);
";"                return tok(TOK_SEMICOLON);
"{"                return tok(TOK_LBRACE);
"}"                return tok(TOK_RBRACE);

  /* this rule is to avoid backing up in the lexer 
   * when there are two dots but not three */
".." {
  yyless(1);     /* put back all but 1; this is inexpensive */
  return tok(TOK_DOT);
}

  /* identifier: e.g. foo */
{LETTER}{ALNUM}* {
  return svalTok(TOK_NAME);
}

  /* integer literal; dec, oct, or hex */
[1-9][0-9]*{INT_SUFFIX}?           |
[0][0-7]*{INT_SUFFIX}?             |
[0][xX][0-9A-Fa-f]+{INT_SUFFIX}?   {
  return svalTok(TOK_INT_LITERAL);
}

  /* hex literal with nothing after the 'x' */
[0][xX] {
  err("hexadecimal literal with nothing after the 'x'");
  return svalTok(TOK_INT_LITERAL);
}

  /* floating literal */
{DIGITS}"."{DIGITS}?([eE]{SIGN}?{DIGITS})?{FLOAT_SUFFIX}?   |
{DIGITS}"."?([eE]{SIGN}?{DIGITS})?{FLOAT_SUFFIX}?	    |
"."{DIGITS}([eE]{SIGN}?{DIGITS})?{FLOAT_SUFFIX}?	    {
  return svalTok(TOK_FLOAT_LITERAL);
}

  /* floating literal with no digits after the 'e' */
{DIGITS}"."{DIGITS}?[eE]{SIGN}?   |
{DIGITS}"."?[eE]{SIGN}?           |
"."{DIGITS}[eE]{SIGN}?            {
  err("floating literal with no digits after the 'e'");
  
  /* in recovery rules like this it's best to yield the best-guess
   * token type, instead of some TOK_ERROR, so the parser can still
   * try to make sense of the input; having reported the error is
   * sufficient to ensure that later stages won't try to interpret
   * the lexical text of this token as a floating literal */
  return svalTok(TOK_FLOAT_LITERAL);
}

  /* string literal */
"L"?{QUOTE}({STRCHAR}|{ESCAPE})*{QUOTE} {
  return svalTok(TOK_STRING_LITERAL);
}

  /* string literal missing final quote */
"L"?{QUOTE}({STRCHAR}|{ESCAPE})*{EOL}   {
  err("string literal missing final `\"'");
  return svalTok(TOK_STRING_LITERAL);     // error recovery
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


  /* character literal */
"L"?{TICK}({CCCHAR}|{ESCAPE})*{TICK}   {
  return svalTok(TOK_CHAR_LITERAL);
}

  /* character literal missing final tick */
"L"?{TICK}({CCCHAR}|{ESCAPE})*{EOL}    {
  err("character literal missing final \"'\"");
  return svalTok(TOK_CHAR_LITERAL);       // error recovery
}

  /* unterminated character literal */
"L"?{TICK}({CCCHAR}|{ESCAPE})*{BACKSL}?  {
  err("unterminated character literal");
  yyterminate();
}


  /* sm: I moved the user-defined qualifier rule into qual_ext.lex
   * in the cc_qual tree */


  /* preprocessor */
  /* technically, if this isn't at the start of a line (possibly after
   * some whitespace), it should be an error.. I'm not sure right now how
   * I want to deal with that (I originally was using '^', but that
   * interacts badly with the whitespace rule) */

  /* #line directive: the word "line" is optional, then a space, and
   * then we accept the rest of the line; 'parseHashLine' will finish
   * parsing the directive */
"#"("line"?){SPTAB}.*{NL} {
  parseHashLine(yytext, yyleng);
  whitespace();       // don't increment line count until after parseHashLine()
}

  /* other preprocessing: ignore it */
  /* trailing optional baskslash to avoid backing up */
"#"{PPCHAR}*({BACKSL}{NL}{PPCHAR}*)*{BACKSL}?   {
  // treat it like whitespace, ignoring it otherwise
  whitespace();
}

  /* whitespace */
  /* 10/20/02: added '\r' to accomodate files coming from Windows */
[ \t\n\f\v\r]+  {
  whitespace();
}

  /* C++ comment */
  /* we don't match the \n because that way this works at EOF */
"//"{NOTNL}*    {
  whitespace();
}

  /* C comment */
"/""*"([^*]|"*"[^/])*"*/"     {
  // TODO: I think this fails to match /***/
  whitespace();
}

  /* unterminated C comment */
"/""*"([^*]|"*"[^/])*"*"     |
"/""*"([^*]|"*"[^/])*        {
  err("unterminated /**/ comment");
  whitespace();
}

  /* illegal */
.  {
  err(stringc << "illegal character: `" << yytext[0] << "'");
}

<<EOF>> {
  srcFile->doneAdding();
  yyterminate();
}


%%
/**************/
/* extra code */
/**************/



