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
 * leave open that possibility, for now backslashes are illegal) */
STRCHR    [^\n\\"]

/* whitespace that doesn't cross line a boundary */
SLWHITE   [ \t]


/* --------------- start conditions ------------------- */
%x C_COMMENT
%x INCLUDE
%x EAT_TO_NEWLINE
%x FUNDECL
%x FUN


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
"/*" {
  /* C-style comments */
  TOKEN_START;
  UPD_COL;
  commentStartLine = fileState.line;
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
"}"                TOK_UPD_COL;  return TOK_RBRACE;
":"                TOK_UPD_COL;  return TOK_COLON;
";"                TOK_UPD_COL;  return TOK_SEMICOLON;
"->"               TOK_UPD_COL;  return TOK_ARROW;
"|"                TOK_UPD_COL;  return TOK_VERTBAR;
":="               TOK_UPD_COL;  return TOK_COLONEQUALS;
"("                TOK_UPD_COL;  return TOK_LPAREN;
")"                TOK_UPD_COL;  return TOK_RPAREN;
"."                TOK_UPD_COL;  return TOK_DOT;
","                TOK_UPD_COL;  return TOK_COMMA;

"||"               TOK_UPD_COL;  return TOK_OROR;
"&&"               TOK_UPD_COL;  return TOK_ANDAND;
"!="               TOK_UPD_COL;  return TOK_NOTEQUAL;
"=="               TOK_UPD_COL;  return TOK_EQUALEQUAL;
">="               TOK_UPD_COL;  return TOK_GREATEREQ;
"<="               TOK_UPD_COL;  return TOK_LESSEQ;
">"                TOK_UPD_COL;  return TOK_GREATER;
"<"                TOK_UPD_COL;  return TOK_LESS;
"-"                TOK_UPD_COL;  return TOK_MINUS;
"+"                TOK_UPD_COL;  return TOK_PLUS;
"%"                TOK_UPD_COL;  return TOK_PERCENT;
"/"                TOK_UPD_COL;  return TOK_SLASH;
"*"                TOK_UPD_COL;  return TOK_ASTERISK;
"?"                TOK_UPD_COL;  return TOK_QUESTION;

"terminals"        TOK_UPD_COL;  return TOK_TERMINALS;
"nonterm"          TOK_UPD_COL;  return TOK_NONTERM;
"formGroup"        TOK_UPD_COL;  return TOK_FORMGROUP;
"form"             TOK_UPD_COL;  return TOK_FORM;
"attr"             TOK_UPD_COL;  return TOK_ATTR;
"action"           TOK_UPD_COL;  return TOK_ACTION;
"condition"        TOK_UPD_COL;  return TOK_CONDITION;
"treeNodeBase"     TOK_UPD_COL;  return TOK_TREENODEBASE;

  /* --------- embedded semantic functions --------- */
"fundecl" {
  TOK_UPD_COL;
  BEGIN(FUNDECL);
  embedded->reset();
  embedFinish = ';';
  embedMode = TOK_FUNDECL_BODY;
  return TOK_FUNDECL;
}

("fun"|"prologue"|"epilogue") {
  TOK_UPD_COL;

  // one or two tokens must be processed before we start the embedded
  // stuff; the parser will ensure they are there, and then this flag
  // will get us into embedded processing; rules for "{" and "=" deal
  // with 'expectedEmbedded' and transitioning into FUN state
  expectingEmbedded = true;

  embedded->reset();
  embedMode = TOK_FUN_BODY;
  return yytext[0]=='f'? TOK_FUN :
         yytext[0]=='p'? TOK_PROLOGUE :
                         TOK_EPILOGUE;
}

  /* punctuation that can start embedded code */
("{"|"=") {
  TOK_UPD_COL;
  if (expectingEmbedded) {
    expectingEmbedded = false;
    embedFinish = (yytext[0] == '{' ? '}' : ';');
    BEGIN(FUN);
  }
  return yytext[0] == '{' ? TOK_LBRACE : TOK_EQUAL;
}


  /* no TOKEN_START here; we'll use the tokenStartLoc that
   * was computed in the opening punctuation */
<FUNDECL,FUN>{
  [^;}\n]+ {
    UPD_COL;
    embedded->handle(yytext, yyleng);
  }

  "\n" {
    newLine();
    embedded->handle(yytext, yyleng);
  }

  ("}"|";") {
    UPD_COL;
    if (yytext[0] == embedFinish &&
        embedded->zeroNesting()) {
      // done
      BEGIN(INITIAL);

      // adjust tokenStartLoc
      if (embedMode == TOK_FUN_BODY) {
        // we get into embedded mode from a single-char token ('{' or
        // '='), and tokenStartLoc currently has that token's location;
        // since lexer 1 is supposed to partition the input, I want it
        // right
        tokenStartLoc.col++;
      }
      else {
        // here we get into embedded from 'fundecl'
        tokenStartLoc.col += 7;
      }

      // put back delimeter
      yyless(yyleng-1);
      fileState.col--;

      // tell the 'embedded' object whether the text just
      // added is to be considered an expression or a
      // complete function body
      embedded->exprOnly =
        (embedMode == TOK_FUN_BODY &&
         embedFinish == ';');

      // caller can get text from embedded->text
      return embedMode;
    }
    else {
      // embedded delimeter, mostly ignore it
      embedded->handle(yytext, yyleng);
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
    char const *leftq = strchr(yytext, '"');
    char const *rightq = strchr(leftq+1, '"');
    xassert(leftq && rightq);

    /* extract filename string */
    includeFileName = string(leftq+1, rightq-leftq-1);

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
  stringLiteral = string(yytext+1, yyleng-2);        // strip quotes
  return TOK_STRING;
}

  /* --------- illegal ------------- */
{ANY} {
  TOK_UPD_COL;
  errorIllegalCharacter(yytext[0]);
}


%%
/* -------------------- additional C code -------------------- */
