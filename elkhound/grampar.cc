// grampar.cc
// additional C++ code for the grammar parser; in essence,
// build the grammar internal representation out of what
// the user supplies in a .gr file

#include "grampar.h"     // this module
#include "gramlex.h"     // GrammarLexer
#include "trace.h"       // tracing debug functions
#include "gramast.h"     // grammar AST nodes
#include "grammar.h"     // Grammar, Production, etc.
#include "owner.h"       // Owner
#include "syserr.h"      // xsyserror
#include "strutil.h"     // quoted
#include "grampar.tab.h" // token constant codes

#include <fstream.h>     // ifstream


// ------------------------- Environment ------------------------
Environment::Environment(Grammar &G)
  : g(G),
    prevEnv(NULL),
    nontermDecls(),
    inherited(),
    sequenceVal(0)
{}

Environment::Environment(Environment &prev)
  : g(prev.g),
    prevEnv(&prev),
    nontermDecls(prev.nontermDecls),
    inherited(),
    sequenceVal(prev.sequenceVal)
{
  // copy the actions and conditions to simplify searching them later
  inherited.appendAll(prev.inherited);
}

Environment::~Environment()
{}


// ------------------ TreesContext -----------------
TreesContext::~TreesContext()
{}

int TreesContext::lookupTreeName(char const *tn) const
{
  if (t0Name.equals(tn)) { 
    return 0;
  }
  else if (t1Name.equals(tn)) {
    return 1;
  }
  else {
    return -1;
  }
}

bool TreesContext::dummyContext() const
{
  return t0Name.length() == 0;
}


// -------------------- XASTParse --------------------
STATICDEF string XASTParse::
  constructMsg(ASTNode const *node, char const *msg)
{
  if (node->hasLeftmostLoc()) {
    return stringc << "near " << node->getLeftmostLoc().toString()
                   << ", at " << node->toString() << ": " << msg;
  }
  else {
    return stringc << "(?loc) at "
                   << node->toString() << ": " << msg;
  }
}

XASTParse::XASTParse(ASTNode const *n, char const *m)
  : xBase(constructMsg(n, m)),
    node(n),
    message(m)
{}


XASTParse::XASTParse(XASTParse const &obj)
  : xBase(obj),
    DMEMB(node),
    DMEMB(message)
{}

XASTParse::~XASTParse()
{}


// -------------------- AST parser support ---------------------
// fwd-decl of parsing fns
void astParseGrammar(Grammar &g, ASTNode const *treeTop);
void astParseNonterm(Environment &env, ASTNode const *node,
                     Nonterminal *nonterm);
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
ExprCondition *astParseCondition(Environment &env, Production *prod,
                                 ASTNode const *cond);
void astParseTreeCompare(Environment &env, Production *prod,
                         ASTNode const *node);
void astParseFunction(Environment &env, Production *prod,
                      ASTNode const *func, bool dupsOk);
AExprNode *astParseExpr(Environment &env, Production *prod,
                        ASTNode const *node);
AExprNode *astParseExpr(Environment &env, Production *prod,
                        ASTNode const *node, TreesContext const &trees);
AExprNode *checkSpecial(Environment &env, Production *prod,
                        ASTNode const *node);



// really a static semantic error, more than a parse error..
void astParseError(ASTNode const *node, char const *msg)
{
  THROW(XASTParse(node, msg));
}

void checkType(ASTNode const *node, int type)
{ 
  if (node->type != type) {
    astParseError(node,
                  stringc << "wrong type; expected " 
                          << astTypeToString(type));
  }
}


ASTIntLeaf const *asIntLeafC(ASTNode const *n)
{
  checkType(n, AST_INTEGER);
  return (ASTIntLeaf const *)n;
}

ASTStringLeaf const *asStringLeafC(ASTNode const *n)
{
  checkType(n, AST_STRING);
  return (ASTStringLeaf const *)n;
}

ASTNameLeaf const *asNameLeafC(ASTNode const *n)
{
  checkType(n, AST_NAME);
  return (ASTNameLeaf const *)n;
}


ASTNode const *nthChild(ASTNode const *n, int c)
{
  return n->asInternalC().children.nthC(c);
}

ASTNode const *nthChildt(ASTNode const *n, int c, int type)
{
  ASTNode const *ret = nthChild(n, c);
  checkType(ret, type);
  return ret;
}


int numChildren(ASTNode const *n)
{
  return n->asInternalC().numChildren();
}


int nodeInt(ASTNode const *n)
  { return asIntLeafC(n)->data; }
string nodeName(ASTNode const *n)
  { return asNameLeafC(n)->data; }
string nodeString(ASTNode const *n)
  { return asStringLeafC(n)->data; }

ObjList<ASTNode> const &nodeList(ASTNode const *n, int type)
{
  checkType(n, type);
  return n->asInternalC().children;
}

SourceLocation nodeSrcLoc(ASTNode const *n)
{
  SourceLocation ret;
  if (!n->leftmostLoc(ret)) {
    astParseError(n, "no source location");
  }
  return ret;
}

LiteralCode * /*owner*/ nodeLitCode(ASTNode const *n)
{
  return new LiteralCode(nodeSrcLoc(n), nodeString(n));
}


int childInt(ASTNode const *n, int c)
  { return nodeInt(nthChild(n, c)); }
string childName(ASTNode const *n, int c)
  { return nodeName(nthChild(n, c)); }
string childString(ASTNode const *n, int c)
  { return nodeString(nthChild(n, c)); }
ObjList<ASTNode> const &childList(ASTNode const *n, int c, int type)
  { return nodeList(nthChild(n, c), type); }
SourceLocation childSrcLoc(ASTNode const *n, int c)
  { return nodeSrcLoc(nthChild(n, c)); }
LiteralCode * /*owner*/ childLitCode(ASTNode const *n, int c)
  { return nodeLitCode(nthChild(n, c)); }


// to put at the end of a switch statement after all the
// legal types have been handled
#define END_TYPE_SWITCH(node) default: astParseError(node, "bad type") /* user ; */


// to put as the catch block; so far it's kind of ad-hoc where
// I actually put 'try' blocks..
#define CATCH_APPLY_CONTEXT(node)       \
  catch (XASTParse &x) {                \
    /* leave unchanged */               \
    throw x;                            \
  }                                     \
  catch (xBase &x) {                    \
    /* add context */                   \
    astParseError(node, x.why());       \
    throw 0;     /* silence warning */  \
  }


// ---------------------- AST "parser" --------------------------
void astParseGrammar(Grammar &g, ASTNode const *treeTop)
{
  checkType(treeTop, AST_TOPLEVEL);

  // default, empty environment
  Environment env(g);

  // process each toplevel form
  FOREACH_OBJLIST(ASTNode, nodeList(treeTop, AST_TOPLEVEL), iter) {
    // at this level it's always an internal node
    ASTInternal const *node = &(iter.data()->asInternalC());
    try {

      switch (node->type) {
        case AST_TERMINALS: {
          // loop over the terminal declarations
          FOREACH_OBJLIST(ASTNode, node->children, termIter) {
            try {
              ASTInternal const *term = &( termIter.data()->asInternalC() );
              checkType(term, AST_TERMDECL);

              // process the terminal declaration
              int code = childInt(term, 0);
              string name = childName(term, 1);
              bool ok;
              if (numChildren(term) == 3) {
                ok = env.g.declareToken(name, code, quoted(childString(term, 2)));
              }
              else {
                ok = env.g.declareToken(name, code, NULL /*alias*/);
              }

              if (!ok) {
                astParseError(node, "token already declared");
              }
            } // try
            CATCH_APPLY_CONTEXT(termIter.data())
          } // for(terminal)
          break;
        }

        case AST_TREENODEBASE:
          g.treeNodeBaseClass = childString(node, 0);
          break;

        case AST_LITERALCODE: {
          // extract AST fields
          string tag = childString(node, 0);
          LiteralCode *code = childLitCode(node, 1);    // (owner)

          if (tag.equals("prologue")) {
            if (g.semanticsPrologue != NULL) {
              astParseError(node, "prologue already defined");
            }
            g.semanticsPrologue = code;
          }

          else if (tag.equals("epilogue")) {
            if (g.semanticsEpilogue != NULL) {
              astParseError(node, "epilogue already defined");
            }
            g.semanticsEpilogue = code;
          }

          else {
            astParseError(node, stringc << "unknown litcode tag: " << tag);
          }

          break;
        }

        case AST_NONTERM: {
          {
            // new environment since it can contain a grouping construct
            Environment newEnv(env);

            // parse it
            astParseNonterm(newEnv, node, NULL /*means original decl*/);
          }

          // add this decl to our running list (in the original environment)
          string name = childName(nthChildt(node, 0, AST_NTNAME), 0);
          env.nontermDecls.add(name, const_cast<ASTInternal*>(node));

          break;
        }

        END_TYPE_SWITCH(node);
      } // switch
    } // try
    CATCH_APPLY_CONTEXT(node)
  } // for(toplevel form)
}


void astParseNonterm(Environment &env, ASTNode const *node,
                     Nonterminal *nonterm)
{
  checkType(node, AST_NONTERM);
  bool originalDecl = !nonterm;

  ASTNode const *ntName = nthChildt(node, 0, AST_NTNAME);
  string name = childName(ntName, 0);

  if (originalDecl) {
    // check for already declared; don't add to the list
    // yet, though, as a safeguard against the base class
    // list referring to itself
    if (env.nontermDecls.isMapped(name)) {
      astParseError(nthChild(node, 0),    // to avoid error spew
                    "nonterminal already declared");
    }
  }

  // decide which nonterminal's context to use
  if (!originalDecl) {
    // we're re-parsing a base class; stay in the derived
    // class' (namely, 'nonterm') context
  }
  else {
    // real, original declaration: new context
    nonterm = env.g.getOrMakeNonterminal(name);
  }

  // are there any base classes?  (possibly a recursive question)
  if (numChildren(ntName) == 2) {
    ASTNode const *classes = nthChildt(ntName, 1, AST_BASECLASSES);

    // for each base class..
    loopi(numChildren(classes)) {
      // get its name
      string baseClassName = childName(classes, i);
                     
      // get its AST node
      if (!env.nontermDecls.isMapped(baseClassName)) {
        astParseError(nthChild(classes, i),
                      "undeclared base class");
      }
      ASTNode const *base = env.nontermDecls.queryf(baseClassName);
                         
      // map the name to a Nonterminal
      Nonterminal *baseNT = env.g.findNonterminal(baseClassName);
      xassert(baseNT);
      
      // record the inheritance in the grammar
      nonterm->superclasses.append(baseNT);

      // primary inheritance implementation mechanism: simply re-parse
      // the definition of the base classes, but in the context of the
      // new nonterminal
      try {
        astParseNonterm(env, base, nonterm);
      }
      catch (XASTParse &x) {
        // add additional context; parse errors in the base classes
        // are usually actually caused by errors in the place where
        // they're referenced, e.g. including a base class twice
        THROW(XASTParse(classes, x.why()));
      }
    }
  }

  // parse the declaration itself; do this *after* re-parsing
  // base classes because base classes generally supply things
  // like fun{} implementations which must be put into the
  // environment before the productions are analyzed
  if (nthChild(node, 1)->type == AST_FORM) {
    // simple "nonterm A -> B C D" form
    astParseForm(env, nonterm, nthChild(node, 1));
  }
  else {
    astParseNonterminalBody(env, nonterm, childList(node, 1, AST_NTBODY));
  }

  if (originalDecl) {
    // add this decl to our running list
    env.nontermDecls.add(name, const_cast<ASTNode*>(node));
  }
}


// this is for parsing NonterminalBody (attrDeclAllowed=true) or
// FormGroupBody (attrDeclAllowed=false) lists
void astParseGroupBody(Environment &env, Nonterminal *nt,
                       ObjList<ASTNode> const &bodyList,
                       bool attrDeclAllowed)
{
  // process each body element
  FOREACH_OBJLIST(ASTNode, bodyList, iter) {
    ASTNode const *node = iter.data();
    try {
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
              childName(node, 0),         // declared name
              childLitCode(node, 1));     // declaration body
          }
          else {
            // cannot happen with current grammar
            astParseError(node, "can only declare functions in nonterminals");
          }
          break;

        case AST_DECLARATION:
          if (attrDeclAllowed) {
            nt->declarations.append(
              childLitCode(node, 0));     // declaration body
          }
          else {
            // cannot happen with current grammar
            astParseError(node, "can only have declarations in nonterminals");
          }
          break;

        case AST_LITERALCODE:
        case AST_NAMEDLITERALCODE: {
          if (!attrDeclAllowed) {
            astParseError(node, "can't define literal code here");
          }

          // extract AST fields
          string tag = childString(node, 0);
          LiteralCode *code = childLitCode(node, 1);      // (owner)
          string name;
          if (node->type == AST_NAMEDLITERALCODE) {
            name = childName(node, 2);
          }

          // look at the tag to figure out what the code means
          if (tag.equals("disamb")) {
            if (!name[0]) {
              astParseError(node, "disamb must have a name");
            }
            if (!nt->funDecls.isMapped(name)) {
              astParseError(node, "undeclared function");
            }
            nt->disambFuns.add(name, code);
          }

          else if (tag.equals("prefix")) {
            if (!name[0]) {
              astParseError(node, "prefix must have a name");
            }
            if (!nt->funDecls.isMapped(name)) {
              astParseError(node, "undeclared function");
            }
            nt->funPrefixes.add(name, code);
          }

          else if (tag.equals("constructor")) {
            if (nt->constructor) {
              //astParseError(node, "constructor already defined");
              // hack: allow overriding ...
            }
            nt->constructor = code;
          }

          else if (tag.equals("destructor")) {
            if (nt->destructor) {
              astParseError(node, "destructor already defined");
            }
            nt->destructor = code;
          }

          else {
            astParseError(node, stringc << "unknown litcode tag: " << tag);
          }     

          break;
        }

        case AST_ACTION:
        case AST_CONDITION:
        case AST_FUNCTION:
        case AST_FUNEXPR:
          // just grab a pointer; will parse for real when a production
          // uses this inherited action
          // NOTE: must prepend here because the list is walked in order
          // when adding inherited items to productions, and later
          // additions must shadow earlier ones
          env.inherited.prepend(const_cast<ASTNode*>(node));
          break;

        case AST_FORM:
          astParseForm(env, nt, node);
          break;

        case AST_FORMGROUPBODY: {
          Environment newEnv(env);     // new environment
          astParseFormgroup(newEnv, nt, node->asInternalC().children);
          break;
        }

        END_TYPE_SWITCH(node);
      }
    } // try
    CATCH_APPLY_CONTEXT(node)
  } // for(body elt)
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
  try {
    checkType(formNode, AST_FORM);

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

          case AST_TAGGEDSTRING:
            symName = quoted(childString(n, 1));
            symTag = childName(n, 0);
            tagValid = true;
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
      if (numChildren(formNode) == 2) {
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
      for (LitCodeDict::Iter iter(nt->funDecls);
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
  } // try
  CATCH_APPLY_CONTEXT(formNode)
}


void astParseFormBodyElt(Environment &env, Production *prod,
                         ASTNode const *n, bool dupsOk)
{
  try {
    switch (n->type) {
      case AST_ACTION:
        astParseAction(env, prod, n, dupsOk);
        break;

      case AST_CONDITION:
        prod->conditions.conditions.append(
          astParseCondition(env, prod, n));
        break;

      case AST_TREECOMPARE:
        astParseTreeCompare(env, prod, n);
        break;

      case AST_FUNDECL: {
        // allow function declarations in form bodies because it's
        // convenient for nonterminals that only have one form..
        string name = childName(n, 0);
        if (!prod->left->funDecls.isMapped(name)) {
          prod->left->funDecls.add(
            name,                  // declared name
            childLitCode(n, 1));   // declaration body
        }
        else {
          astParseError(n, "duplicate function declaration");
        }
        break;
      }

      case AST_FUNCTION:
      case AST_FUNEXPR:
        astParseFunction(env, prod, n, dupsOk);
        break;

      END_TYPE_SWITCH(n);
    }
  }
  CATCH_APPLY_CONTEXT(n)
}


void astParseAction(Environment &env, Production *prod,
                    ASTNode const *act, bool dupsOk)
{
  checkType(act, AST_ACTION);

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
    new AttrAction(AttrLvalue(0 /*whichTree*/, 0 /*LHS*/, attrName),
                   expr));
}


ExprCondition *astParseCondition(Environment &env, Production *prod,
                                 ASTNode const *cond)
{
  checkType(cond, AST_CONDITION);

  AExprNode *expr = astParseExpr(env, prod, nthChild(cond, 0));

  return new ExprCondition(expr);
}


void astParseTreeCompare(Environment &env, Production *prod,
                         ASTNode const *node)
{
  checkType(node, AST_TREECOMPARE);

  if (prod->treeCompare != NULL) {
    astParseError(node, "duplicate treeCompare definition");
  }

  // get tree names to use
  string t1Name = childName(node, 0);
  string t2Name = childName(node, 1);

  // parse the expression with those names in context
  TreesContext trees(t1Name, t2Name);
  AExprNode *expr = astParseExpr(env, prod,
                                 nthChild(node, 2), trees);

  // put it into the production
  prod->treeCompare = expr;
}


void astParseFunction(Environment &env, Production *prod,
                      ASTNode const *func, bool dupsOk)
{
  try {
    xassert(func->type == AST_FUNCTION ||
            func->type == AST_FUNEXPR);

    string name = childName(func, 0);
    if (!prod->left->hasFunDecl(name)) {
      astParseError(nthChild(func, 0),
        stringc << "undeclared function: " << name);
    }
    if (prod->hasFunction(name)) {
      if (!dupsOk) {
        astParseError(nthChild(func, 0),
          stringc << "duplicate function implementation: " << name);
      }
      else {
        return;    // ignore duplicates
      }
    }

    prod->functions.add(name, childLitCode(func, 1));
  }
  CATCH_APPLY_CONTEXT(func)
}


// parse an expression AST (action or condition), in the context
// of the production where that expression will be eval'd
AExprNode *astParseExpr(Environment &env, Production *prod,
                        ASTNode const *node)
{
  TreesContext dummy;
  return astParseExpr(env, prod, node, dummy);
}

AExprNode *astParseExpr(Environment &env, Production *prod,
                        ASTNode const *node, TreesContext const &trees)
{
  switch (node->type) {
    case AST_INTEGER:
      return new AExprLiteral(nodeInt(node));

    case EXP_ATTRREF: {
      // which of two trees we refer to
      int whichTree;

      // names the RHS symbol we're referring to
      int whichRHS = 0;        // tag missing means LHS symbol

      // process AST children right to left
      int child = numChildren(node)-1;
      string attrName = childName(node, child--);

      if (child >= 0) {        // tag is present
        string tag = childName(node, child--);
        whichRHS = prod->findTag(tag);
        if (whichRHS == -1) {
          astParseError(node, "invalid tag");
        }
      }

      if (child >= 0) {        // tree specifier is present
        string treeName = childName(node, child--);
        whichTree = trees.lookupTreeName(treeName);
        if (whichTree == -1) {
          astParseError(node, "invalid tree specifier");
        }
      }
      else {                   // tree specifier not present
        if (!trees.dummyContext()) {
          // but one is required, because we're in a context
          // where there are multiple trees to refer to
          astParseError(node, "tree specifier required");
        }
        else {
          // the context only has one tree to name
          whichTree = 0;
        }
      }

      xassert(child == -1);        // otherwise my AST is bad..

      return new AExprAttrRef(AttrLvalue(whichTree, whichRHS, attrName));
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
        fn->args.append(astParseExpr(env, prod, iter.data(), trees));
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
        fn->args.append(astParseExpr(env, prod, iter.data(), trees));
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

  try {
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
      case TOK_DECL_BODY:
        *lvalp = new ASTStringLeaf(lexer.curFuncBody(), lexer.curLoc());
        break;

      default:
        *lvalp = NULL;
    }
  }
  catch (xBase &x) {
    // e.g. malformed fundecl
    cout << lexer.curLocStr() << ": " << x << endl;
    
    // optimistically try just skipping the bad token
    return yylex(lvalp, parseParam);
  }

  return code;
}


void my_yyerror(char const *message, void *parseParam)
{
  ParseParams *par = (ParseParams*)parseParam;
  cout << message << " at " << par->lexer.curLocStr() << endl;
}


// ---------------- external interface -------------------
bool isGramlexEmbed(int code);     // defined in gramlex.lex

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
    lexer = new GrammarLexer(isGramlexEmbed);
  }
  else {
    // file
    in = new ifstream(fname);
    if (!*in) {
      xsyserror("open", stringc << "error opening input file " << fname);
    }
    lexer = new GrammarLexer(isGramlexEmbed, fname, in.xfr());
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

    // then check grammar properties; throws exception
    // on failure
    traceProgress() << "beginning grammar analysis..\n";
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


// ----------------------- test code -----------------------
#ifdef TEST_GRAMPAR

#include "bflatten.h"     // BFlatten
#include <stdlib.h>       // system

int main(int argc, char **argv)
{
  traceAddSys("progress");
  TRACE_ARGS();

  bool printActions = true;
  bool printCode = false;

  // read the file
  Grammar g1;
  readGrammarFile(g1, argc>=2? argv[1] : NULL /*stdin*/);

  // and print the grammar
  char const g1Fname[] = "grammar.g1.tmp";
  traceProgress() << "printing initial grammar to " << g1Fname << "\n";
  {
    ofstream out(g1Fname);
    g1.printProductions(out, printActions, printCode);
  }

  // write it to a binary file
  char const binFname[] = "grammar.bin.tmp";
  traceProgress() << "writing initial grammar to " << binFname << "\n";
  {
    BFlatten flat(binFname, false /*reading*/);
    g1.xfer(flat);
  }

  // read it back
  traceProgress() << "reading grammar from " << binFname << "\n";
  Grammar g2;
  {
    BFlatten flat(binFname, true /*reading*/);
    g2.xfer(flat);
  }

  // print that too
  char const g2Fname[] = "grammar.g2.tmp";
  traceProgress() << "printing just-read grammar to " << g2Fname << "\n";
  {
    ofstream out(g2Fname);
    g2.printProductions(out, printActions, printCode);
  }

  // compare the two written files
  int result = system(stringc << "diff " << g1Fname << " " << g2Fname);
  if (result != 0) {
    cout << "the two ascii representations differ!!\n";
    return 4;
  }

  // remove the temp files
  if (!tracingSys("keep-tmp")) {
    remove(g1Fname);
    remove(g2Fname);
    remove(binFname);
  }

  cout << "successfully parsed, printed, wrote, and read a grammar!\n";
  return 0;
}

#endif // TEST_GRAMPAR
