/* Remove __attribute__(()) expressions from a C or C++ program
 * -*-c++-*-
 * Based on topformflat
 *
 * Copyright (C) 2002 Daniel S. Wilkerson <dsw@cs.berkeley.edu>
 * Copyright (C) 2002 Scott McPeak <smcpeak@cs.berkeley.edu>
 * */

%{
//  #include <stdlib.h>     // atoi
#include <assert.h>

  // number of nested attribute levels
  int attr_nesting;
  
  // emit yytext as-is unless attr_nesting > 0;
  void emit();

//    // check that we are in this lexer state ("start condition" in flex lingo)
//    void assert_state(int state);

  void push(int state);
  void pop();

  // debugging diagnostic, emitted when enabled
  void diag(char const *str);

%}

/* don't try to call yywrap() */
%option noyywrap
/* dsw: don't define yyunput() */
%option nounput
/* dsw: we want a stack */
%option stack
/* count the lines for me */
%option yylineno 

/* start condition for strings */
%x STRING
%x CHARLIT
%x C_COMMENT
%x CURLY
%x PAREN
%x ATTRIBUTE
%x ATTROPEN1
%x ATTROPEN2
%x ATTRCLOSE1

SPACE [ \t\r\n\f\v]

%%

  /* double-quoted string */
<STRING>{
  "\\"(.|\n)  { emit(); }                        /* escaped character */
  "\""        { emit(); diag("</STR>"); pop(); } /* close quote */
  (.|\n)      { emit(); }                        /* ordinary char */
}

  /* single-quoted character literal */
<CHARLIT>{
  "\\"(.|\n)  { emit(); }                         /* escaped character */
  "\'"        { emit(); diag("</CHAR>"); pop(); } /* close tick */
  (.|\n)      { emit(); }                         /* ordinary char */
}

<CURLY>"}" {pop(); emit();}
<PAREN>")" {pop(); emit();}

<INITIAL,PAREN>"}" { fprintf(stderr, "unexpected '%s' at line %d\n", yytext, yylineno); exit(1); }
<INITIAL,CURLY>")" { fprintf(stderr, "unexpected '%s' at line %d\n", yytext, yylineno); exit(1); }

  /* attributes */
<ATTRIBUTE>"("  {BEGIN(ATTROPEN1); emit();}
<ATTROPEN1>"("  {BEGIN(ATTROPEN2); emit();}
<ATTROPEN2>")"  {BEGIN ATTRCLOSE1; emit();}
<ATTRCLOSE1>")" {pop(); emit(); --attr_nesting; diag("</ATTR>"); }

<INITIAL,CURLY,PAREN,ATTRIBUTE,ATTROPEN1,ATTROPEN2,ATTRCLOSE1>{
  {SPACE}* {emit();}                       /* whitespace */
  "//".*"\n" { emit(); }                   /* C++ comment */
  "/*" {emit(); push(C_COMMENT);} /* C comment */
}

<C_COMMENT>{
  [^*]*    {emit();}        /* eat anything not a star */
  "*"+[^/] {emit();}        /* eat stars followed by a non-slash */
  "*"+"/"  {pop(); emit();} /* exit on stars followed by a slash */
}

  /* normal mode */
<INITIAL,CURLY,PAREN,ATTROPEN2>{
  /* start double-quoted string */
  "\"" {diag("<STR>"); emit(); push(STRING);}
  /* start single-quoted character literal */
  "\'" {diag("<CHAR>"); emit(); push(CHARLIT);}

  "{" {push(CURLY); emit();}
  "(" {push(PAREN); emit();}
  "__attribute__" {diag("<ATTR>"); push(ATTRIBUTE); ++attr_nesting; emit();}

  /* anything else */
  [^})] { emit(); }
}

<*>(.|\n) { fprintf(stderr, "unexpected character at line %d\n", yylineno); exit(1); }

%%

void emit()
{
  assert(attr_nesting >= 0);
  if (attr_nesting > 0) {
    for(int i=0; i<yyleng; ++i) {
      int c=yytext[i];
      switch(c) {               // preserve interesting whitespace
      case '\t': case '\r': case '\n': case '\f': case '\v': break;
      default: c = ' '; break;  // everything else goes to spaces
      }
      printf("%c", c);
    }
  } else {
    printf("%.*s", yyleng, yytext);
  }
}

//  void assert_state(int state) {
//  //    fprintf(stderr, "assert_state, state:%d, line:%d", state, yylineno);
//    if (state != YY_START) {
//      fprintf(stderr, "Too many closing paired delimiters (curly or paren) at line %d.\n",
//              yylineno);
//      exit(1);
//    }
//  }

void push(int state) {
//    printf("<STATE:%d>", state);
  yy_push_state(state);
}

void pop() {
//    printf("</STATE:%d>", YY_START);
  yy_pop_state();
}

void diag(char const *str)
{
  //printf("%s", str);
}

char *version = "2003.3.14";
int main(int argc, char *argv[])
{
  if (isatty(0) || argc!=1) {
    fprintf(stderr, "deattribute version %s\n", version);
    fprintf(stderr, "Removes __attribute__(()) expressions from C and C++.\n");
    fprintf(stderr, "usage: %s <input.c >output.c\n", argv[0]);
    return 1;
  }

  yyin = stdin;
  attr_nesting = 0;
  yylex();
//    fprintf(stderr, "**** end\n");
  if (INITIAL != YY_START) {
    fprintf(stderr, "Too few closing paired delimiters (curly or paren) in file.\n");
    exit(1);
  }
  assert(attr_nesting >= 0);
  if (attr_nesting > 0) {
    // not sure this can happen without being caught above
    fprintf(stderr, "End of file withing attribute.\n");
    exit(1);
  }

  if (0) yy_top_state();        // useless; make gcc warning go away
  return 0;
}
