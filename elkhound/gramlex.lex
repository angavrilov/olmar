/* grammar.lex
 * lexical analyzer for my grammar input format
 */

/* ----------------- C definitions -------------------- */
%{

// pull in the bison-generated token codes
#include "grampar.tab.h"

// pull in my declaration of the lexer class -- this defines
// the additional lexer state, some of which is used in the
// action rules below
#include "gramlex.h"

// for maintaining column count
#define UPD_COL   column += yyleng;

%}


/* -------------------- flex options ------------------ */
/* no wrapping is needed; setting this means we don't have to link with libfl.a */
%option noyywrap

/* don't use the default-echo rules */
%option nodefault

/* generate a c++ lexer */
%option c++

/* and I will define the class */
%option yyclass="GrammarLexer"


/* ------------------- definitions -------------------- */
/* any character, including newline */
ANY       (.|"\n")

/* any character except newline */
ANYBUTNL  .

/* starting character in a name */
LETTER    [a-zA-Z_"]

/* starting character in a numeric literal */
DIGIT     [0-9]


/* --------------- start conditions ------------------- */
%x C_COMMENT
%x INCLUDE
%x EAT_TO_NEWLINE


/* ---------------------- rules ----------------------- */
%%

  /* -------- whitespace ------ */
"\n" {
  newLine();
}

[ \t\f\v]+ {
  UPD_COL
}

  /* -------- comments -------- */
"/*" {
  /* C-style comments */
  UPD_COL
  commentStartLine = line;
  BEGIN(C_COMMENT);
}

<C_COMMENT>{
  "*/" {
    /* end of comment */
    UPD_COL
    BEGIN(INITIAL);
  }

  . {
    /* anything but slash-star or newline -- eat it */
    UPD_COL
  }

  "\n" {
    newLine();
  }

  <<EOF>> {
    errorUnterminatedComment();
    return TOK_EOF;
  }
}


"//".*"\n" {
  /* C++-style comment -- eat it */
  newLine();
}


  /* -------- punctuators, operators, keywords --------- */
"{"                UPD_COL  return TOK_LBRACE;
"}"                UPD_COL  return TOK_RBRACE;
":"                UPD_COL  return TOK_COLON;
";"                UPD_COL  return TOK_SEMICOLON;
"->"               UPD_COL  return TOK_ARROW;
"|"                UPD_COL  return TOK_VERTBAR;
":="               UPD_COL  return TOK_COLONEQUALS;
"("                UPD_COL  return TOK_LPAREN;
")"                UPD_COL  return TOK_RPAREN;

"||"               UPD_COL  return TOK_OROR;
"&&"               UPD_COL  return TOK_ANDAND;
"!="               UPD_COL  return TOK_NOTEQUAL;
"=="               UPD_COL  return TOK_EQUALEQUAL;
">="               UPD_COL  return TOK_GREATEREQ;
"<="               UPD_COL  return TOK_LESSEQ;
">"                UPD_COL  return TOK_GREATER;
"<"                UPD_COL  return TOK_LESS;
"-"                UPD_COL  return TOK_MINUS;
"+"                UPD_COL  return TOK_PLUS;
"%"                UPD_COL  return TOK_PERCENT;
"/"                UPD_COL  return TOK_SLASH;
"*"                UPD_COL  return TOK_ASTERISK;

"terminals"        UPD_COL  return TOK_TERMINALS;
"nonterm"          UPD_COL  return TOK_NONTERM;
"formGroup"        UPD_COL  return TOK_FORMGROUP;
"form"             UPD_COL  return TOK_FORM;
"action"           UPD_COL  return TOK_ACTION;
"condition"        UPD_COL  return TOK_CONDITION;


  /* -------- name literal --------- */
{LETTER}({LETTER}|{DIGIT})* {
  /* get text from yytext and yyleng */
  UPD_COL
  return TOK_NAME;
}

  /* -------- numeric literal ------ */
{DIGIT}+ {
  UPD_COL
  integerLiteral = strtoul(yytext, NULL, 10 /*radix*/);
  return TOK_INTEGER;
}

  /* ---------- includes ----------- */
"#include" {
  UPD_COL
  BEGIN(INCLUDE);
}

<INCLUDE>{
  [ \t]* {
    /* bypass same-line whitespace */
    UPD_COL
  }

  "\""[^"\n]+"\"" {
    /* e.g.: "filename" */
    /* file name to include */
    UPD_COL
    includeFileName = string(yytext+1, yyleng-2);    // strip quotes
    BEGIN(INITIAL);
    return TOK_INCLUDE;
  }

  {ANY}      {
    /* anything else: malformed */
    UPD_COL
    errorMalformedInclude();
    BEGIN(EAT_TO_NEWLINE);
  }
}

<EAT_TO_NEWLINE>{
  .+ {
    UPD_COL
    /* not newline, eat it */
  }

  "\n" {
    /* get out of here */
    newLine();
    BEGIN(INITIAL);
  }
}


  /* --------- illegal ------------- */
{ANY} {
  UPD_COL
  errorIllegalCharacter(yytext[0]);
}


%%
/* -------------------- additional C code -------------------- */
