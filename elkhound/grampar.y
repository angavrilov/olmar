/* grammar.y
 * parser for grammar files with new ast format */


/* C declarations */
%{

#include "grampar.h"        // yylex, etc.
#include "gramast.gen.h"    // grammar syntax AST definition
#include "gramlex.h"        // GrammarLexer

#include <stdlib.h>         // malloc, free
#include <iostream.h>       // cout

// enable debugging the parser
#ifndef NDEBUG
  #define YYDEBUG 1
#endif

// name of extra parameter to yylex
#define YYLEX_PARAM parseParam

// make it call my yylex
#define yylex(lv, param) grampar_yylex(lv, param)

// Bison calls yyerror(msg) on error; we need the extra
// parameter too, so the macro shoehorns it in there
#define yyerror(msg) grampar_yyerror(msg, YYPARSE_PARAM)

// rename the externally-visible parsing routine to make it
// specific to this instance, so multiple bison-generated
// parsers can coexist
#define yyparse grampar_yyparse


// return contents of 's', which is then deallocated
//LocString unbox(LocString *s);

// return a locstring for 's' with no location information
#define noloc(str)                                                    \
  new LocString(SourceLocation(NULL),      /* unknown location */     \
                ((ParseParams*)parseParam)->lexer.strtable.add(str))

//LocString *noloc(char const *s);

%}


/* ================== bison declarations =================== */
// don't use globals
%pure_parser


/* ===================== tokens ============================ */
/* tokens that have many lexical spellings */
%token <num> TOK_INTEGER
%token <str> TOK_NAME
%token <str> TOK_STRING
%token <str> TOK_LIT_CODE

/* punctuators */
%token TOK_LBRACE "{"
%token TOK_RBRACE "}"
%token TOK_COLON ":"
%token TOK_SEMICOLON ";"
%token TOK_ARROW "->"
%token TOK_LPAREN "("
%token TOK_RPAREN ")"

/* keywords */
%token TOK_TERMINALS "terminals"
%token TOK_TOKEN "token"
%token TOK_NONTERM "nonterm"
%token TOK_VERBATIM "verbatim"


/* ===================== types ============================ */
/* all pointers are owner pointers */
%union {
  int num;
  LocString *str;              

  Terminals *terminals;
  ASTList<TermDecl> *termDecls;
  TermDecl *termDecl;
  ASTList<TermType> *termTypes;
  
  ASTList<NontermDecl> *nonterms;
  NontermDecl *nonterm;
  ASTList<ProdDecl> *prodDecls;
  ProdDecl *prodDecl;
  ASTList<RHSElt> *rhsList;
  RHSElt *rhsElt;
}

%type <num> StartSymbol
%type <str> Type Verbatim

%type <terminals> Terminals
%type <termDecls> TermDecls
%type <termDecl> TerminalDecl
%type <termTypes> TermTypes

%type <nonterms> Nonterminals
%type <nonterm> Nonterminal
%type <prodDecls> Productions
%type <prodDecl> Production
%type <rhsList> RHS
%type <rhsElt> RHSElt


/* ===================== productions ======================= */
%%

/* The actions in this file simply build an Abstract Syntax Tree (AST)
 * for later processing.  This keeps the grammar uncluttered, and is
 * an experiment -- my parser will do this automatically.  */


/* start symbol */
/* yields: int (dummy value) */
StartSymbol: Verbatim Terminals Nonterminals
               {
                 // return the AST tree top to the caller
                 ((ParseParams*)parseParam)->treeTop = new GrammarAST($1, $2, $3);
                 $$ = 0;
               }
           ;

/* yields: LocString */
Verbatim: /* empty */                    { $$ = noloc(""); }
        | "verbatim" TOK_LIT_CODE        { $$ = $2; }
        ;

/* ------ terminals ------ */
/*
 * the terminals are the grammar symbols that appear only on the RHS of
 * forms; they are the output of the lexer; the Terminals list declares
 * all of the terminals that will appear in the rules
 */
/* yields: Terminals */
Terminals: "terminals" "{" TermDecls TermTypes "}" { $$ = new Terminals($3, $4); }
         ;

/* yields: ASTList<TermDecl> */
TermDecls: /* empty */                             { $$ = new ASTList<TermDecl>; }
         | TermDecls TerminalDecl                  { ($$=$1)->append($2); }
         ;

/* each terminal has an integer code which is the integer value the
 * lexer uses to represent that terminal.  it is followed by a
 * canonical name, and an optional alias; the name/alias appears in
 * the forms, rather than the integer code itself */
/* yields: TermDecl */
TerminalDecl: TOK_INTEGER ":" TOK_NAME ";"
                { $$ = new TermDecl($1, $3, noloc("")); }
            | TOK_INTEGER ":" TOK_NAME TOK_STRING ";"
                { $$ = new TermDecl($1, $3, $4); }
            ;

/* yields: LocString */
Type: TOK_LIT_CODE                                 { $$ = $1; }
    ;

/* yields: ASTList<TermType> */
TermTypes: /* empty */
             { $$ = new ASTList<TermType>; }
         | TermTypes "token" Type TOK_NAME ";"
             { ($$=$1)->append(new TermType($4, $3)); }
         ;


/* ------ nonterminals ------ */
/*
 * a nonterminal is a grammar symbol that appears on the LHS of forms;
 * the body of the Nonterminal declaration specifies the the RHS forms,
 * attribute info, etc.
 */
/* yields: ASTList<NontermDecl> */
Nonterminals: /* empty */                  { $$ = new ASTList<NontermDecl>; }
            | Nonterminals Nonterminal     { ($$=$1)->append($2); }
            ;

/* yields: NontermDecl */
Nonterminal: "nonterm" Type TOK_NAME Production
               { $$ = new NontermDecl($3, $2, new ASTList<ProdDecl>($4)); }
           | "nonterm" Type TOK_NAME "{" Productions "}"
               { $$ = new NontermDecl($3, $2, $5); }
           ;

/* yields: ASTList<ProdDecl> */
Productions: /* empty */                   { $$ = new ASTList<ProdDecl>; }
           | Productions Production        { ($$=$1)->append($2); }
           ;

/* yields: ProdDecl */
Production: "->" RHS TOK_LIT_CODE          { $$ = new ProdDecl($2, $3); }
          ;

/* yields: ASTList<RHSElt> */
RHS: /* empty */                           { $$ = new ASTList<RHSElt>; }
   | RHS RHSElt                            { ($$=$1)->append($2); }
   ;

/*
 * each element on the RHS of a form can have a tag, which appears before a
 * colon (':') if present; the tag is required if that symbol's attributes
 * are to be referenced anywhere in the actions or conditions for the form
 */
/* yields: RHSElt */
RHSElt: TOK_NAME                { $$ = new RH_name($1); }
          /* name (only) */
      | TOK_NAME ":" TOK_NAME   { $$ = new RH_taggedName($1, $3); }
          /* tag : name */
      | TOK_STRING              { $$ = new RH_string($1); }
          /* mnemonic terminal */
      | TOK_NAME ":" TOK_STRING { $$ = new RH_taggedString($1, $3); }
          /* tagged terminal */
      ;

%%
