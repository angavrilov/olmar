/* grammar.y
 * parser for grammar files */


/* C declarations */
%{

#include "grampar.h"        // yylex, etc.
#include "gramast.h"        // ASTNode, etc.

#include <stdlib.h>         // malloc, free
#include <iostream.h>       // cout

%}


/* ===================== tokens ============================ */
/* tokens that have many lexical spellings */
%token TOK_INTEGER
%token TOK_NAME
%token TOK_STRING

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

/* keywords */
%token TOK_TERMINALS "terminals"
%token TOK_NONTERM "nonterm"
%token TOK_FORMGROUP "formGroup"
%token TOK_FORM "form"
%token TOK_ATTR "attr"
%token TOK_ACTION "action"
%token TOK_CONDITION "condition"

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

/* operator precedence; same line means same precedence */
/*   lowest precedence */
%left "||"
%left "&&"
%left "!=" "=="
%left ">=" "<=" ">" "<"
%left "-" "+"
%left "%" "/" "*"
/*   highest precedence */


/* ===================== productions ======================= */
%%

/* The actions in this file simply build an Abstract Syntax Tree (AST)
 * for later processing.  This keeps the grammar uncluttered, and is
 * an experiment -- my parser will do this automatically.  */


/* start symbol */
StartSymbol: Input
               {                         
                 cout << "AST:\n";
                 $1->debugPrint(cout, 2);
                 delete $1;
               }
           ;

/* sequence of toplevel forms */
Input: /* empty */           { $$ = AST0(AST_TOPLEVEL); }
     | Input Terminals       { $$ = iappend($1, $2); }
     | Input Nonterminal     { $$ = iappend($1, $2); }
     ;

/* ------ terminals ------ */
/*
 * the terminals are the grammar symbols that appear only on the RHS of
 * forms; they are the output of the lexer; the Terminals list declares
 * all of the terminals that will appear in the rules
 */
Terminals: "terminals" "{" TerminalSeqOpt "}"      { $$ = $3; }
         ;

TerminalSeqOpt: /* empty */                        { $$ = AST0(AST_TERMINALS); }
              | TerminalSeqOpt TerminalDecl        { $$ = iappend($1, $2); }
              ;

/* each terminal has an integer code which is the integer value the
 * lexer uses to represent that terminal.  it is followed by at least
 * one alias, where it is the aliases that appear in the forms, rather
 * than the integer codes */
TerminalDecl: TOK_INTEGER ":" AliasSeq ";"         { $$ = AST2(AST_TERMDECL, $1, $3); }
            ;

AliasSeq: Alias                                    { $$ = AST1(AST_ALIASES, $1); }
        | AliasSeq Alias                           { $$ = iappend($1, $2); }
        ;

/* allow canonical names (e.g. TOK_COLON) and also mnemonic
 * strings (e.g. ":") for terminals */
Alias: TOK_NAME                                    { $$ = $1; }
     | TOK_STRING                                  { $$ = $1; }
     ;


/* ------ nonterminals ------ */
/*
 * a nonterminal is a grammar symbol that appears on the LHS of forms;
 * the body of the Nonterminal declaration specifies the the RHS forms,
 * attribute info, etc.
 */
Nonterminal: "nonterm" TOK_NAME "{" NonterminalBody "}"
               { $$ = AST2(AST_NONTERM, $2, $4); }
           | "nonterm" TOK_NAME Form
               { $$ = AST2(AST_NONTERM, $2, $3); }
           ;

NonterminalBody: /* empty */                       { $$ = AST0(AST_NTBODY); }
               | NonterminalBody AttributeDecl     { $$ = iappend($1, $2); }
               | NonterminalBody Form              { $$ = iappend($1, $2); }
               | NonterminalBody FormGroup         { $$ = iappend($1, $2); }
               ;

/*
 * this declares a synthesized attribute of a nonterminal; every form
 * must specify a value for every declared attribute; at least for
 * now, all attributes are integer-valued
 */
AttributeDecl: "attr" TOK_NAME ";"                 { $$ = AST1(AST_ATTR, $2); }
             ;

/*
 * a form is a context-free grammar production.  it specifies a
 * "rewrite rule", such that any instance of the nonterminal LHS can
 * be rewritten as the sequence of RHS symbols, in the process of
 * deriving the input sentance from the start symbol
 */
Form: "->" FormRHS ";"                             { $$ = AST1(AST_FORM, $2); }
    | "->" FormRHS "{" FormBody "}"                { $$ = AST2(AST_FORM, $2, $4); }
    ;

/*
 * if the form body is present, it is a list of actions and conditions
 * over the attributes; the conditions must be true for the form to be
 * usable, and if used, the actions are then executed to compute the
 * nonterminal's attribute values
 */
FormBody: /* empty */                              { $$ = AST0(AST_FORMBODY); }
        | FormBody Action                          { $$ = iappend($1, $2); }
        | FormBody Condition                       { $$ = iappend($1, $2); }
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
FormGroup: "formGroup" "{" FormGroupBody "}"       { $$ = $3; }
         ;

FormGroupBody: /* empty */                         { $$ = AST0(AST_FORMGROUPBODY); }
             | FormGroupBody Action                { $$ = iappend($1, $2); }
             | FormGroupBody Condition             { $$ = iappend($1, $2); }
             | FormGroupBody Form                  { $$ = iappend($1, $2); }
             | FormGroupBody FormGroup             { $$ = iappend($1, $2); }
             ;

/* ------ form right-hand side ------ */
/*
 * for now, the only extension to BNF is several alternatives for the
 * top level of a form; this is semantically equivalent to a formgroup
 * with the alternatives as alternative forms
 */
FormRHS: RHSEltSeqOpt              { $$ = $1; }
/*       | FormRHS "|" RHSEltSeqOpt   */
       ;

/*
 * each element on the RHS of a form can have a tag, which appears before a
 * colon (':') if present; the tag is required if that symbol's attributes
 * are to be referenced anywhere in the actions or conditions for the form
 */
RHSEltSeqOpt: /* empty */                          { $$ = AST0(AST_RHS); }

            | RHSEltSeqOpt TOK_NAME                { $$ = iappend($1, $2); }
                /* name (only) */
            | RHSEltSeqOpt TOK_NAME ":" TOK_NAME   { $$ = iappend($1, AST2(AST_TAGGEDNAME, $2, $4)); }
                /* tag : name */
            | RHSEltSeqOpt TOK_STRING              { $$ = iappend($1, $2); }
                /* mnemonic terminal */
            ;

/* ------ actions and conditions ------ */
/*
 * for now, the only action is to define the value of a synthesized
 * attribute
 */
Action: "action" TOK_NAME ":=" AttrExpr ";"        { $$ = AST2(AST_ACTION, $2, $4); }
      ;

/*
 * a condition is a boolean (0 or 1) valued expression; when it evaluates
 * to 0, the associated form cannot be used as a rewrite rule; when it evals
 * to 1, it can; any other value is an error
 */
Condition: "condition" AttrExpr ";"                { $$ = AST1(AST_CONDITION, $2); }
         ;


/* ------ attribute expressions, basically C expressions ------ */
PrimaryExpr: TOK_NAME "." TOK_NAME           { $$ = AST2(EXP_ATTRREF, $1, $3); }
               /* tag . attr */
           | TOK_INTEGER                     { $$ = $1; }
               /* literal */
           | "(" AttrExpr ")"                { $$ = $2; }
               /* grouping */
           | TOK_NAME "(" ExprListOpt ")"    { $$ = AST2(EXP_FNCALL, $1, $3); }
               /* function call */

           ;

ExprListOpt: /* empty */                     { $$ = AST0(EXP_LIST); }
           | ExprList                        { $$ = $1; }
           ;

ExprList: AttrExpr                           { $$ = AST1(EXP_LIST, $1) }
        | ExprList "," AttrExpr              { $$ = iappend($1, $3); }
        ;


UnaryExpr: PrimaryExpr                       { $$ = $1; }
         | "+" PrimaryExpr                   { $$ = $2; }    /* semantically ignored */
         | "-" PrimaryExpr                   { $$ = AST1(EXP_NEGATE, $2); }
         | "!" PrimaryExpr                   { $$ = AST1(EXP_NOT, $2); }
         ;

/* this rule relies on Bison precedence and associativity declarations */
BinaryExpr: UnaryExpr                        { $$ = $1; }
          | UnaryExpr "*" UnaryExpr          { $$ = AST2(EXP_MULT, $1, $3); }
          | UnaryExpr "/" UnaryExpr          { $$ = AST2(EXP_DIV, $1, $3); }
          | UnaryExpr "%" UnaryExpr          { $$ = AST2(EXP_MOD, $1, $3); }
          | UnaryExpr "+" UnaryExpr          { $$ = AST2(EXP_PLUS, $1, $3); }
          | UnaryExpr "-" UnaryExpr          { $$ = AST2(EXP_MINUS, $1, $3); }
          | UnaryExpr "<" UnaryExpr          { $$ = AST2(EXP_LT, $1, $3); }
          | UnaryExpr ">" UnaryExpr          { $$ = AST2(EXP_GT, $1, $3); }
          | UnaryExpr "<=" UnaryExpr         { $$ = AST2(EXP_LTE, $1, $3); }
          | UnaryExpr ">=" UnaryExpr         { $$ = AST2(EXP_GTE, $1, $3); }
          | UnaryExpr "==" UnaryExpr         { $$ = AST2(EXP_EQ, $1, $3); }
          | UnaryExpr "!=" UnaryExpr         { $$ = AST2(EXP_NEQ, $1, $3); }
          | UnaryExpr "&&" UnaryExpr         { $$ = AST2(EXP_AND, $1, $3); }
          | UnaryExpr "||" UnaryExpr         { $$ = AST2(EXP_OR, $1, $3); }
          ;

/* the expression "a ? b : c" means "if (a) then b, else c" */
CondExpr: BinaryExpr                             { $$ = $1; }
        | BinaryExpr "?" AttrExpr ":" AttrExpr   { $$ = AST3(EXP_COND, $1, $3, $5); }
        ;

AttrExpr: CondExpr                           { $$ = $1; }
        ;


%%



