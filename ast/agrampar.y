/* agrampar.y            see license.txt for copyright and terms of use
 * parser for abstract grammar definitions (.ast files) */


/* C declarations */
%{

#include "agrampar.h"       // agrampar_yylex, etc.

#include <stdlib.h>         // malloc, free
#include <iostream.h>       // cout

// enable debugging the parser
#ifndef NDEBUG
  #define YYDEBUG 1
#endif

// permit having other parser's codes in the same program
#define yyparse agrampar_yyparse

%}


/* ================== bison declarations =================== */
// don't use globals
%pure_parser


/* ===================== tokens ============================ */
/* tokens that have many lexical spellings */
%token <str> TOK_NAME
%token <str> TOK_EMBEDDED_CODE

/* punctuators */
%token TOK_LBRACE "{"
%token TOK_RBRACE "}"
%token TOK_SEMICOLON ";"
%token TOK_ARROW "->"
%token TOK_LPAREN "("
%token TOK_RPAREN ")"
%token TOK_LANGLE "<"
%token TOK_RANGLE ">"
%token TOK_STAR "*"
%token TOK_AMPERSAND "&"
%token TOK_COMMA ","

/* keywords */
%token TOK_CLASS "class"
%token TOK_PUBLIC "public"
%token TOK_PRIVATE "private"
%token TOK_PROTECTED "protected"
%token TOK_VERBATIM "verbatim"
%token TOK_IMPL_VERBATIM "impl_verbatim"
%token TOK_CTOR "ctor"
%token TOK_DTOR "dtor"
%token TOK_PURE_VIRTUAL "pure_virtual"
%token TOK_CUSTOM "custom"


/* ======================== types ========================== */
/* all pointers are interpreted as owner pointers */
%union {
  ASTSpecFile *file;
  ASTList<ToplevelForm> *formList;
  TF_class *tfClass;
  ASTList<CtorArg> *ctorArgList;
  ASTList<Annotation> *userDeclList;
  string *str;
  enum AccessCtl accessCtl;
  ToplevelForm *verbatim;
  Annotation *annotation;
}

%type <file> StartSymbol
%type <formList> Input
%type <tfClass> Class ClassBody ClassMembersOpt
%type <ctorArgList> CtorArgsOpt CtorArgs
%type <userDeclList> CtorMembersOpt
%type <str> Arg ArgWord Embedded ArgList
%type <accessCtl> Public
%type <verbatim> Verbatim
%type <annotation> Annotation


/* ===================== productions ======================= */
%%

/* start symbol */
/* yields ASTSpecFile, though really $$ isn't used.. */
StartSymbol: Input
               { $$ = *((ASTSpecFile**)parseParam) = new ASTSpecFile($1); }
           ;

/* sequence of toplevel forms */
/* yields ASTList<ToplevelForm> */
Input: /* empty */           { $$ = new ASTList<ToplevelForm>; }
     | Input Class           { ($$=$1)->append($2); }
     | Input Verbatim        { ($$=$1)->append($2); }
     ;

/* a class is a nonterminal in the abstract grammar */
/* yields TF_class */
Class: "class" TOK_NAME ClassBody
         { ($$=$3)->super->name = unbox($2); }
     | "class" TOK_NAME "(" CtorArgsOpt ")" ClassBody
         { ($$=$6)->super->name = unbox($2); $$->super->args.steal($4); }
     ;

/* yields TF_class */
ClassBody: "{" ClassMembersOpt "}"
             { $$=$2; }
         | ";"
             { $$ = new TF_class(new ASTClass("(placeholder)", NULL, NULL), NULL); }
         ;

/* yields TF_class */
/* does this by making an empty one initially, and then adding to it */
ClassMembersOpt
  : /* empty */
      { $$ = new TF_class(new ASTClass("(placeholder)", NULL, NULL), NULL); }
  | ClassMembersOpt "->" TOK_NAME "(" CtorArgsOpt ")" ";"
      { ($$=$1)->ctors.append(new ASTClass(unbox($3), $5, NULL)); }
  | ClassMembersOpt "->" TOK_NAME "(" CtorArgsOpt ")" "{" CtorMembersOpt "}"
      { ($$=$1)->ctors.append(new ASTClass(unbox($3), $5, $8)); }
  | ClassMembersOpt Annotation
      { ($$=$1)->super->decls.append($2); }
  ;
  
/* yields ASTList<CtorArg> */
CtorArgsOpt
  : /* empty */
      { $$ = new ASTList<CtorArg>; }
  | CtorArgs
      { $$ = $1; }
  ;

/* yields ASTList<CtorArg> */
CtorArgs: Arg
            { $$ = new ASTList<CtorArg>;
              {
                string tmp = unbox($1);
                $$->append(parseCtorArg(tmp));
              }
            }
        | CtorArgs "," Arg
            { ($$=$1)->append(parseCtorArg(unbox($3))); }
        ;

/* yields string */
Arg: ArgWord
       { $$ = $1; }
   | Arg ArgWord
       { $$ = appendStr($1, $2); }   // dealloc's $1, $2
   ;

/* yields string */
ArgWord
  : TOK_NAME         { $$ = appendStr($1, box(" ")); }
  | "<" ArgList ">"  { $$ = appendStr(box("<"), appendStr($2, box(">"))); }
  | "*"              { $$ = box("*"); }
  | "&"              { $$ = box("&"); }
  | TOK_CLASS        { $$ = box("class "); }    /* special b/c is ast spec keyword */
  ;

/* yields string, and may have commas inside */
ArgList: Arg
           { $$ = $1 }
       | Arg "," ArgList
           { $$ = appendStr($1, appendStr(box(","), $3)); }
       ;

/* yields ASTList<Annotation> */
CtorMembersOpt
  : /* empty */
      { $$ = new ASTList<Annotation>; }
  | CtorMembersOpt Annotation
      { ($$=$1)->append($2); }
  ;

/* yields Annotation */
Annotation
  : Public Embedded
      { $$ = new UserDecl($1, unbox($2)); }
  | "custom" TOK_NAME Embedded
      { $$ = new CustomCode(unbox($2), unbox($3)); }
  ;

/* yields string */
Embedded
  : TOK_EMBEDDED_CODE ";"
      { $$ = $1; }
  | "{" TOK_EMBEDDED_CODE "}"
      { $$ = $2; }
  ;

/* yields AccessCtl */
Public
  : "public"        { $$ = AC_PUBLIC; }
  | "private"       { $$ = AC_PRIVATE; }
  | "protected"     { $$ = AC_PROTECTED; }
  | "ctor"          { $$ = AC_CTOR; }
  | "dtor"          { $$ = AC_DTOR; }
  | "pure_virtual"  { $$ = AC_PUREVIRT; }
  ;

/* yields TF_verbatim */
Verbatim: "verbatim" Embedded
            { $$ = new TF_verbatim(unbox($2)); }
        | "impl_verbatim" Embedded
            { $$ = new TF_impl_verbatim(unbox($2)); }
        ;

%%

/* ----------------- extra C code ------------------- */

