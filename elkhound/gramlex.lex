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
#define UPD_COL   fileState.col += yyleng;

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
  UPD_COL
}

  /* -------- comments -------- */
"/*" {
  /* C-style comments */
  UPD_COL
  commentStartLine = fileState.line;
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
    UPD_COL      // <<EOF>> yyleng is 1!
    errorUnterminatedComment();
    return TOK_EOF;
  }
}


"//".*"\n" {
  /* C++-style comment -- eat it */
  newLine();
}


  /* -------- punctuators, operators, keywords --------- */
  /* first, punctuation that can start embedded code */
("{"|"=") {
  UPD_COL
  if (expectingEmbedded) {
    expectingEmbedded = false;
    embedFinish = (yytext[0] == '{' ? '}' : ';');
    BEGIN(FUN);
  }
  return yytext[0] == '{' ? TOK_LBRACE : TOK_EQUAL;
}


  /* now just normal stuff */
"}"                UPD_COL  return TOK_RBRACE;
":"                UPD_COL  return TOK_COLON;
";"                UPD_COL  return TOK_SEMICOLON;
"->"               UPD_COL  return TOK_ARROW;
"|"                UPD_COL  return TOK_VERTBAR;
":="               UPD_COL  return TOK_COLONEQUALS;
"("                UPD_COL  return TOK_LPAREN;
")"                UPD_COL  return TOK_RPAREN;
"."                UPD_COL  return TOK_DOT;
","                UPD_COL  return TOK_COMMA;

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
"?"                UPD_COL  return TOK_QUESTION;

"terminals"        UPD_COL  return TOK_TERMINALS;
"nonterm"          UPD_COL  return TOK_NONTERM;
"formGroup"        UPD_COL  return TOK_FORMGROUP;
"form"             UPD_COL  return TOK_FORM;
"attr"             UPD_COL  return TOK_ATTR;
"action"           UPD_COL  return TOK_ACTION;
"condition"        UPD_COL  return TOK_CONDITION;

  /* --------- embedded semantic functions --------- */
"fundecl" {
  UPD_COL
  BEGIN(FUNDECL);
  embedded->reset();
  embedFinish = ';';
  embedMode = TOK_FUNDECL_BODY;
  return TOK_FUNDECL;
}

("fun"|"prologue"|"epilogue") {
  UPD_COL

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

<FUNDECL,FUN>{
  [^;}\n]+ {
    UPD_COL
    embedded->handle(yytext, yyleng);
  }     
  
  "\n" {
    newLine();
    embedded->handle(yytext, yyleng);
  }

  ("}"|";") {
    UPD_COL
    if (yytext[0] == embedFinish &&
        embedded->zeroNesting()) {
      // done
      BEGIN(INITIAL);

      // put back delimeter
      yyless(yyleng-1);
      
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
  UPD_COL
  BEGIN(INCLUDE);
}

<INCLUDE>{
  {SLWHITE}*"("{SLWHITE}*{DQUOTE}{STRCHR}+{DQUOTE}{SLWHITE}*")" {
    /* e.g.: ("filename") */
    /* file name to include */
    UPD_COL

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
    UPD_COL
    errorMalformedInclude();

    /* rudimentary error recovery.. */
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

  /* ----------- string literal ----- */
{DQUOTE}{STRCHR}*{DQUOTE} {
  UPD_COL
  stringLiteral = string(yytext+1, yyleng-2);        // strip quotes
  return TOK_STRING;
}

  /* --------- illegal ------------- */
{ANY} {
  UPD_COL
  errorIllegalCharacter(yytext[0]);
}


%%
/* -------------------- additional C code -------------------- */
