/* grammar.lex
 * lexical analyzer for my grammar input format
 */

/* ----------------- C definitions -------------------- */
%{

// pull in my declaration of the lexer class -- this defines
// the additional lexer state, some of which is used in the
// action rules below (this is in the ../ast/ directory now)
#include "gramlex.h"

// pull in the bison-generated token codes
#include "grampar.codes.h"

#include <string.h>     // strchr, strrchr

// for maintaining column count
#define TOKEN_START  tokenStartLoc = fileState /* user ; */
#define UPD_COL      fileState.col += yyleng  /* user ; */
#define TOK_UPD_COL  TOKEN_START; UPD_COL  /* user ; */

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
LETTER    [a-zA-Z_]

/* starting character in a numeric literal */
DIGIT     [0-9]

/* double-quote */
DQUOTE    "\""

/* character that can appear in a quoted string */
/* (I currently don't have any backslash codes, but I want to
 * leave open that possibility, so for now backslashes are illegal) */
STRCHR    [^\n\\\"]

/* whitespace that doesn't cross line a boundary */
SLWHITE   [ \t]


/* --------------- start conditions ------------------- */
%x C_COMMENT
%x INCLUDE
%x EAT_TO_NEWLINE
%x LITCODE


/* ---------------------- rules ----------------------- */
%%

  /* -------- whitespace ------ */
"\n" {
  newLine();
}

[ \t\f\v]+ {
  UPD_COL;
}

  /* -------- comments -------- */
"/""*" {
  /* C-style comments */
  TOKEN_START;
  UPD_COL;
  //commentStartLine = fileState.line;
  BEGIN(C_COMMENT);
}

<C_COMMENT>{
  "*/" {
    /* end of comment */
    UPD_COL;
    BEGIN(INITIAL);
  }

  . {
    /* anything but slash-star or newline -- eat it */
    UPD_COL;
  }

  "\n" {
    newLine();
  }

  <<EOF>> {
    UPD_COL;      // <<EOF>> yyleng is 1!
    errorUnterminatedComment();
    return TOK_EOF;
  }
}


"//".*"\n" {
  /* C++-style comment -- eat it */
  TOKEN_START;
  newLine();
}


  /* -------- punctuators, operators, keywords --------- */
"{"                TOK_UPD_COL;  return TOK_LBRACE;
"}"                TOK_UPD_COL;  return TOK_RBRACE;
":"                TOK_UPD_COL;  return TOK_COLON;
";"                TOK_UPD_COL;  return TOK_SEMICOLON;
"->"               TOK_UPD_COL;  return TOK_ARROW;
"("                TOK_UPD_COL;  return TOK_LPAREN;
")"                TOK_UPD_COL;  return TOK_RPAREN;
","                TOK_UPD_COL;  return TOK_COMMA;

"terminals"        TOK_UPD_COL;  return TOK_TERMINALS;
"token"            TOK_UPD_COL;  return TOK_TOKEN;
"nonterm"          TOK_UPD_COL;  return TOK_NONTERM;
"verbatim"         TOK_UPD_COL;  return TOK_VERBATIM;
"precedence"       TOK_UPD_COL;  return TOK_PRECEDENCE;
"option"           TOK_UPD_COL;  return TOK_OPTION;
"expect"           TOK_UPD_COL;  return TOK_EXPECT;

  /* --------- embedded literal code --------- */
"[" {
  TOK_UPD_COL;
  BEGIN(LITCODE);
  embedded->reset();
  embedFinish = ']';
  embedMode = TOK_LIT_CODE;
}

  /* no TOKEN_START here; we'll use the tokenStartLoc that
   * was computed in the opening punctuation */
<LITCODE>{
  [^\]\n]+ {
    UPD_COL;
    embedded->handle(yytext, yyleng, embedFinish);
  }

  "\n" {
    newLine();
    embedded->handle(yytext, yyleng, embedFinish);
  }

  "]" {
    UPD_COL;
    if (embedded->zeroNesting()) {
      // done
      BEGIN(INITIAL);

      // don't add "return" or ";"
      embedded->exprOnly = false;
      
      // can't extract anything
      embedded->isDeclaration = false;

      // caller can get text from embedded->text
      return embedMode;
    }
    else {
      // delimeter paired within the embedded code, mostly ignore it
      embedded->handle(yytext, yyleng, embedFinish);
    }
  }
}


  /* ---------- includes ----------- */
"include" {
  TOK_UPD_COL;    /* hence no TOKEN_START in INCLUDE area */
  BEGIN(INCLUDE);
}

<INCLUDE>{
  {SLWHITE}*"("{SLWHITE}*{DQUOTE}{STRCHR}+{DQUOTE}{SLWHITE}*")" {
    /* e.g.: ("filename") */
    /* file name to include */
    UPD_COL;

    /* find quotes */
    char *leftq = strchr(yytext, '"');
    char *rightq = strchr(leftq+1, '"');
    xassert(leftq && rightq);

    /* extract filename string */
    includeFileName = addString(leftq+1, rightq-leftq-1);

    /* go back to normal processing */
    BEGIN(INITIAL);
    return TOK_INCLUDE;
  }

  {ANY}      {
    /* anything else: malformed */
    UPD_COL;
    errorMalformedInclude();

    /* rudimentary error recovery.. */
    BEGIN(EAT_TO_NEWLINE);
  }
}

<EAT_TO_NEWLINE>{
  .+ {
    UPD_COL;
    /* not newline, eat it */
  }

  "\n" {
    /* get out of here */
    newLine();
    BEGIN(INITIAL);
  }
}

  /* -------- name literal --------- */
{LETTER}({LETTER}|{DIGIT})* {
  /* get text from yytext and yyleng */
  TOK_UPD_COL;
  return TOK_NAME;
}

  /* -------- numeric literal ------ */
{DIGIT}+ {
  TOK_UPD_COL;
  integerLiteral = strtoul(yytext, NULL, 10 /*radix*/);
  return TOK_INTEGER;
}

  /* ----------- string literal ----- */
{DQUOTE}{STRCHR}*{DQUOTE} {
  TOK_UPD_COL;
  stringLiteral = addString(yytext+1, yyleng-2);        // strip quotes
  return TOK_STRING;
}

  /* --------- illegal ------------- */
{ANY} {
  TOK_UPD_COL;
  errorIllegalCharacter(yytext[0]);
}


%%
/* -------------------- additional C code -------------------- */

// identify tokens representing embedded text
bool isGramlexEmbed(int code)
{
  return code == TOK_LIT_CODE;
}
