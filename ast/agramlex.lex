/* agrammar.lex            see license.txt for copyright and terms of use
 * lexical analyzer for my AST input format
 */

/* ----------------- C definitions -------------------- */
%{

// pull in my declaration of the lexer class -- this defines
// the additional lexer state, some of which is used in the
// action rules below
#include "gramlex.h"

// pull in the bison-generated token codes
#include "agrampar.codes.h"

#include <string.h>         // strchr, strrchr

// for maintaining column count
#define TOKEN_START  tokenStartLoc = fileState.loc /* user ; */
#define UPD_COL      advCol(yyleng) /* user ; */
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
STRCHR    [^\n\\\"]

/* whitespace that doesn't cross line a boundary */
SLWHITE   [ \t]


/* --------------- start conditions ------------------- */
%x C_COMMENT
%x EMBED
%x INITVAL


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
  advCol(yyleng-1);   // don't count the newline
  newLine();          // get it here
}


  /* -------- punctuators, operators, keywords --------- */
"}"                TOK_UPD_COL;  return TOK_RBRACE;
";"                TOK_UPD_COL;  return TOK_SEMICOLON;
"->"               TOK_UPD_COL;  return TOK_ARROW;
"("                TOK_UPD_COL;  return TOK_LPAREN;
","                TOK_UPD_COL;  return TOK_COMMA;

"<"                TOK_UPD_COL;  return TOK_LANGLE;
">"                TOK_UPD_COL;  return TOK_RANGLE;
"*"                TOK_UPD_COL;  return TOK_STAR;
"&"                TOK_UPD_COL;  return TOK_AMPERSAND;
"="                TOK_UPD_COL;  return TOK_EQUALS;

"class"            TOK_UPD_COL;  return TOK_CLASS;
"option"           TOK_UPD_COL;  return TOK_OPTION;
"new"              TOK_UPD_COL;  return TOK_NEW;
"enum"             TOK_UPD_COL;  return TOK_ENUM;

  /* --------- embedded text --------- */
("public"|"protected"|"private"|"ctor"|"dtor"|"pure_virtual")("(")? {
  TOK_UPD_COL;
  
  // is a paren included?
  if (yytext[yyleng-1] == '(') {
    // don't drop into embedded just yet; wait for the ')'
    embedStart = ')';
    yyless(yyleng-1);
    advCol(-1);
  }
  else {
    BEGIN(EMBED);
  }

  embedded->reset();
  embedFinish = ';';
  allowInit = yytext[0]=='p';
  embedMode = TOK_EMBEDDED_CODE;
  return yytext[0]=='c'?   TOK_CTOR :
         yytext[0]=='d'?   TOK_DTOR :
         yytext[2] == 'b'? TOK_PUBLIC :
         yytext[2] == 'o'? TOK_PROTECTED :
         yytext[2] == 'i'? TOK_PRIVATE :
             /*[2] == 'r'*/TOK_PURE_VIRTUAL ;
}

("verbatim"|"impl_verbatim") {
  TOK_UPD_COL;

  // need to see one more token before we begin embedded processing
  embedStart = '{';
  embedFinish = '}';
  allowInit = false;

  embedded->reset();
  embedMode = TOK_EMBEDDED_CODE;
  return yytext[0]=='v'? TOK_VERBATIM :
                         TOK_IMPL_VERBATIM ;
}

"custom" {
  TOK_UPD_COL;

  embedStart = '{';
  embedFinish = '}';
  allowInit = false;
  embedded->reset();
  embedMode = TOK_EMBEDDED_CODE;

  return TOK_CUSTOM;
}

  /* punctuation that can start embedded code */
("{"|")") {
  TOK_UPD_COL;
  if (yytext[0] == embedStart) {
    BEGIN(EMBED);
  }
  return yytext[0]=='{'? TOK_LBRACE : TOK_RPAREN;
}


  /* no TOKEN_START here; we'll use the tokenStartLoc that
   * was computed in the opening punctuation */
<EMBED>{
  /* no special significance to lexer */
  [^;}=\n]+ {
    UPD_COL;
    embedded->handle(yytext, yyleng, embedFinish);
  }

  "\n" {
    newLine();
    embedded->handle(yytext, yyleng, embedFinish);
  }

  /* possibly closing delimiter */
  ("}"|";"|"=") {
    UPD_COL;

    // we're done if we're at a zero nesting level and the
    // delimiter matches ...
    if (embedded->zeroNesting() && embedFinishMatches(yytext[0])) {
      // done
      BEGIN(INITIAL);

      if (yytext[0] == '=') {
        // switch to a special mode that will handle the '=' and
        // jump right back into embedded mode
        BEGIN(INITVAL);
      }
      else {
        // turn off embedded detection
        embedStart = 0;
      }

      // put back delimeter so parser will see it
      yyless(yyleng-1);
      advCol(-1);

      // in the abstract grammar we don't have embedded expressions
      embedded->exprOnly = false;

      // and similarly for the other flag
      embedded->isDeclaration = (embedFinish == ';');

      // caller can get text from embedded->text
      return embedMode;
    }
    else {
      // embedded delimeter, mostly ignore it
      embedded->handle(yytext, yyleng, embedFinish);
    }
  }
}


<INITVAL>{
  "=" {
    // yield the '=', switch back into embedded
    BEGIN(EMBED);
    embedded->reset();
    allowInit = false;
    return TOK_EQUALS;
  }

  {ANY} {
    xfailure("somehow got a char other than '=' in INITVAL state");
  }
}


  /* -------- name literal --------- */
{LETTER}({LETTER}|{DIGIT})* {
  // get text from yytext and yyleng
  TOK_UPD_COL;
  return TOK_NAME;
}

  /* --------- integer literal --------- */
{DIGIT}+ {
  TOK_UPD_COL;
  return TOK_INTLIT;
}

  /* --------- illegal ------------- */
{ANY} {
  TOK_UPD_COL;
  errorIllegalCharacter(yytext[0]);
}


%%
/* -------------------- additional C code -------------------- */

bool isAGramlexEmbed(int code)
{
  return code == TOK_EMBEDDED_CODE;
}
