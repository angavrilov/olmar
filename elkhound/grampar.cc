// grampar.cc
// additional C++ code for the grammar parser

#include "grampar.h"     // this module
#include "gramlex.h"     // GrammarLexer
#include "trace.h"       // tracing debug functions
#include "gramast.h"     // grammar AST nodes
#include "grammar.h"     // Grammar, Production, etc.

#include <fstream.h>     // ifstream


// ------------ mapping a grammar AST into a Grammar object -----------
Environment::Environment(Grammar &G)
  : g(G),
    prevEnv(NULL),
    sequenceVal(0)
{}

Environment::Environment(Environment &prev)
  : g(prev.g),
    prevEnv(&prev),
    sequenceVal(prev.sequenceVal)
{
  // copy the actions and conditions to simplify searching them later
  actions.appendAll(prev.actions);
  conditions.appendAll(prev.conditions);
}

Environment::~Environment()
{}


// fwd-decl of parsing fns
void astParseGrammar(Grammar &g, ASTNode const *treeTop);
void astParseNonterm(Environment &env, Nonterminal *nt,
                     ObjList<ASTNode> const &bodyList);
void astParseFormgroup(Environment &env, Nonterminal *nt,
                       ObjList<ASTNode> const &groupList);
void astParseForm(Environment &env, Nonterminal *nt,
                  ASTNode const *formNode);
Action *astParseAction(Environment &env, Production *prod,
                       ASTNode const *act);
Condition *astParseCondition(Environment &env, Production *prod,
                             ASTNode const *cond);
AExprNode *astParseExpr(Environment &env, Production *prod,
                        ASTNode const *node);
AExprNode *checkSpecial(Environment &env, Production *prod,
                        ASTNode const *node);



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

ASTNode const *nthChildt(ASTNode const *n, int c, int type)
{
  ASTNode const *ret = nthChild(n, c);
  xassert(ret->type == type);
  return ret;
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


// really a static semantic error, more than a parse error..
void astParseError(ASTNode const *node, char const *msg)
{
  xfailure(stringc << node->toString() << ": " << msg);
}


// to put at the end of a switch statement after all the
// legal types have been handled
#define END_TYPE_SWITCH(node) default: astParseError(node, "bad type") /* user ; */


void astParseGrammar(Grammar &g, ASTNode const *treeTop)
{
  xassert(treeTop->type == AST_TOPLEVEL);

  // default, empty environment
  Environment env(g);

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
            ok = env.g.declareToken(name, code, childString(term, 2));
          }
          else {
            ok = env.g.declareToken(name, code, NULL /*alias*/);
          }

          if (!ok) {
            astParseError(node, "token already declared");
          }
        }
        break;
      }

      case AST_NONTERM: {
        string name = childName(node, 0);
        Nonterminal *nt = g.getOrMakeNonterminal(name);
        if (nthChild(node, 1)->type == AST_FORM) {
          // simple "nonterm A -> B C D" form
          astParseForm(env, nt, nthChild(node, 1));
        }
        else {
          astParseNonterm(env, nt, childList(node, 1, AST_NTBODY));
        }
        break;
      }

      END_TYPE_SWITCH(node);
    }
  }
}


void astParseNonterm(Environment &env, Nonterminal *nt,
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
        astParseForm(env, nt, node);
        break;

      case AST_FORMGROUPBODY: {
        astParseFormgroup(env, nt, node->asInternalC().children);
        break;
      }

      END_TYPE_SWITCH(node);
    }
  }
}


void astParseFormgroup(Environment &prevEnv, Nonterminal *nt,
                       ObjList<ASTNode> const &groupList)
{
  // make a new environment for this stuff
  Environment env(prevEnv);

  // process each formgroup body element
  FOREACH_OBJLIST(ASTNode, groupList, iter) {
    ASTNode const *node = iter.data();

    switch (node->type) {
      case AST_ACTION:
        // just grab a pointer
        env.actions.append(const_cast<ASTNode*>(node));
        break;

      case AST_CONDITION:
        // same
        env.conditions.append(const_cast<ASTNode*>(node));
        break;

      case AST_FORM:
        astParseForm(env, nt, node);
        break;

      case AST_FORMGROUPBODY:
        astParseFormgroup(env, nt, nodeList(node, AST_FORMGROUPBODY));
        break;

      END_TYPE_SWITCH(node);
    }
  }
}


void astParseForm(Environment &env, Nonterminal *nt,
                  ASTNode const *formNode)
{
  xassert(formNode->type == AST_FORM);

  // every alternative separated by "|" requires the entire treatment
  FOREACH_OBJLIST(ASTNode, childList(formNode, 0, AST_RHSALTS), altIter) {
    ASTNode const *rhsListNode = altIter.data();

    // build a production; use 'this' as the tag for LHS elements
    Production *prod = new Production(nt, "this");

    // first, deal with RHS elements
    FOREACH_OBJLIST(ASTNode, nodeList(rhsListNode, AST_RHS), iter) {
      ASTNode const *n = iter.data();
      string symName;
      string symTag;
      bool tagValid = false;
      switch (n->type) {
        case AST_NAME:
          symName = nodeName(n);
          break;
          
        case AST_TAGGEDNAME:
          symName = childName(n, 1);
          symTag = childName(n, 0);
          tagValid = true;
          break;
          
        case AST_STRING:
          symName = nodeString(n);
          break;
          
        END_TYPE_SWITCH(n);
      }
      
      // see which (if either) thing this name already is
      Terminal *term = env.g.findTerminal(symName);
      Nonterminal *nonterm = env.g.findNonterminal(symName);
      xassert(!( term && nonterm ));     // better not be both!

      // some syntax rules
      if (n->type == AST_TAGGEDNAME  &&  term) {
        astParseError(n, "can't tag a terminal");
      }

      if (n->type == AST_STRING  &&  !term) {
        astParseError(n, "terminals must be declared");
      }

      // decide which symbol to put in the production
      Symbol *s;
      if (nonterm) {
        s = nonterm;            // could do these two with a bitwise OR
      }                         // if I were feeling extra clever today
      else if (term) {
        s = term;
      }
      else {
        // not declared as either; I require all tokens to be
        // declared, so this must be a new nonterminal
        s = env.g.getOrMakeNonterminal(symName);
      }

      // add it to the production
      prod->append(s, tagValid? (char const*)symTag : (char const*)NULL);
    }

    // after constructing the production we need to do this
    prod->finished();

    // deal with formBody (we evaluate it once for each alternative form)
    if (formNode->asInternalC().numChildren() == 2) {
      // iterate over form body elements
      FOREACH_OBJLIST(ASTNode, childList(formNode, 1, AST_FORMBODY), iter) {
        ASTNode const *n = iter.data();
        switch (n->type) {
          case AST_ACTION:
            prod->actions.actions.append(
              astParseAction(env, prod, n));
            break;

          case AST_CONDITION:
            prod->conditions.conditions.append(
              astParseCondition(env, prod, n));
            break;

          END_TYPE_SWITCH(n);
        }
      }
    }
    else {
      // no form body
    }

    // grab actions from the environment
    FOREACH_OBJLIST(string, nt->attributes, attrIter) {
      char const *attr = attrIter.data()->pcharc();
      if (!prod->actions.getAttrActionFor(attr)) {
        // the production doesn't currently have a rule to set
        // attribute 'attr'; look in the environment for one
        bool foundOne = false;
        SFOREACH_OBJLIST(ASTNode, env.actions, actIter) {
          ASTNode const *actNode = actIter.data();
          if (childName(actNode, 0) == string(attr)) {
            // found a suitable action from the environment;
            // parse it now
            Action *action = astParseAction(env, prod, actNode);
            prod->actions.actions.append(action);

            // break out of the loop
            foundOne = true;
            break;
          }
        }

        if (!foundOne) {
          // for now, just a warning
          // TODO: elevate to error
          cout << "WARNING: rule " << *prod
               << " has no action for `" << attr << "'\n";
        }
      }
    }

    // grab conditions from the environment
    SFOREACH_OBJLIST(ASTNode, env.conditions, envIter) {
      prod->conditions.conditions.append(
        astParseCondition(env, prod, envIter.data()));
    }

    // add production to grammar
    env.g.addProduction(prod);

  } // end of for-each alternative
}


Action *astParseAction(Environment &env, Production *prod,
                       ASTNode const *act)
{
  xassert(act->type == AST_ACTION);

  string attrName = childName(act, 0);
  if (!prod->left->hasAttribute(attrName)) {
    astParseError(act, "undeclared attribute");
  }

  AExprNode *expr = astParseExpr(env, prod, nthChild(act, 1));

  return new AttrAction(AttrLvalue(0 /*LHS*/, attrName),
                        expr);
}


Condition *astParseCondition(Environment &env, Production *prod,
                             ASTNode const *cond)
{
  xassert(cond->type == AST_CONDITION);

  AExprNode *expr = astParseExpr(env, prod, nthChild(cond, 0));

  return new ExprCondition(expr);
}


// parse an expression AST (action or condition), in the context
// of the production where that expression will be eval'd
AExprNode *astParseExpr(Environment &env, Production *prod,
                        ASTNode const *node)
{
  switch (node->type) {
    case AST_INTEGER:
      return new AExprLiteral(nodeInt(node));

    case EXP_ATTRREF: {
      // tag names the RHS symbol we're referring to
      string tag = childName(node, 0);
      int tagNum = prod->findTag(tag);
      if (tagNum == -1) {
        astParseError(node, "invalid tag");
      }

      // attrName says which attribute of RHS symbol to use
      string attrName = childName(node, 1);

      return new AExprAttrRef(AttrLvalue(tagNum, attrName));
    }

    case EXP_FNCALL: {
      // handle any eval-at-grammar-parse functions
      AExprNode *special = checkSpecial(env, prod, node);
      if (special) {
        return special;
      }

      // function name
      AExprFunc *fn = new AExprFunc(childName(node, 0));

      // and arguments
      FOREACH_OBJLIST(ASTNode, childList(node, 1, EXP_LIST), iter) {
        fn->args.append(astParseExpr(env, prod, iter.data()));
      }

      return fn;
    }
  }

  // what's left is the arithmetic operators; we just need to get the
  // function name, then handle args
  loopi(AExprFunc::numFuncEntries) {
    AExprFunc::FuncEntry const *entry = &AExprFunc::funcEntries[i];

    if (entry->astTypeCode == node->type) {
      // function name
      AExprFunc *fn = new AExprFunc(entry->name);

      // arguments
      ObjList<ASTNode> const &children = node->asInternalC().children;
      xassert(entry->numArgs == children.count());
      FOREACH_OBJLIST(ASTNode, children, iter) {
        fn->args.append(astParseExpr(env, prod, iter.data()));
      }

      return fn;
    }
  }

  // wasn't an ast expression node type we recongnize
  astParseError(node, "bad type");
  return NULL;    // silence warning
}


// 'node' is a function node; if the function is one we recognize
// as special, handle it; otherwise return NULL
AExprNode *checkSpecial(Environment &env, Production *prod,
                        ASTNode const *node)
{
  // for now, this is only special
  if (childName(node, 0) == string("sequence")) {
    // convenience
    int &seq = env.sequenceVal;
    ASTNode const *args = nthChild(node, 1);

    // this is a rather flaky implementation.. it's unclear (to the
    // user) what resets the counter, and that there's only one in
    // any given context.. I'll improve it when I need to ..

    if (seq == 0) {
      // start the sequence
      seq = childInt(args, 0);
    }
    else {
      // increment current value
      ASTNode const *incNode = nthChild(args, 1);
      if (incNode->type == AST_INTEGER) {
        seq += nodeInt(incNode);
      }
      else if (incNode->type == EXP_NEGATE) {
        // hacky special case.. should have a const-eval..
        seq -= childInt(incNode, 0);
      }
      else {
        astParseError(node, "sequence args need to be ints (or -ints)");
      }
    }

    // install the sequence value as the expression value
    return new AExprLiteral(seq);
  }

  // nothing else is special
  return NULL;
}


// ----------------------- parser support ---------------------
// Bison parser calls this to get a token
int yylex(ASTNode **lvalp, void *parseParam)
{
  ParseParams *par = (ParseParams*)parseParam;
  GrammarLexer &lexer = par->lexer;

  int code = lexer.yylexInc();

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
    lexer = new GrammarLexer(argv[1], in);
  }

  ParseParams params(*lexer);

  cout << "go!\n";
  int retval = yyparse(&params);
  if (retval == 0) {
    cout << "parsing finished successfully.\n";

    if (tracingSys("ast")) {
      // print AST
      cout << "AST:\n";
      params.treeTop->debugPrint(cout, 2);
    }

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
