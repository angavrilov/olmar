/* grammar.y
 * parser for grammar files with new ast format */


/* C declarations */
%{

#include "grampar.h"        // yylex, etc.
//#include "gramast.h"        // ASTNode, etc.
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
%token <funDecl> TOK_FUNDECL_BODY
%token <str> TOK_FUN_BODY
%token <str> TOK_DECL_BODY

/* punctuators */
%token TOK_LBRACE "{"
%token TOK_RBRACE "}"
%token TOK_COLON ":"
%token TOK_SEMICOLON ";"
%token TOK_ARROW "->"
%token TOK_VERTBAR "|"
%token TOK_COLONEQUALS ":="
%token TOK_LPAREN "("
%token TOK_RPAREN ")"
%token TOK_DOT "."
%token TOK_COMMA ","
%token TOK_EQUAL "="

/* keywords */
%token TOK_TERMINALS "terminals"
%token TOK_NONTERM "nonterm"
%token TOK_FORMGROUP "formGroup"
%token TOK_ATTR "attr"
%token TOK_ACTION "action"
%token TOK_CONDITION "condition"
%token TOK_FUNDECL "fundecl"
%token TOK_FUN "fun"
%token TOK_TREENODEBASE "treeNodeBase"
%token TOK_TREECOMPARE "treeCompare"
%token TOK_DECLARE "datadecl"
%token TOK_LITERALCODE "literalCode"

/* operators */
%token TOK_OROR "||"
%token TOK_ANDAND "&&"
%token TOK_NOTEQUAL "!="
%token TOK_EQUALEQUAL "=="
%token TOK_GREATEREQ ">="
%token TOK_LESSEQ "<="
%token TOK_GREATER ">"
%token TOK_LESS "<"
%token TOK_MINUS "-"
%token TOK_PLUS "+"
%token TOK_PERCENT "%"
%token TOK_SLASH "/"
%token TOK_ASTERISK "*"
%token TOK_QUESTION "?"
%token TOK_BANG "!"
%token TOK_IF "if"
  /* this last one is just to define TOK_IF uniquely, because attrexpr.cc uses it */

/* operator precedence; same line means same precedence */
/*   lowest precedence */
%left "||"
%left "&&"
%left "!=" "=="
%left ">=" "<=" ">" "<"
%left "-" "+"
%left "%" "/" "*"
/*   highest precedence */


/* ===================== types ============================ */
/* all pointers are owner pointers */
%union {
  int num;
  LocString *str;
  ASTList<ToplevelForm> *formList;
  TF_terminals *terminals;
  ASTList<TermDecl> *termDecls;
  TermDecl *termDecl;
  TF_treeNodeBase *tnBase;
  TF_nonterminal *nonterm;
  ASTList<LocString> *stringList;
  ASTList<NTBodyElt> *ntBodyElts;
  GroupElement *grpElt;
  NT_attr *attr;
  GE_form *form;
  ASTList<FormBodyElt> *fbElts;
  FormBodyElt *fbElt;
  GE_formGroup *formGrp;
  ASTList<GroupElement> *grpElts;
  ASTList<RHS> *rhsList;
  ASTList<RHSElt> *rhsElts;
  FB_action *action;
  FB_condition *condition;
  FB_treeCompare *treeCompare;
  ExprAST *expr;
  ASTList<ExprAST> *exprList;
  FB_funDecl *funDecl;
  FB_funDefn *funDefn;
  FB_dataDecl *dataDecl;
  LiteralCodeAST *litCode;
  LC_standAlone *simpleLitCode;
}

%type <num> StartSymbol
%type <formList> Input
%type <terminals> Terminals
%type <termDecls> TerminalSeqOpt
%type <termDecl> TerminalDecl
%type <tnBase> TreeNodeBase
%type <nonterm> Nonterminal
%type <stringList> BaseClassListOpt BaseClassList
%type <ntBodyElts> NonterminalBody
%type <grpElt> GroupElement
%type <attr> AttributeDecl
%type <form> Form
%type <fbElts> FormBody
%type <fbElt> FormBodyElement
%type <formGrp> FormGroup
%type <grpElts> FormGroupBody
%type <rhsList> FormRHS
%type <rhsElts> RHSEltSeqOpt
%type <action> Action
%type <condition> Condition
%type <treeCompare> TreeCompare
%type <expr> PrimaryExpr UnaryExpr BinaryExpr CondExpr AttrExpr
%type <exprList> ExprListOpt ExprList
%type <funDecl> FunDecl
%type <funDefn> Function
%type <dataDecl> Declaration
%type <litCode> LiteralCode
%type <simpleLitCode> SimpleLiteralCode



/* ===================== productions ======================= */
%%

/* The actions in this file simply build an Abstract Syntax Tree (AST)
 * for later processing.  This keeps the grammar uncluttered, and is
 * an experiment -- my parser will do this automatically.  */


/* start symbol */
/* yields: int (dummy value) */
StartSymbol: Input
               {
                 // return the AST tree top to the caller
                 ((ParseParams*)parseParam)->treeTop = new GrammarAST($1);
                 $$ = 0;
               }
           ;

/* sequence of toplevel forms */
/* yields: ASTList<ToplevelForm> */
Input: /* empty */              { $$ = new ASTList<ToplevelForm>; }
     | Input Terminals          { ($$=$1)->append($2); }
     | Input TreeNodeBase       { ($$=$1)->append($2); }
     | Input Nonterminal        { ($$=$1)->append($2); }
     | Input SimpleLiteralCode  { ($$=$1)->append(new TF_lit($2)); }
     ;

/* ------ terminals ------ */
/*
 * the terminals are the grammar symbols that appear only on the RHS of
 * forms; they are the output of the lexer; the Terminals list declares
 * all of the terminals that will appear in the rules
 */
/* yields: TF_terminals */
Terminals: "terminals" "{" TerminalSeqOpt "}"      { $$ = new TF_terminals($3); }
         ;

/* yields: ASTList<TermDecl> */
TerminalSeqOpt: /* empty */                        { $$ = new ASTList<TermDecl>; }
              | TerminalSeqOpt TerminalDecl        { ($$=$1)->append($2); }
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


/* ------ tree node base ------ */
/* yields: TF_treeNodeBase */
TreeNodeBase: TOK_TREENODEBASE TOK_STRING ";"
                { $$ = new TF_treeNodeBase($2); }
            ;


/* ------ nonterminals ------ */
/*
 * a nonterminal is a grammar symbol that appears on the LHS of forms;
 * the body of the Nonterminal declaration specifies the the RHS forms,
 * attribute info, etc.
 */
/* yields: TF_nonterminal */
Nonterminal: "nonterm" TOK_NAME BaseClassListOpt "{" NonterminalBody "}"
               { $$ = new TF_nonterminal($2, $3, $5); }
           | "nonterm" TOK_NAME BaseClassListOpt Form
               { $$ = new TF_nonterminal($2, $3,
                        new ASTList<NTBodyElt>(new NT_elt($4))); }
           ;

/* a name, and possibly some base-class nonterminals */
/* yields: ASTList<string> */
BaseClassListOpt: /* empty */                 { $$ = new ASTList<LocString>; }
                | ":" BaseClassList           { $$ = $2; }
                ;

/* yields: ASTList<string> */
BaseClassList: TOK_NAME                       { $$ = new ASTList<LocString>($1); }
             | BaseClassList "," TOK_NAME     { ($$=$1)->append($3); }
             ;

/* yields: ASTList<NTBodyElt> */
NonterminalBody: /* empty */                       { $$ = new ASTList<NTBodyElt>; }
               | NonterminalBody AttributeDecl     { ($$=$1)->append($2); }
               | NonterminalBody GroupElement      { ($$=$1)->append(new NT_elt($2)); }
               | NonterminalBody Declaration       { ($$=$1)->append(new NT_elt(new GE_fbe($2))); }
               | NonterminalBody LiteralCode       { ($$=$1)->append(new NT_lit($2)); }
               ;

/* things that can appear in any grouping construct; specifically,
 * a NonterminalBody or a FormGroupBody */
/* yields: GroupElement */
GroupElement: FormBodyElement   { $$ = new GE_fbe($1); }
            | Form              { $$ = $1; }
            | FormGroup         { $$ = $1; }
            ;

/*
 * this declares a synthesized attribute of a nonterminal; every form
 * must specify a value for every declared attribute; at least for
 * now, all attributes are integer-valued
 */
/* yields: NT_attr */
AttributeDecl: "attr" TOK_NAME ";"                 { $$ = new NT_attr($2); }
             ;

/*
 * a form is a context-free grammar production.  it specifies a
 * "rewrite rule", such that any instance of the nonterminal LHS can
 * be rewritten as the sequence of RHS symbols, in the process of
 * deriving the input sentance from the start symbol
 */
/* yields: GE_form */
Form: "->" FormRHS ";"                             { $$ = new GE_form($2, NULL); }
    | "->" FormRHS "{" FormBody "}"                { $$ = new GE_form($2, $4); }
    ;

/*
 * if the form body is present, it is a list of actions and conditions
 * over the attributes; the conditions must be true for the form to be
 * usable, and if used, the actions are then executed to compute the
 * nonterminal's attribute values
 */
/* yields: ASTList<FormBodyElt> */
FormBody: /* empty */                              { $$ = new ASTList<FormBodyElt>; }
        | FormBody FormBodyElement                 { ($$=$1)->append($2); }
        ;

/* things that can be directly associated with a form */
/* yields: FormBodyElt */
FormBodyElement: Action            { $$ = $1; }
               | Condition         { $$ = $1; }
               | TreeCompare       { $$ = $1; }
               | FunDecl           { $$ = $1; }
               | Function          { $$ = $1; }
               ;

/*
 * forms can be grouped together with actions and conditions that apply
 * to all forms in the group;
 *
 * Forms:  alternative forms in a formgroup are used by the parser as
 * if they were all "flattened" to be toplevel alternatives
 *
 * Conditions:  all conditions in enclosing formGroups must be satisfied,
 * in addition to those specified at the level of the form
 *
 * Actions:
 *   - an action that defines an attribute value at an outer level is
 *     inherited by inner forms, unless overridden by a different
 *     definition for the same attribute
 *   - actions can be special -- certain functions, such as
 *     sequence(), are available which yield a different value for each
 *     form
 */
/* yields: GE_formGroup */
FormGroup: "formGroup" "{" FormGroupBody "}"       { $$ = new GE_formGroup($3); }
         ;

/* observe a FormGroupBody has everything a NonterminalBody has, except
 * for attribute declarations */
/* yields: ASTList<GroupElement> */
FormGroupBody: /* empty */                         { $$ = new ASTList<GroupElement>; }
             | FormGroupBody GroupElement          { ($$=$1)->append($2); }
             ;

/* ------ form right-hand side ------ */
/*
 * for now, the only extension to BNF is several alternatives for the
 * top level of a form; this is semantically equivalent to a formgroup
 * with the alternatives as alternative forms
 */
/* yields: ASTList<RHS> */
FormRHS: RHSEltSeqOpt                              { $$ = new ASTList<RHS>(new RHS($1)); }
       | FormRHS "|" RHSEltSeqOpt                  { ($$=$1)->append(new RHS($3)); }
       ;

/*
 * each element on the RHS of a form can have a tag, which appears before a
 * colon (':') if present; the tag is required if that symbol's attributes
 * are to be referenced anywhere in the actions or conditions for the form
 */
/* yields: ASTList<RHSElt> */
RHSEltSeqOpt: /* empty */                          { $$ = new ASTList<RHSElt>; }

            | RHSEltSeqOpt TOK_NAME                { ($$=$1)->append(new RH_name($2)); }
                /* name (only) */
            | RHSEltSeqOpt TOK_NAME ":" TOK_NAME   { ($$=$1)->append(new RH_taggedName($2, $4)); }
                /* tag : name */
            | RHSEltSeqOpt TOK_STRING              { ($$=$1)->append(new RH_string($2)); }
                /* mnemonic terminal */
            | RHSEltSeqOpt TOK_NAME ":" TOK_STRING { ($$=$1)->append(new RH_taggedString($2, $4)); }
                /* tagged terminal */
            ;


/* ------ actions and conditions ------ */
/*
 * for now, the only action is to define the value of a synthesized
 * attribute
 */
/* yields: FB_action */
Action: "action" TOK_NAME ":=" AttrExpr ";"        { $$ = new FB_action($2, $4); }
      ;

/*
 * a condition is a boolean (0 or 1) valued expression; when it evaluates
 * to 0, the associated form cannot be used as a rewrite rule; when it evals
 * to 1, it can; any other value is an error
 */   
/* yields: FB_condition */
Condition: "condition" AttrExpr ";"                { $$ = new FB_condition($2); }
         ;

/*
 * a tree comparison defines an expression that, when evaluated, will
 * yield a value which says which, if either, of two competing
 * interpretations to keep and which to drop
 */
/* yields: FB_treeCompare */
TreeCompare: "treeCompare" "(" TOK_NAME "," TOK_NAME ")" "=" AttrExpr ";"
             { $$ = new FB_treeCompare($3, $5, $8); }
           ;


/* ------ attribute expressions, basically C expressions ------ */
/* yields: ExprAST */
PrimaryExpr: TOK_NAME "." TOK_NAME           { $$ = new E_attrRef($1, $3); }
               /* tag . attr */
           | TOK_NAME "." TOK_NAME "." TOK_NAME   { $$ = new E_treeAttrRef($1, $3, $5); }
               /* tree . tag . attr */
           | TOK_NAME                        { $$ = new E_attrRef(noloc("this"), $1); }
               /* attr (implicit 'this' tag) */
           | TOK_INTEGER                     { $$ = new E_intLit($1); }
               /* literal */
           | "(" AttrExpr ")"                { $$ = $2; }
               /* grouping */
           | TOK_NAME "(" ExprListOpt ")"    { $$ = new E_funCall($1, $3); }
               /* function call */
           ;

/* yields: ASTList<ExprAST> */
ExprListOpt: /* empty */                     { $$ = new ASTList<ExprAST>; }
           | ExprList                        { $$ = $1; }
           ;

/* yields: ASTList<ExprAST> */
ExprList: AttrExpr                           { $$ = new ASTList<ExprAST>($1); }
        | ExprList "," AttrExpr              { ($$=$1)->append($3); }
        ;


/* yields: ExprAST */
UnaryExpr: PrimaryExpr                       { $$ = $1; }
         | "+" PrimaryExpr                   { $$ = $2; }    /* semantically ignored */
         | "-" PrimaryExpr                   { $$ = new E_unary(TOK_MINUS, $2); }
         | "!" PrimaryExpr                   { $$ = new E_unary(TOK_BANG,  $2); }
         ;

/* this rule relies on Bison precedence and associativity declarations */
/* yields: ExprAST */
BinaryExpr: UnaryExpr                        { $$ = $1; }
          | UnaryExpr "*" UnaryExpr          { $$ = new E_binary(TOK_ASTERISK,   $1, $3); }
          | UnaryExpr "/" UnaryExpr          { $$ = new E_binary(TOK_SLASH,      $1, $3); }
          | UnaryExpr "%" UnaryExpr          { $$ = new E_binary(TOK_PERCENT,    $1, $3); }
          | UnaryExpr "+" UnaryExpr          { $$ = new E_binary(TOK_PLUS,       $1, $3); }
          | UnaryExpr "-" UnaryExpr          { $$ = new E_binary(TOK_MINUS,      $1, $3); }
          | UnaryExpr "<" UnaryExpr          { $$ = new E_binary(TOK_LESS,       $1, $3); }
          | UnaryExpr ">" UnaryExpr          { $$ = new E_binary(TOK_GREATER,    $1, $3); }
          | UnaryExpr "<=" UnaryExpr         { $$ = new E_binary(TOK_LESSEQ,     $1, $3); }
          | UnaryExpr ">=" UnaryExpr         { $$ = new E_binary(TOK_GREATEREQ,  $1, $3); }
          | UnaryExpr "==" UnaryExpr         { $$ = new E_binary(TOK_EQUALEQUAL, $1, $3); }
          | UnaryExpr "!=" UnaryExpr         { $$ = new E_binary(TOK_NOTEQUAL,   $1, $3); }
          | UnaryExpr "&&" UnaryExpr         { $$ = new E_binary(TOK_ANDAND,     $1, $3); }
          | UnaryExpr "||" UnaryExpr         { $$ = new E_binary(TOK_OROR,       $1, $3); }
          ;

/* the expression "a ? b : c" means "if (a) then b, else c" */
/* yields: ExprAST */
CondExpr: BinaryExpr                             { $$ = $1; }
        | BinaryExpr "?" AttrExpr ":" AttrExpr   { $$ = new E_cond($1, $3, $5); }
        ;

/* yields: ExprAST */
AttrExpr: CondExpr                               { $$ = $1; }
        ;


/* ------------- semantic functions -------------- */
/* the principle is: "nonterminal = class".  that is, every node
 * representing a reduction to a nonterminal will implement a
 * set of semantic functions.  these functions can call semantic
 * functions of the reduction children, can have parameters passed
 * in, and return values.
 *
 * this is the only part of the grammar language that is specific
 * to the language that will be used to access and process the
 * parser's results; this is the "substrate language" in my
 * terminology. */

/* fundecls declare a function that the nonterminal class supports;
 * every production must implement the function; when C++ is the
 * substrate language, the TOK_FUNDECL is a function prototype;
 * the substrate interface must be able to pull out the name to
 * match it with the corresponding 'fun' */
/* yields: FB_funDecl */
FunDecl: "fundecl" TOK_FUNDECL_BODY ";"          { $$ = $2; }
       ;     /* note: $2 here is an FB_funDecl node already */
             /* todo: make this true */

/* fun is a semantic function; the TOK_FUNCTION is substrate language
 * code that will be emitted into a file for later compilation;
 * it can refer to production children by their tagged names */
/* yields: FB_funDefn */
Function: "fun" TOK_NAME "{" TOK_FUN_BODY "}"    { $$ = new FB_funDefn($2, $4); }
        | "fun" TOK_NAME "=" TOK_FUN_BODY ";"    { $$ = new FB_funDefn($2, $4); }
        ;

/* declarations; can declare data fields, or functions whose
 * implementation is provided externally */
/* yields: FB_dataDecl */
Declaration: "datadecl" TOK_DECL_BODY ";"        { $$ = new FB_dataDecl($2); }
           ;


/* generic literalCode mechanism so I can add new syntax without
 * modifying this file */
/* yields: LiteralCodeAST */
LiteralCode: SimpleLiteralCode
               { $$ = $1; }
           | "literalCode" TOK_STRING TOK_NAME "{" TOK_FUN_BODY "}"
               { $$ = new LC_modifier($2, $3, $5); }
           ;
      
/* yields: LC_standAlone */
SimpleLiteralCode: "literalCode" TOK_STRING "{" TOK_FUN_BODY "}"
                     { $$ = new LC_standAlone($2, $4); }
                 ;

%%
/* ================== extra C code ============== */
#if 0
LocString unbox(LocString *s)
{
  LocString ret(*s);
  delete s;
  return ret;
}    

LocString *noloc(char const *s)
{
  StringRef ref = ((ParseParams*)parseParam)->lexer.strtable.add(s);
  return new LocString(SourceLocation(NULL), ref);    // unknown location
}
#endif // 0
