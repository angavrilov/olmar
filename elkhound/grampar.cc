// grampar.cc
// additional C++ code for the grammar parser

#include "grampar.h"     // this module
#include "gramlex.h"     // GrammarLexer
#include "trace.h"       // tracing debug functions
#include "gramast.h"     // grammar AST nodes
#include "grammar.h"     // Grammar, Production, etc.
#include "owner.h"       // Owner
#include "syserr.h"      // xsyserror
#include "strutil.h"     // quoted

#include <fstream.h>     // ifstream


// ------------ mapping a grammar AST into a Grammar object -----------
Environment::Environment(Grammar &G)
  : g(G),
    prevEnv(NULL),
    inherited(),
    sequenceVal(0)
{}

Environment::Environment(Environment &prev)
  : g(prev.g),
    prevEnv(&prev),
    inherited(),
    sequenceVal(prev.sequenceVal)
{
  // copy the actions and conditions to simplify searching them later
  inherited.appendAll(prev.inherited);
}

Environment::~Environment()
{}


// fwd-decl of parsing fns
void astParseGrammar(Grammar &g, ASTNode const *treeTop);
void astParseNonterminalBody(Environment &env, Nonterminal *nt,
                             ObjList<ASTNode> const &bodyList);
void astParseFormgroup(Environment &env, Nonterminal *nt,
                       ObjList<ASTNode> const &groupList);
void astParseGroupBody(Environment &env, Nonterminal *nt,
                       ObjList<ASTNode> const &bodyList,
                       bool attrDeclAllowed);
void astParseForm(Environment &env, Nonterminal *nt,
                  ASTNode const *formNode);
void astParseFormBodyElt(Environment &env, Production *prod,
                         ASTNode const *n, bool dupsOk);
void astParseAction(Environment &env, Production *prod,
                    ASTNode const *act, bool dupsOk);
Condition *astParseCondition(Environment &env, Production *prod,
                             ASTNode const *cond);
void astParseFunction(Environment &env, Production *prod,
                      ASTNode const *func, bool dupsOk);
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
  { return asStringLeafC(n)->data; }

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
  SourceLocation loc;
  if (node->leftmostLoc(loc)) {
    xfailure(stringc << "near " << loc.toString() << ", at "
                     << node->toString() << ": " << msg);
  }
  else {
    xfailure(stringc << "(?loc) "
                     << node->toString() << ": " << msg);
  }
}


// to put at the end of a switch statement after all the
// legal types have been handled
#define END_TYPE_SWITCH(node) default: astParseError(node, "bad type") /* user ; */


// ---------------------- AST "parser" --------------------------
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
            ok = env.g.declareToken(name, code, quoted(childString(term, 2)));
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

      case AST_PROLOGUE:
        if (g.semanticsPrologue.length() != 0) {
          astParseError(node, "prologue already defined");
        }
        g.semanticsPrologue = childString(node, 0);
        break;

      case AST_EPILOGUE:
        if (g.semanticsEpilogue.length() != 0) {
          astParseError(node, "epilogue already defined");
        }
        g.semanticsEpilogue = childString(node, 0);
        break;

      case AST_NONTERM: {
        string name = childName(node, 0);
        
        // TODO: need to check that this wasn't already declared; the code
        // below doesn't work because another rule could simply have
        // *referred* to this nonterminal
        //if (g.findNonterminal(name)) {
        //  astParseError(nthChild(node, 0), "nonterminal already declared");
        //}

        Nonterminal *nt = g.getOrMakeNonterminal(name);
        if (nthChild(node, 1)->type == AST_FORM) {
          // simple "nonterm A -> B C D" form
          astParseForm(env, nt, nthChild(node, 1));
        }
        else {
          astParseNonterminalBody(env, nt, childList(node, 1, AST_NTBODY));
        }
        break;
      }

      END_TYPE_SWITCH(node);
    }
  }
}


// this is for parsing NonterminalBody (attrDeclAllowed=true) or
// FormGroupBody (attrDeclAllowed=false) lists
void astParseGroupBody(Environment &prevEnv, Nonterminal *nt,
                       ObjList<ASTNode> const &bodyList,
                       bool attrDeclAllowed)
{
  // make a new environment for this stuff
  Environment env(prevEnv);

  // process each body element
  FOREACH_OBJLIST(ASTNode, bodyList, iter) {
    ASTNode const *node = iter.data();

    switch (node->type) {
      case AST_ATTR:
        if (attrDeclAllowed) {
          nt->attributes.append(new string(childName(node, 0)));
        }
        else {
          // should never happen with current grammar, but if I collapse
          // the grammar then it might..
          astParseError(node, "can only declare attributes in nonterminals");
        }
        break;

      case AST_FUNDECL:
        if (attrDeclAllowed) {
          nt->funDecls.add(
            childName(node, 0),      // declared name
            childString(node, 1));   // declaration body
        }
        else {
          // cannot happen with current grammar
          astParseError(node, "can only declare functions in nonterminals");
        }
        break;

      case AST_ACTION:
      case AST_CONDITION:
      case AST_FUNCTION:
      case AST_FUNEXPR:
        // just grab a pointer; will parse for real when a production
        // uses this inherited action
        env.inherited.append(const_cast<ASTNode*>(node));
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


void astParseNonterminalBody(Environment &env, Nonterminal *nt,
                             ObjList<ASTNode> const &bodyList)
{
  astParseGroupBody(env, nt, bodyList, true /*attrDeclAllowed*/);
}

void astParseFormgroup(Environment &env, Nonterminal *nt,
                       ObjList<ASTNode> const &groupList)
{
  astParseGroupBody(env, nt, groupList, false /*attrDeclAllowed*/);
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
          symName = quoted(nodeString(n));
          break;
          
        END_TYPE_SWITCH(n);
      }
      
      // see which (if either) thing this name already is
      Terminal *term = env.g.findTerminal(symName);
      Nonterminal *nonterm = env.g.findNonterminal(symName);
      xassert(!( term && nonterm ));     // better not be both!

      // some syntax rules
      if (n->type == AST_STRING  &&  !term) {
        astParseError(n, "terminals must be declared");
      }
        
      // I eliminated this rule because I need the tags to
      // be able to refer to tokens in semantic functions
      //if (n->type == AST_TAGGEDNAME  &&  term) {
      //  astParseError(n, "can't tag a terminal");
      //}

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

      if (s->isEmptyString) {
        // "empty" is a syntactic convenience; it doesn't get
        // added to the production
      }
      else {
        // add it to the production
        prod->append(s, tagValid? (char const*)symTag : (char const*)NULL);
      }
    }

    // after constructing the production we need to do this
    // no we don't -- GrammarAnalysis takes care of it (and
    // complains if we do)
    //prod->finished();

    // deal with formBody (we evaluate it once for each alternative form)
    if (formNode->asInternalC().numChildren() == 2) {
      // iterate over form body elements
      FOREACH_OBJLIST(ASTNode, childList(formNode, 1, AST_FORMBODY), iter) {
        astParseFormBodyElt(env, prod, iter.data(), false /*dupsOk*/);
      }
    }
    else {
      // no form body
    }

    // grab stuff inherited from the environment
    SFOREACH_OBJLIST(ASTNode, env.inherited, iter) {
      astParseFormBodyElt(env, prod, iter.data(), true /*dupsOk*/);
    }

    // make sure all attributes have actions
    FOREACH_OBJLIST(string, nt->attributes, attrIter) {
      char const *attr = attrIter.data()->pcharc();
      if (!prod->actions.getAttrActionFor(attr)) {
        astParseError(formNode,
          stringc << "rule " << prod->toString()
                  << " has no action for `" << attr << "'");
      }
    }

    // make sure all fundecls have implementations
    for (StringDict::IterC iter(nt->funDecls); 
         !iter.isDone(); iter.next()) {
      char const *name = iter.key();
      if (!prod->hasFunction(name)) {
        astParseError(formNode,
          stringc << "rule " << prod->toString()
                  << " has no implementation for `" << name << "'");
      }
    }

    // add production to grammar
    env.g.addProduction(prod);

  } // end of for-each alternative
}


void astParseFormBodyElt(Environment &env, Production *prod,
                         ASTNode const *n, bool dupsOk)
{
  switch (n->type) {
    case AST_ACTION:
      astParseAction(env, prod, n, dupsOk);
      break;

    case AST_CONDITION:
      prod->conditions.conditions.append(
        astParseCondition(env, prod, n));
      break;

    case AST_FUNDECL:    
      // allow function declarations in form bodies because it's
      // convenient for nonterminals that only have one form..
      prod->left->funDecls.add(
        childName(n, 0),      // declared name
        childString(n, 1));   // declaration body
      break;

    case AST_FUNCTION:
    case AST_FUNEXPR:
      astParseFunction(env, prod, n, dupsOk);
      break;

    END_TYPE_SWITCH(n);
  }
}


void astParseAction(Environment &env, Production *prod,
                    ASTNode const *act, bool dupsOk)
{
  xassert(act->type == AST_ACTION);

  string attrName = childName(act, 0);
  if (!prod->left->hasAttribute(attrName)) {
    astParseError(act, stringc << "undeclared attribute: " << attrName);
  }
  if (prod->actions.getAttrActionFor(attrName)) {
    // there is already an action for this attribute
    if (dupsOk) {
      return;     // fine; just ignore it
    }
    else {
      astParseError(act, stringc << "duplicate action for " << attrName);
    }
  }

  AExprNode *expr = astParseExpr(env, prod, nthChild(act, 1));

  prod->actions.actions.append(
    new AttrAction(AttrLvalue(0 /*LHS*/, attrName),
                   expr));
}


Condition *astParseCondition(Environment &env, Production *prod,
                             ASTNode const *cond)
{
  xassert(cond->type == AST_CONDITION);

  AExprNode *expr = astParseExpr(env, prod, nthChild(cond, 0));

  return new ExprCondition(expr);
}


void astParseFunction(Environment &env, Production *prod,
                      ASTNode const *func, bool dupsOk)
{
  xassert(func->type == AST_FUNCTION || 
          func->type == AST_FUNEXPR);

  string name = childName(func, 0);
  if (!prod->left->hasFunDecl(name)) {
    astParseError(func,
      stringc << "undeclared function: " << name);
  }
  if (prod->hasFunction(name)) {
    if (!dupsOk) {
      astParseError(func,
        stringc << "duplicate function implementation: " << name);
    }
    else {
      return;    // ignore duplicates
    }
  }

  string body = childString(func, 1);
  prod->functions.add(name, body);
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

    case AST_NAME:
      // no tag = implicit 'this' (=LHS) tag
      return new AExprAttrRef(AttrLvalue(0 /*LHS*/, nodeName(node)));

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
      *lvalp = new ASTIntLeaf(lexer.integerLiteral, lexer.curLoc());
      break;

    case TOK_STRING:
      *lvalp = new ASTStringLeaf(lexer.stringLiteral, lexer.curLoc());
      break;

    case TOK_NAME:
      *lvalp = new ASTNameLeaf(lexer.curToken(), lexer.curLoc());
      break;

    case TOK_FUNDECL_BODY: {
      // grab the declaration body and put it into a leaf
      ASTNode *declBody =
        new ASTStringLeaf(lexer.curDeclBody(), lexer.curLoc());

      // grab the declared function name and put it into another leaf
      ASTNode *declName =
        new ASTNameLeaf(lexer.curDeclName(), lexer.curLoc());

      // wrap them into another ast node
      *lvalp = AST2(AST_FUNDECL, declName, declBody);
      break;
    }

    case TOK_FUN_BODY:
      *lvalp = new ASTStringLeaf(lexer.curFuncBody(), lexer.curLoc());
      break;

    default:
      *lvalp = NULL;
  }

  return code;
}


void my_yyerror(char const *message, void *parseParam)
{
  ParseParams *par = (ParseParams*)parseParam;
  cout << message << " at " << par->lexer.curLocStr() << endl;
}


// ---------------- external interface -------------------
void readGrammarFile(Grammar &g, char const *fname)
{
  ASTNode::typeToString = astTypeToString;

  if (tracingSys("yydebug")) {
    yydebug = true;
  }

  Owner<GrammarLexer> lexer;
  Owner<ifstream> in;
  if (fname == NULL) {
    // stdin
    lexer = new GrammarLexer;
  }
  else {
    // file
    in = new ifstream(fname);
    if (!*in) {
      xsyserror("open", stringc << "error opening input file " << fname);
    }
    lexer = new GrammarLexer(fname, in.xfr());
  }

  ParseParams params(*lexer);

  traceProgress() << "parsing grammar source..\n";
  int retval = yyparse(&params);
  if (retval == 0) {
    // make sure the tree gets deleted
    Owner<ASTNode> treeTop(params.treeTop);

    if (tracingSys("ast")) {
      // print AST
      cout << "AST:\n";
      treeTop->debugPrint(cout, 2);
    }

    // parse the AST into a Grammar
    traceProgress() << "parsing grammar AST..\n";
    astParseGrammar(g, treeTop);

    // and print that
    if (tracingSys("grammar")) {
      g.printProductions(cout);
    }

    // then check grammar properties; throws exception
    // on failure
    g.checkWellFormed();

    treeTop.del();
    if (ASTNode::nodeCount > 0) {
      cout << "leaked " << ASTNode::nodeCount << " AST nodes\n";
    }
  }
  else {
    xbase("parsing finished with an error");
  }
}


#ifdef TEST_GRAMPAR

int main(int argc, char **argv)
{
  TRACE_ARGS();

  traceAddSys("progress");
  traceAddSys("grammar");

  Grammar g;
  readGrammarFile(g, argc>=2? argv[1] : NULL /*stdin*/);

  return 0;
}

#endif // TEST_GRAMPAR
