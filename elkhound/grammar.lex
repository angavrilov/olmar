/* grammar.lex
 * lexical analyzer for my grammar input format
 */

/* ----------------- C definitions -------------------- */
%{

// pull in the bison-generated token codes */
#include "grammar.tab.h"

// my lexer state
int commentStartLine;        // for reporting unterminated C comments
int integerLiteral;          // to store number literal value
int lineNumber = 1;          // for reporting errors

// lexer state automatically supplied by flex
//   yytext      - contents of matched string, for names
//   yyleng      - # of characters in yytext

%}


/* -------------------- flex options ------------------ */
/* no wrapping is needed; setting this means we don't have to link with libfl.a */
%option noyywrap


/* ------------------- definitions -------------------- */
/* any character, including newline */
ANY       (.|"\n")

/* starting character in a name */
LETTER    [a-zA-Z_"]

/* starting character in a numeric literal */
DIGIT     [0-9]


/* --------------- start conditions ------------------- */
%x C_COMMENT


/* ---------------------- rules ----------------------- */
%%

  /* -------- whitespace ------ */
"\n" {
  lineNumber++;
}

[ \t\f\v]+ {
  /* no-op */
}

  /* -------- comments -------- */
"/*" {
  /* C-style comments */
  commentStartLine = lineNumber;
  BEGIN(C_COMMENT);
}

<C_COMMENT>{
  "*/" {
    /* end of comment */
    BEGIN(INITIAL);
  }

  {ANY} {
    /* anything but slash-star -- eat it */
  }

  <<EOF>> {
    fprintf(stderr, "unterminated comment, beginning on line %d\n", commentStartLine);
    return 0;     // eof
  }
}


"//".*"\n" {
  /* C++-style comment -- eat it */
}


  /* -------- punctuators, operators, keywords --------- */
"{"                return TOK_LBRACE;
"}"                return TOK_RBRACE;
":"                return TOK_COLON;
";"                return TOK_SEMICOLON;
"->"               return TOK_ARROW;
"|"                return TOK_VERTBAR;
":="               return TOK_COLONEQUALS;
"("                return TOK_LPAREN;
")"                return TOK_RPAREN;

"||"               return TOK_OROR;
"&&"               return TOK_ANDAND;
"!="               return TOK_NOTEQUAL;
"=="               return TOK_EQUALEQUAL;
">="               return TOK_GREATEREQ;
"<="               return TOK_LESSEQ;
">"                return TOK_GREATER;
"<"                return TOK_LESS;
"-"                return TOK_MINUS;
"+"                return TOK_PLUS;
"%"                return TOK_PERCENT;
"/"                return TOK_SLASH;
"*"                return TOK_ASTERISK;

"nonterm"          return TOK_NONTERM;
"formGroup"        return TOK_FORMGROUP;
"form"             return TOK_FORM;
"action"           return TOK_ACTION;
"condition"        return TOK_CONDITION;


  /* -------- name literal -------- */
{LETTER}({LETTER}|{DIGIT})* {
  /* get text from yytext and yyleng */
  return TOK_NAME;
}

  /* -------- numeric literal ----- */
{DIGIT}+ {
  integerLiteral = strtoul(yytext, NULL, 10 /*radix*/);
  return TOK_INTEGER;
}

. {
  fprintf(stderr, "illegal character: `%c', line %d\n",
                  yytext[0], lineNumber);
}



%%
/* -------------------- additional C code -------------------- */

#ifdef TEST_GRAMMAR_LEX

int main()
{
  while (1) {
    int code = yylex();
    if (code == 0) {  // eof
      break;
    }
    
    printf("token: code=%d, text: %.*s\n", code, yyleng, yytext);
  }

  return 0;
}



#endif // TEST_GRAMMAR_LEX
