/* agrampar.y
 * parser for abstract grammar definitions (.ast files) */


/* C declarations */
%{

//#include "grampar.h"        // yylex, etc.
//#include "gramast.h"        // ASTNode, etc.

#include <stdlib.h>         // malloc, free
#include <iostream.h>       // cout

// enable debugging the parser
#ifndef NDEBUG
  #define YYDEBUG 1
#endif

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

/* keywords */
%token TOK_CLASS "class"
%token TOK_PUBLIC "public"
%token TOK_PRIVATE "private"
%token TOK_PROTECTED "protected"
%token TOK_VERBATIM "verbatim"


/* ======================== types ========================== */
/* all pointers are interpreted as owner pointers */
%union {
  ASTSpecFile *file;
  ASTClass *astClass;
  ASTList<CtorArg> *ctorArgList
  ASTList<UserDecl> *userDeclList
  string *str;
  enum AccessCtl accessCtl;
  TF_verbatim *verbatim;
}

%type <file> StartSymbol Input
%type <astClass> Class ClassMembersOpt
%type <ctorArgList> CtorArgsOpt CtorArgs
%type <userDeclList> CtorMembersOpt
%type <str> Arg ArgWord Embedded
%type <accessCtl> Public
%type <verbatim> Verbatim


/* ===================== productions ======================= */
%%

/* start symbol */
/* yields ASTSpecFile, though really $$ isn't used.. */
StartSymbol: Input
               { $$ = ((ParseParams*)parseParam)->treeTop = $1; }
           ;

/* sequence of toplevel forms */
/* yields ASTSpecFile */
Input: /* empty */           { $$ = new ASTSpecFile; }
     | Input Class           { ($$=$1)->forms.append($2); }
     | Input Verbatim        { ($$=$1)->forms.append($2); }
     ;

/* a class is a nonterminal in the abstract grammar */
/* yields ASTClass */
Class: "class" TOK_NAME "{" ClassMembersOpt "}"
         { ($$=$4)->name = $2; }
     ;

/* yields ASTClass */
ClassMembersOpt
  : /* empty */
      { $$ = new ASTClass; }
  | ClassMembersOpt "->" TOK_NAME "(" CtorArgsOpt ")" ";"
      { ($$=$1)->ctors.append(new ASTCtor(unbox($3), $5, new ASTList<UserDecl>())); }
  | ClassMembersOpt "->" TOK_NAME "(" CtorArgsOpt ")" "{" CtorMembersOpt "}"
      { ($$=$1)->ctors.append(new ASTCtor(unbox($3), $5, $8)); }
  | ClassMembersOpt Public Embedded
      { ($$=$1)->decls.append(new UserDecl($2, unbox($3))); }
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
            { $$ = new ASTList<CtorArg>; $$->append(parseCtorArg($1)); }
        | CtorArgs "," Arg
            { ($$=$1)->append(parseCtorArg($3)); }
        ;

/* yields string */
Arg: ArgWord
       { $$ = $1; }
   | Arg ArgWord
       { $$ = appendStr($1, $2); }   // dealloc's $1, $2
   ;

/* yields string */
ArgWord
  : TOK_NAME     { $$ = $1; }
  | "<"          { $$ = new string("<"); }
  | ">"          { $$ = new string(">"); }
  | "*"          { $$ = new string("*"); }
  | "&"          { $$ = new string("&"); }
  | TOK_CLASS    { $$ = new string("class"); }
  ;

/* yields ASTList<UserDecl> */
CtorMembersOpt
  : /* empty */
      { $$ = new ASTList<UserDecl>; }
  | CtorMembersOpt Public Embedded
      { ($$=$1)->append(new UserDecl($2, unbox($3))); }
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
  ;

/* yields TF_verbatim */
Verbatim: TOK_VERBATIM Embedded
            { $$ = new TF_verbatim(unbox($2)); }
        ;

%%
