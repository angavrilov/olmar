// grampar.cc
// additional C++ code for the grammar parser

#include "grampar.h"     // this module
#include "gramlex.h"     // GrammarLexer
#include "trace.h"       // tracing debug functions
#include "gramast.h"     // grammar AST nodes
#include "grammar.h"     // Grammar, Production, etc.

#include <fstream.h>     // ifstream


// ------------ mapping a grammar AST into a Grammar object -----------

// fwd-decl of parsing fns
void astParseGrammar(Grammar &g, ASTNode const *treeTop);
void astParseNonterm(Grammar &g, Nonterminal *nt, Environment &env,
                     ObjList<ASTNode> const &bodyList);
void astParseFormgroup(Grammar &g, Nonterminal *nt, Environment &env,
                       ObjList<ASTNode> const &groupList);
void astParseForm(Grammar &g, Nonterminal *nt, Environment &,
                  ASTNode const *formNode);



ASTIntLeaf const *asIntLeafC(ASTNode const *n)
{
  xassert(n->type == AST_INTEGER);
  return (ASTIntLeaf const *)n;
}

ASTStringLeaf const *asStringLeafC(ASTNode const *n)
{
  xassert(n->type == AST_STRING);
  return (ASTStringLeaf const *)n;
}

ASTNameLeaf const *asNameLeafC(ASTNode const *n)
{
  xassert(n->type == AST_NAME);
  return (ASTNameLeaf const *)n;
}


ASTNode const *nthChild(ASTNode const *n, int c)
{
  return n->asInternalC().children.nthC(c);
}


int nodeInt(ASTNode const *n)
  { return asIntLeafC(n)->data; }
string nodeName(ASTNode const *n)
  { return asNameLeafC(n)->data; }
string nodeString(ASTNode const *n)
  { return stringc << "\"" << asStringLeafC(n)->data << "\""; }

ObjList<ASTNode> const &nodeList(ASTNode const *n, int type)
{
  xassert(n->type == type);
  return n->asInternalC().children;
}


int childInt(ASTNode const *n, int c)
  { return nodeInt(nthChild(n, c)); }
string childName(ASTNode const *n, int c)
  { return nodeName(nthChild(n, c)); }
string childString(ASTNode const *n, int c)
  { return nodeString(nthChild(n, c)); }
ObjList<ASTNode> const &childList(ASTNode const *n, int c, int type)
  { return nodeList(nthChild(n, c), type); }

  
// to put at the end of a switch statement after all the
// legal types have been handled
#define END_TYPE_SWITCH default: xfailure("bad type");


void astParseGrammar(Grammar &g, ASTNode const *treeTop)
{
  // default, empty environment
  Environment env;

  // process each toplevel form
  FOREACH_OBJLIST(ASTNode, nodeList(treeTop, AST_TOPLEVEL), iter) {
    // at this level it's always an internal node
    ASTInternal const *node = &(iter.data()->asInternalC());

    switch (node->type) {
      case AST_TERMINALS: {
        // loop over the terminal declarations
        FOREACH_OBJLIST(ASTNode, node->children, termIter) {
          ASTInternal const *term = &( termIter.data()->asInternalC() );
          xassert(term->type == AST_TERMDECL);

          // process the terminal declaration
          int code = childInt(term, 0);
          string name = childName(term, 1);
          bool ok;
          if (term->numChildren() == 3) {
            ok = g.declareToken(name, code, childString(term, 2));
          }
          else {
            ok = g.declareToken(name, code, NULL /*alias*/);
          }

          if (!ok) {
            xfailure(stringc << "token " << name << " already declared");
          }
        }
        break;
      }

      case AST_NONTERM: {
        string name = childName(node, 0);
        Nonterminal *nt = g.getOrMakeNonterminal(name);
        if (nthChild(node, 1)->type == AST_FORM) {
          // simple "nonterm A -> B C D" form
          astParseForm(g, nt, env, nthChild(node, 1));
        }
        else {
          astParseNonterm(g, nt, env, childList(node, 1, AST_NTBODY));
        }
        break;
      }

      END_TYPE_SWITCH
    }
  }
}


void astParseNonterm(Grammar &g, Nonterminal *nt, Environment &env,
                     ObjList<ASTNode> const &bodyList)
{
  // process each nonterminal body element
  FOREACH_OBJLIST(ASTNode, bodyList, iter) {
    ASTNode const *node = iter.data();

    switch (node->type) {
      case AST_ATTR:
        nt->attributes.append(new string(childName(node, 0)));
        break;

      case AST_FORM:
        astParseForm(g, nt, env, node);
        break;

      case AST_FORMGROUPBODY: {
        astParseFormgroup(g, nt, env,
                          node->asInternalC().children);
        break;
      }

      END_TYPE_SWITCH
    }
  }
}


void astParseFormgroup(Grammar &g, Nonterminal *nt, Environment &env,
                       ObjList<ASTNode> const &groupList)
{
  // process each formgroup body element
  FOREACH_OBJLIST(ASTNode, groupList, iter) {
    ASTNode const *node = iter.data();

    switch (node->type) {
      case AST_ACTION:
      case AST_CONDITION:
        // TODO: handle these
        break;

      case AST_FORM:
        astParseForm(g, nt, env, node);
        break;

      case AST_FORMGROUPBODY:
        astParseFormgroup(g, nt, env, nodeList(node, AST_FORMGROUPBODY));
        break;

      END_TYPE_SWITCH
    }
  }
}


void astParseForm(Grammar &g, Nonterminal *nt, Environment &,
                  ASTNode const *formNode)
{
  // build a production
  Production *prod = new Production(nt, NULL /*tag*/);

  // first, deal with RHS of "->"
  FOREACH_OBJLIST(ASTNode, childList(formNode, 0, AST_RHS), iter) {
    ASTNode const *n = iter.data();
    switch (n->type) {
      case AST_NAME:
        prod->append(g.getOrMakeSymbol(nodeName(n)), NULL /*tag*/);
        break;

      case AST_TAGGEDNAME:
        prod->append(g.getOrMakeSymbol(childName(n, 1)),   // name
                     childName(n, 0));                     // tag
        break;

      case AST_STRING:
        prod->append(g.getOrMakeSymbol(nodeString(n)), NULL /*tag*/);
        break;

      END_TYPE_SWITCH
    }
  }

  // TODO: deal with formBody

  // add production to grammar
  g.addProduction(prod);
}


// ----------------------- parser support ---------------------
// Bison parser calls this to get a token
int yylex(ASTNode **lvalp, void *parseParam)
{
  ParseParams *par = (ParseParams*)parseParam;
  GrammarLexer &lexer = par->lexer;

  int code = lexer.yylex();
  trace("yylex") << "yielding token (" << code << ") "
                 << lexer.curToken() << " at "
                 << lexer.curLoc() << endl;

  // yield semantic values for some things
  switch (code) {
    case TOK_INTEGER:
      *lvalp = new ASTIntLeaf(lexer.integerLiteral);
      break;

    case TOK_STRING:
      *lvalp = new ASTStringLeaf(lexer.stringLiteral);
      break;

    case TOK_NAME:
      *lvalp = new ASTNameLeaf(lexer.curToken());
      break;

    default:
      *lvalp = NULL;
  }

  return code;
}


void my_yyerror(char const *message, void *parseParam)
{
  ParseParams *par = (ParseParams*)parseParam;
  cout << message << " at " << par->lexer.curLoc() << endl;
}


#ifdef TEST_GRAMPAR

int main(int argc, char **argv)
{
  TRACE_ARGS();

  ASTNode::typeToString = astTypeToString;

  GrammarLexer *lexer;
  ifstream *in = NULL;
  if (argc == 1) {
    // stdin
    lexer = new GrammarLexer;
  }
  else {
    // file           
    in = new ifstream(argv[1]);
    if (!*in) {
      cout << "error opening input file " << argv[1] << endl;
      return 2;
    }
    lexer = new GrammarLexer(in);
  }

  ParseParams params(*lexer);

  cout << "go!\n";
  int retval = yyparse(&params);
  if (retval == 0) {
    cout << "parsing finished successfully.\n";

    // print AST
    cout << "AST:\n";
    params.treeTop->debugPrint(cout, 2);
                
    // parse the AST into a Grammar
    Grammar g;
    astParseGrammar(g, params.treeTop);

    // and print that
    cout << "parsed productions:\n";
    g.printProductions(cout);

    delete params.treeTop;
    cout << "leaked " << ASTNode::nodeCount << " AST nodes\n";
  }
  else {
    cout << "parsing finished with an error.\n";
  }

  delete lexer;
  if (in) {
    delete in;
  }

  return retval;
}

#endif // TEST_GRAMPAR
