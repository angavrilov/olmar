/* grammar.y
 * parser for grammar files */

 
/* ===================== tokens ============================ */
/* tokens that have many lexical spellings */
%token TOK_INTEGER
%token TOK_NAME

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

/* keywords */
%token TOK_NONTERM "nonterm"
%token TOK_FORMGROUP "formGroup"
%token TOK_FORM "form"
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

/* start symbol */
Input: /* empty */
     | Terminals Input
     | Nonterminal Input
     ;

/* ------ terminals ------ */
/*
 * the terminals are the grammar symbols that appear only on the RHS of
 * forms; they are the output of the lexer
 */
Terminals: "{" TerminalSeqOpt "}"
         ;

TerminalSeqOpt: /* empty */
              | TerminalSeqOpt Terminal
              ;

/*
 * each terminal has an integer code which is the integer value the lexer uses
 * to represent that terminal.  it is followed by at least one alias, where it is
 * the aliases that appear in the forms, rather than the integer codes
 */
Terminal: TOK_INTEGER ":" AliasSeq ";"
        ;

AliasSeq: TOK_NAME
        | AliasSeq TOK_NAME
        ;


/* ------ nonterminals ------ */
/* a nonterminal is a grammar symbol that appears on the LHS of forms */
Nonterminal: "nonterm" TOK_NAME "{" NonterminalBody "}"
           | "nonterm" TOK_NAME Form
           ;

NonterminalBody: /* empty */
               | NonterminalBody AttributeDecl
               | NonterminalBody Form
               | NonterminalBody FormGroup
               ;

/*
 * this declares a synthesized attribute of a nonterminal; every form must specify
 * a value for this attribute 
 */
AttributeDecl: "attr" TOK_NAME ";"
             ;

/*
 * a form is a context-free grammar production.  it specifies a "rewrite rule", such that any
 * instance of the nonterminal LHS can be rewritten as the sequence of RHS symbols, in the process
 * of deriving the input sentance from the start symbol
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
 * to all forms in the group
 */
FormGroup: "formGroup" "{" FormGroupBody "}"
         ;

FormGroupBody: /* empty */
             | FormGroupBody Action
             | FormGroupBody Condition
             | FormGroupBody Form
             ;

/* ------ form right-hand side ------ */
/*
 * for now, the only extension to BNF is several alternatives for the
 * top level of a form
 */
FormRHS: TaggedNameSeqOpt
       | FormRHS "|" TaggedNameSeqOpt
       ;

/*
 * each element on the RHS of a form can have a tag, which appears before a
 * colon (':') if present; the tag is required if that symbol's attributes
 * are referenced anywhere in the actions or conditions for the form
 */
TaggedNameSeqOpt: /* empty */
                | TaggedNameSeqOpt TOK_NAME                 /* no tag */
                | TaggedNameSeqOpt TOK_NAME ":" TOK_NAME    /* tag */
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
PrimaryExpr: TOK_NAME
           | TOK_INTEGER
           | "(" AttrExpr ")"
           ;

UnaryExpr: PrimaryExpr
         | "+" PrimaryExpr
         | "-" PrimaryExpr
         | "!" PrimaryExpr
         ;

/* this rule relies on precedence and associativity declarations */
BinaryExpr: UnaryExpr
          | UnaryExpr "*" UnaryExpr
          | UnaryExpr "/" UnaryExpr
          | UnaryExpr "%" UnaryExpr
          | UnaryExpr "+" UnaryExpr
          | UnaryExpr "-" UnaryExpr
          | UnaryExpr "<" UnaryExpr
          | UnaryExpr ">" UnaryExpr
          | UnaryExpr "<=" UnaryExpr
          | UnaryExpr ">=" UnaryExpr
          | UnaryExpr "==" UnaryExpr
          | UnaryExpr "!=" UnaryExpr
          | UnaryExpr "&&" UnaryExpr
          | UnaryExpr "||" UnaryExpr
          ;

/* the expression "a ? b : c" means "if (a) then b, else c"
CondExpr: BinaryExpr
        | BinaryExpr "?" AttrExpr ":" AttrExpr
        ;

AttrExpr: CondExpr
        ;


%%



