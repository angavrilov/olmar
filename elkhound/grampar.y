/* grammar.y
 * parser for grammar files */


/* C declarations */
%{

#include "grampar.h"        // yylex, etc.
#include "attrexpr.h"       // AExprNode, etc.

#include <stdlib.h>         // malloc, free
#include <iostream.h>       // cout

typedef ObjList<AExprNode> ExprList;

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


/* ==================== semantic types ===================== */
/* all the possible semantic types */
%union {
  int num;                       /* for int literals */
  string *str;                   /* (serf) for string literals and names */
  AExprNode *exprNode;           /* (owner) a node in an attribute expression */
  ExprList *expList;             /* (owner, nullable) list of expressions */
}

/* mapping between terminals and their semantic types */
%type <num>          TOK_INTEGER
%type <str>          TOK_STRING TOK_NAME

/* mapping between nonterminals and their semantic types */
%type <exprNode>     AttrExpr CondExpr BinaryExpr UnaryExpr PrimaryExpr
%type <AExprNode>    ExprListOpt ExprList


/* ===================== productions ======================= */
%%

/* start symbol */
Input: /* empty */
     | Terminals Input
     | Nonterminal Input
     ;

/* ------ terminals ------ */
/*
 * the terminals are the grammar symbols that appear only on the RHS of
 * forms; they are the output of the lexer; the Terminals list declares
 * all of the terminals that will appear in the rules
 */
Terminals: "terminals" "{" TerminalSeqOpt "}"
         ;

TerminalSeqOpt: /* empty */
              | TerminalSeqOpt TerminalDecl
              ;

/* each terminal has an integer code which is the integer value the
 * lexer uses to represent that terminal.  it is followed by at least
 * one alias, where it is the aliases that appear in the forms, rather
 * than the integer codes */
TerminalDecl: TOK_INTEGER ":" AliasSeq ";"
            ;

AliasSeq: Alias
        | AliasSeq Alias
        ;

/* allow canonical names (e.g. TOK_COLON) and also mnemonic
 * strings (e.g. ":") for terminals */
Alias: TOK_NAME
     | TOK_STRING
     ;


/* ------ nonterminals ------ */
/*
 * a nonterminal is a grammar symbol that appears on the LHS of forms;
 * the body of the Nonterminal declaration specifies the the RHS forms,
 * attribute info, etc.
 */
Nonterminal: "nonterm" TOK_NAME "{" NonterminalBody "}"
           | "nonterm" TOK_NAME Form
           ;

NonterminalBody: /* empty */
               | NonterminalBody AttributeDecl
               | NonterminalBody Form
               | NonterminalBody FormGroup
               ;

/*
 * this declares a synthesized attribute of a nonterminal; every form
 * must specify a value for every declared attribute; at least for
 * now, all attributes are integer-valued
 */
AttributeDecl: "attr" TOK_NAME ";"
             ;

/*
 * a form is a context-free grammar production.  it specifies a
 * "rewrite rule", such that any instance of the nonterminal LHS can
 * be rewritten as the sequence of RHS symbols, in the process of
 * deriving the input sentance from the start symbol
 */
Form: "->" FormRHS ";"
    | "->" FormRHS "{" FormBody "}"
    ;

/*
 * if the form body is present, it is a list of actions and conditions
 * over the attributes; the conditions must be true for the form to be
 * usable, and if used, the actions are then executed to compute the
 * nonterminal's attribute values
 */
FormBody: /* empty */
        | FormBody Action
        | FormBody Condition
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
FormGroup: "formGroup" "{" FormGroupBody "}"
         ;

FormGroupBody: /* empty */
             | FormGroupBody Action
             | FormGroupBody Condition
             | FormGroupBody Form
             | FormGroupBody FormGroup
             ;

/* ------ form right-hand side ------ */
/*
 * for now, the only extension to BNF is several alternatives for the
 * top level of a form; this is semantically equivalent to a formgroup
 * with the alternatives as alternative forms
 */
FormRHS: RHSEltSeqOpt
       | FormRHS "|" RHSEltSeqOpt
       ;

/*
 * each element on the RHS of a form can have a tag, which appears before a
 * colon (':') if present; the tag is required if that symbol's attributes
 * are to be referenced anywhere in the actions or conditions for the form
 */
RHSEltSeqOpt: /* empty */
            | RHSEltSeqOpt TOK_NAME                 /* name (only) */
            | RHSEltSeqOpt TOK_NAME ":" TOK_NAME    /* tag : name */
            | RHSEltSeqOpt TOK_STRING               /* mnemonic terminal */
            ;

/* ------ actions and conditions ------ */
/*
 * for now, the only action is to define the value of a synthesized
 * attribute
 */
Action: "action" TOK_NAME ":=" AttrExpr ";"
      ;

/*
 * a condition is a boolean (0 or 1) valued expression; when it evaluates
 * to 0, the associated form cannot be used as a rewrite rule; when it evals
 * to 1, it can; any other value is an error
 */
Condition: "condition" AttrExpr ";"
         ;


/* ------ attribute expressions, basically C expressions ------ */
PrimaryExpr: TOK_NAME "." TOK_NAME         /* tag . attr */
               { return new ?? ($1, $3); }
           | TOK_INTEGER                   /* literal */
               { return new AExprLiteral($1); }
           | "(" AttrExpr ")"              /* grouping */
               { return $2; }
           | TOK_NAME "(" ExprListOpt ")"  /* function call */
               {
                 AExprFunc *ret = new AExprFunc($1);
                 if ($3) {
                   ret->args.concat(*$3);
                   delete $3;
                 }
               }
           ;

ExprListOpt: /* empty */       { return NULL; }
           | ExprList          { return $1; }
           ;

ExprList: AttrExpr
            {
              ExprList *ret = new ExprList;
              ret->append($1);
              return ret;
            }
        | ExprList "," AttrExpr
            {
              $1.append($3);
              return $1;
            }
        ;


UnaryExpr: PrimaryExpr          { return $1; }
         | "+" PrimaryExpr      { return $2; }    /* semantically ignored */
         | "-" PrimaryExpr      { return new fnExpr1("-", $2); }
         | "!" PrimaryExpr      { return new fnExpr1("!", $2); }
         ;

/* this rule relies on Bison precedence and associativity declarations */
BinaryExpr: UnaryExpr                   { return $1; }
          | UnaryExpr "*" UnaryExpr     { return new fnExpr2("*", $1, $3); }
          | UnaryExpr "/" UnaryExpr     { return new fnExpr2("*", $1, $3); }
          | UnaryExpr "%" UnaryExpr     { return new fnExpr2("*", $1, $3); }
          | UnaryExpr "+" UnaryExpr     { return new fnExpr2("*", $1, $3); }
          | UnaryExpr "-" UnaryExpr     { return new fnExpr2("*", $1, $3); }
          | UnaryExpr "<" UnaryExpr     { return new fnExpr2("*", $1, $3); }
          | UnaryExpr ">" UnaryExpr     { return new fnExpr2("*", $1, $3); }
          | UnaryExpr "<=" UnaryExpr    { return new fnExpr2("*", $1, $3); }
          | UnaryExpr ">=" UnaryExpr    { return new fnExpr2("*", $1, $3); }
          | UnaryExpr "==" UnaryExpr    { return new fnExpr2("*", $1, $3); }
          | UnaryExpr "!=" UnaryExpr    { return new fnExpr2("*", $1, $3); }
          | UnaryExpr "&&" UnaryExpr    { return new fnExpr2("*", $1, $3); }
          | UnaryExpr "||" UnaryExpr    { return new fnExpr2("*", $1, $3); }
          ;

/* the expression "a ? b : c" means "if (a) then b, else c" */
CondExpr: BinaryExpr
            { return $1; }
        | BinaryExpr "?" AttrExpr ":" AttrExpr
            { return new fnExpr3("if", $1, $3, $5); }
        ;

AttrExpr: CondExpr       { return $1; }
        ;


%%



