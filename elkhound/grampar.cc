// grampar.cc
// additional C++ code for the grammar parser; in essence,
// build the grammar internal representation out of what
// the user supplies in a .gr file

#include "grampar.h"     // this module
#include "gramlex.h"     // GrammarLexer
#include "trace.h"       // tracing debug functions
#include "gramast.gen.h" // grammar AST nodes
#include "grammar.h"     // Grammar, Production, etc.
#include "owner.h"       // Owner
#include "syserr.h"      // xsyserror
#include "strutil.h"     // quoted
#include "grampar.tab.h" // token constant codes, union YYSTYPE

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


void Environment::addInherited(FormBodyElt const *fbe)
{
  // NOTE: must prepend here because the list is walked in order
  // when adding inherited items to productions, and later
  // additions must shadow earlier ones
  // UPDATE: changed impl of astParseFunction so appending is correct,
  // since that's required anyway to let decls precede definitions
  // (how did it even work before?)
  inherited.append(const_cast<FormBodyElt*>(fbe));
}


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
  constructMsg(LocString const &tok, char const *msg)
{
  if (tok.isValid()) {
    return stringc << "near " << tok
                   << ", at " << tok.toString() << ": " << msg;
  }
  else {
    return stringc << "(?loc): " << msg;
  }
}

XASTParse::XASTParse(LocString const &tok, char const *m)
  : xBase(constructMsg(tok, m)),
    failToken(tok),
    message(m)
{}


XASTParse::XASTParse(XASTParse const &obj)
  : xBase(obj),
    DMEMB(failToken),
    DMEMB(message)
{}

XASTParse::~XASTParse()
{}


// -------------------- AST parser support ---------------------
// fwd-decl of parsing fns
void astParseGrammar(Grammar &g, GrammarAST const *treeTop);
void astParseNonterm(Environment &env, TF_nonterminal const *nt,
                     Nonterminal *nonterm);
void astParseNonterminalBody(Environment &env, Nonterminal *nt,
                             ASTList<NTBodyElt> const &bodyList);
void astParseFormgroup(Environment &env, Nonterminal *nt,
                       ASTList<GroupElement> const &groupList);
void astParseGroupElement(Environment &env, Nonterminal *nt,
                          GroupElement const *grpElt);
void astParseForm(Environment &env, Nonterminal *nt,
                  GE_form const *formNode);
void astParseFormBodyElt(Environment &env, Production *prod,
                         FormBodyElt const *n, bool dupsOk);
void astParseAction(Environment &env, Production *prod,
                    FB_action const *act, bool dupsOk);
ExprCondition *astParseCondition(Environment &env, Production *prod,
                                 FB_condition const *cond);
void astParseTreeCompare(Environment &env, Production *prod,
                         FB_treeCompare const *tc);
void astParseFunction(Environment &env, Production *prod,
                      FB_funDefn const *func, bool dupsOk);
AExprNode *astParseExpr(Environment &env, Production *prod,
                        ExprAST const *node);
AExprNode *astParseExpr(Environment &env, Production *prod,
                        ExprAST const *node, TreesContext const &trees);
AExprNode *checkSpecial(Environment &env, Production *prod,
                        E_funCall const *call);


// really a static semantic error, more than a parse error..
void astParseError(LocString const &failToken, char const *msg)
{
  THROW(XASTParse(failToken, msg));
}

void astParseError(char const *msg)
{
  LocString ls;   // no location info
  THROW(XASTParse(ls, msg));
}


// to put as the catch block; so far it's kind of ad-hoc where
// I actually put 'try' blocks..
#define CATCH_APPLY_CONTEXT(tok)        \
  catch (XASTParse &x) {                \
    /* leave unchanged */               \
    throw x;                            \
  }                                     \
  catch (xBase &x) {                    \
    /* add context */                   \
    astParseError(tok, x.why());        \
    throw 0;     /* silence warning */  \
  }


// ---------------------- AST "parser" --------------------------
// map the grammar definition AST into a Grammar data structure
void astParseGrammar(Grammar &g, GrammarAST const *treeTop)
{
  // default, empty environment
  Environment env(g);

  // process each toplevel form
  FOREACH_ASTLIST(ToplevelForm, treeTop->forms, iter) {
    //try {
                                 
      ASTSWITCHC(ToplevelForm, iter.data()) {
        ASTCASEC(TF_terminals, terms) {
          // loop over the terminal declarations
          FOREACH_ASTLIST(TermDecl, terms->terms, termIter) {
            //try {
              TermDecl const &term = *(termIter.data());

              // process the terminal declaration
              int code = term.code;
              StringRef name = term.name;
              bool ok;
              if (term.alias[0]) {
                ok = env.g.declareToken(name, code, quoted(term.alias));
              }
              else {
                ok = env.g.declareToken(name, code, NULL /*alias*/);
              }

              if (!ok) {
                astParseError(term.name, "token already declared");
              }
            //} // try
            //CATCH_APPLY_CONTEXT(termIter.data())
          } // for(terminal)
        }

        ASTNEXTC(TF_treeNodeBase, tn) {
          g.treeNodeBaseClass = string(tn->baseClassName);
        }
        
        ASTNEXTC(TF_lit, lit) {
          // extract AST fields
          LocString const &tag = lit->lit->codeKindTag;
          LiteralCode code = lit->lit->codeBody;

          if (tag.equals("prologue")) {
            if (!g.semanticsPrologue.isNull()) {
              astParseError(tag, "prologue already defined");
            }
            g.semanticsPrologue = code;
          }

          else if (tag.equals("epilogue")) {
            if (!g.semanticsEpilogue.isNull()) {
              astParseError(tag, "epilogue already defined");
            }
            g.semanticsEpilogue = code;
          }

          else {
            astParseError(tag, stringc << "unknown litcode tag: " << tag);
          }
        }

        ASTNEXTC(TF_nonterminal, nt) {
          {
            // new environment since it can contain a grouping construct
            Environment newEnv(env);

            // parse it
            astParseNonterm(newEnv, nt, NULL /*means original decl*/);
          }

          // add this decl to our running list (in the original environment)
          env.nontermDecls.add(nt->name, const_cast<TF_nonterminal*>(nt));
        }

        ASTENDCASEC
      } // switch
    //} // try
    //CATCH_APPLY_CONTEXT(node)
  } // for(toplevel form)
}


void astParseNonterm(Environment &env, TF_nonterminal const *nt,
                     Nonterminal *nonterm)
{
  bool originalDecl = !nonterm;

  LocString const &name = nt->name;

  if (originalDecl) {
    // check for already declared; don't add to the list
    // yet, though, as a safeguard against the base class
    // list referring to itself
    if (env.nontermDecls.isMapped(name)) {
      astParseError(name, "nonterminal already declared");
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

  // for each base class..
  FOREACH_ASTLIST(LocString, nt->baseClasses, iter) {
    // get its name
    LocString const &baseClassName = *(iter.data());

    // get its AST node
    if (!env.nontermDecls.isMapped(baseClassName)) {
      astParseError(baseClassName, "undeclared base class");
    }
    TF_nonterminal const *base = env.nontermDecls.queryf(baseClassName);

    // map the name to a Nonterminal (part of a Grammar)
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
      THROW(XASTParse(baseClassName, x.why()));
    }
  }

  // parse the declaration itself; do this *after* re-parsing
  // base classes because base classes generally supply things
  // like fun{} implementations which must be put into the
  // environment before the productions are analyzed
  astParseNonterminalBody(env, nonterm, nt->elts);

  if (originalDecl) {
    // add this decl to our running list
    env.nontermDecls.add(name, const_cast<TF_nonterminal*>(nt));
  }
}


#if 0       // this has been reorganized
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
          env.addInherited(node);
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
#endif // 0


void astParseNonterminalBody(Environment &env, Nonterminal *nt,
                             ASTList<NTBodyElt> const &bodyList)
{
  FOREACH_ASTLIST(NTBodyElt, bodyList, iter) {
    ASTSWITCHC(NTBodyElt, iter.data()) {
      ASTCASEC(NT_attr, attr) {
        nt->attributes.append(new string(attr->name));
      }

      ASTNEXTC(NT_decl, decl) {
        nt->declarations.append(new LocString(decl->declBody));
      }

      ASTNEXTC(NT_elt, grpElt) {
        astParseGroupElement(env, nt, grpElt->elt);
      }

      ASTNEXTC(NT_lit, llit) {
        // extract AST fields
        LiteralCodeAST *lit = llit->lit;
        LocString const &tag = lit->codeKindTag;
        LiteralCode const &code = lit->codeBody;

        // if it's an LC_modifier, get the name of the function it modifies
        LocString name;          // assume it's not a modifier
        LC_modifier const *mod = lit->ifLC_modifierC();
        if (mod) {
          name = mod->funcToModify;
          trace("semant") << "[" << nt->name << "] LC_modifier of " << name << endl;
        }

        // look at the tag to figure out what the code means
        if (tag.equals("disamb")) {
          if (name.isNull()) {
            astParseError(tag, "disamb must have a name");
          }
          if (!nt->funDecls.isMapped(name)) {
            astParseError(name, "undeclared function");
          }
          nt->disambFuns.add(name, new LiteralCode(code));
        }

        else if (tag.equals("prefix")) {
          if (name.isNull()) {
            astParseError(tag, "prefix must have a name");
          }
          if (!nt->funDecls.isMapped(name)) {
            astParseError(name, "undeclared function");
          }
          nt->funPrefixes.add(name, new LiteralCode(code));
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
            astParseError(tag, "destructor already defined");
          }
          nt->destructor = code;
        }

        else {
          astParseError(tag, stringc << "unknown litcode tag: " << tag);
        }
      }

      ASTENDCASEC
    } // switch
  } // foreach
}

void astParseFormgroup(Environment &env, Nonterminal *nt,
                       ASTList<GroupElement> const &groupList)
{
  Environment newEnv(env);     // new environment
  FOREACH_ASTLIST(GroupElement, groupList, iter) {
    astParseGroupElement(newEnv, nt, iter.data());
  }
}

void astParseGroupElement(Environment &env, Nonterminal *nt,
                          GroupElement const *grpElt)
{
  ASTSWITCHC(GroupElement, grpElt) {
    ASTCASEC(GE_form, form) {
      astParseForm(env, nt, form);
    }

    ASTNEXTC(GE_formGroup, formGrp) {
      astParseFormgroup(env, nt, formGrp->elts);
    }

    ASTNEXTC(GE_fbe, elt) {
      FB_funDecl const *funDecl = elt->e->ifFB_funDeclC();
      FB_dataDecl const *dataDecl = elt->e->ifFB_dataDeclC();
      if (funDecl) {
        // we process declarations immediately, since they're associated
        // with the nonterminal (as opposed to the production); in fact
        // it's somewhat odd to call a funDecl a formBodyElt, but I did
        // that in the previous version so I'll keep it for now

        // even more suggestive that there's a design flaw: I have to
        // synthesize a fake production, and rely on knowledge that
        // the function I'm calling only looks at prod->left ...
        Production dummy(nt, "dummy tag");
        astParseFormBodyElt(env, &dummy, funDecl, false /*dupsOk*/);
        
        // put it in here too?  I'm kinda flailing at this point..
        env.addInherited(elt->e);
      }
      else if (dataDecl) {
        // here we handle it directly, and don't have a case for handling
        // it inside astParseFormBodyElt, which is at least closer to the
        // right design...
        nt->declarations.append(new LocString(dataDecl->declBody));
      }
      else {
        // just grab a pointer; will parse for real when a production
        // uses this inherited action
        env.addInherited(elt->e);
      }
    }

    ASTENDCASEC
  }
}


void astParseForm(Environment &env, Nonterminal *nt,
                  GE_form const *formNode)
{
  //try {
    // every alternative separated by "|" requires the entire treatment
    FOREACH_ASTLIST(RHS, formNode->rhsides, altIter) {
      RHS const *rhsListNode = altIter.data();

      // build a production; use 'this' as the tag for LHS elements
      Production *prod = new Production(nt, "this");

      // first, deal with RHS elements
      FOREACH_ASTLIST(RHSElt, rhsListNode->rhs, iter) {
        RHSElt const *n = iter.data();
        LocString symName;
        LocString symTag;
        bool tagValid = false;
        bool isString = false;

        ASTSWITCHC(RHSElt, n) {
          ASTCASEC(RH_name, name) {
            symName = name->name;
          }

          ASTNEXTC(RH_taggedName, tname) {
            symName = tname->name;
            symTag = tname->tag;
            tagValid = true;
          }

          ASTNEXTC(RH_string, s) {
            symName = s->str;
            isString = true;
          }

          ASTNEXTC(RH_taggedString, ts) {
            symName = ts->str;
            symTag = ts->tag;
            tagValid = true;
            isString = true;
          }

          ASTENDCASEC
        }

        // quote the string if it was quoted in syntax
        string quotSymName(symName);
        if (isString) {
          quotSymName = quoted(symName);
        }

        // see which (if either) thing this name already is
        Terminal *term = env.g.findTerminal(quotSymName);
        Nonterminal *nonterm = env.g.findNonterminal(quotSymName);
        xassert(!( term && nonterm ));     // better not be both!

        // some syntax rules
        if (isString  &&  !term) {
          astParseError(symName, "terminals must be declared");
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
      // update: no we don't -- GrammarAnalysis takes care of it (and
      // complains if we do)
      //prod->finished();

      // deal with formBody (we evaluate it once for each alternative form):
      // iterate over form body elements
      FOREACH_ASTLIST(FormBodyElt, formNode->elts, iter) {
        astParseFormBodyElt(env, prod, iter.data(), false /*dupsOk*/);
      }

      // grab stuff inherited from the environment
      SFOREACH_OBJLIST(FormBodyElt, env.inherited, iter) {
        astParseFormBodyElt(env, prod, iter.data(), true /*dupsOk*/);
      }

      // make sure all attributes have actions
      FOREACH_OBJLIST(string, nt->attributes, attrIter) {
        char const *attr = attrIter.data()->pcharc();
        if (!prod->actions.getAttrActionFor(attr)) {
          astParseError(
            stringc << "rule " << prod->toString()
                    << " has no action for `" << attr << "'");
        }
      }

      // make sure all fundecls have implementations
      for (LitCodeDict::Iter iter(nt->funDecls);
           !iter.isDone(); iter.next()) {
        char const *name = iter.key();
        if (!prod->hasFunction(name)) {
          astParseError(
            stringc << "rule " << prod->toString()
                    << " has no implementation for `" << name << "'");
        }
      }

      // add production to grammar
      env.g.addProduction(prod);

    } // end of for-each alternative
  //} // try
  //CATCH_APPLY_CONTEXT(formNode)
}


void astParseFormBodyElt(Environment &env, Production *prod,
                         FormBodyElt const *n, bool dupsOk)
{
  //try {
    ASTSWITCHC(FormBodyElt, n) {
      ASTCASEC(FB_action, act) {
        astParseAction(env, prod, act, dupsOk);
      }

      ASTNEXTC(FB_condition, cond) {
        prod->conditions.conditions.append(
          astParseCondition(env, prod, cond));
      }
       
      ASTNEXTC(FB_treeCompare, tc) {
        astParseTreeCompare(env, prod, tc);
      }
       
      ASTNEXTC(FB_funDecl, fd) {
        // allow function declarations in form bodies because it's
        // convenient for nonterminals that only have one form..
        LocString name = fd->declName;
        trace("semant") << "[" << prod->left->name << "] funDecl " << name << endl;
        if (!prod->left->funDecls.isMapped(name)) {
          prod->left->funDecls.add(
            name,                             // declared name
            new LiteralCode(fd->declBody));   // declaration body
        }
        else {
          // the fundecl design is rather broken, and I end up multiply
          // declaring things even when the .gr file didn't ..
          //astParseError(name, "duplicate function declaration");
        }
      }

      ASTNEXTC(FB_funDefn, fd) {
        astParseFunction(env, prod, fd, dupsOk);
      }
      
      ASTNEXTC(FB_dataDecl, dd) {
        // old code didn't have a case here ... ?
        astParseError(dd->declBody, "data decl not allowed here (?)");
      }

      ASTENDCASEC
    }
  //}
  //CATCH_APPLY_CONTEXT(n)
}


void astParseAction(Environment &env, Production *prod,
                    FB_action const *act, bool dupsOk)
{
  LocString attrName = act->name;
  if (!prod->left->hasAttribute(attrName)) {
    astParseError(attrName, stringc << "undeclared attribute: " << attrName);
  }
  if (prod->actions.getAttrActionFor(attrName)) {
    // there is already an action for this attribute
    if (dupsOk) {
      return;     // fine; just ignore it
    }
    else {
      astParseError(attrName, stringc << "duplicate action for " << attrName);
    }
  }

  AExprNode *expr = astParseExpr(env, prod, act->expr);

  prod->actions.actions.append(
    new AttrAction(AttrLvalue(0 /*whichTree*/, 0 /*LHS*/, AttrName(attrName)),
                   expr));
}


ExprCondition *astParseCondition(Environment &env, Production *prod,
                                 FB_condition const *cond)
{
  AExprNode *expr = astParseExpr(env, prod, cond->condExpr);

  return new ExprCondition(expr);
}


void astParseTreeCompare(Environment &env, Production *prod,
                         FB_treeCompare const *tc)
{
  if (prod->treeCompare != NULL) {
    astParseError(tc->leftName, "duplicate treeCompare definition");
  }

  // get tree names to use
  LocString t1Name = tc->leftName;
  LocString t2Name = tc->rightName;

  // parse the expression with those names in context
  TreesContext trees(t1Name, t2Name);
  AExprNode *expr = astParseExpr(env, prod,
                                 tc->decideExpr, trees);

  // put it into the production
  prod->treeCompare = expr;
}


void astParseFunction(Environment &env, Production *prod,
                      FB_funDefn const *func, bool dupsOk)
{
  //try {
    //xassert(func->type == AST_FUNCTION ||
    //        func->type == AST_FUNEXPR);

    LocString name = func->name;
    trace("semant") << "[" << prod->left->name << "] funDefn " << name << endl;

    if (!prod->left->hasFunDecl(name)) {
      astParseError(name, stringc << "undeclared function: " << name);
    }
    if (prod->hasFunction(name)) {
      if (!dupsOk) {
        astParseError(name, stringc << "duplicate function implementation: " << name);
      }
      else {
        // duplicates replace previous versions: this is the mechanism
        // of overriding inherited definitions
        prod->functions.deleteAt(name);
      }
    }

    prod->functions.add(name, new LiteralCode(func->defnBody));
  //}
  //CATCH_APPLY_CONTEXT(func)
}


// parse an expression AST (action or condition), in the context
// of the production where that expression will be eval'd
AExprNode *astParseExpr(Environment &env, Production *prod,
                        ExprAST const *node)
{
  TreesContext dummy;
  return astParseExpr(env, prod, node, dummy);
}

AExprNode *astParseExpr(Environment &env, Production *prod,
                        ExprAST const *node, TreesContext const &trees)
{
  ASTSWITCHC(ExprAST, node) {
    ASTCASEC(E_intLit, i) {
      return new AExprLiteral(i->val);
    }

    ASTNEXTC(E_attrRef, ar) {
      if (!trees.dummyContext()) {
        // a tree specifier is required, because we're in a context
        // where there are multiple trees to refer to
        astParseError(ar->tag, "tree specifier required");
      }

      int whichRHS = prod->findTag(ar->tag);      // which RHS elt
      if (whichRHS == -1) {
        astParseError(ar->tag, "invalid tag");
      }

      return new AExprAttrRef(AttrLvalue(0 /*tree*/, whichRHS, AttrName(ar->attr)));
    }

    ASTNEXTC(E_treeAttrRef, ar) {
      int whichRHS = prod->findTag(ar->tag);      // which RHS elt
      if (whichRHS == -1) {
        astParseError(ar->tag, "invalid tag");
      }

      int whichTree = trees.lookupTreeName(ar->tree);
      if (whichTree == -1) {
        astParseError(ar->tree, "invalid tree specifier");
      }

      return new AExprAttrRef(AttrLvalue(whichTree, whichRHS, AttrName(ar->attr)));
    }

    ASTNEXTC(E_funCall, call) {
      // handle any eval-at-grammar-parse functions
      AExprNode *special = checkSpecial(env, prod, call);
      if (special) {
        return special;
      }

      // function name
      AExprFunc *fn = new AExprFunc(call->funcName);

      // and arguments
      FOREACH_ASTLIST(ExprAST, call->args, iter) {
        fn->args.append(astParseExpr(env, prod, iter.data(), trees));
      }

      return fn;
    }

    ASTNEXTC(E_unary, u) {
      char const *funcName = AExprFunc::lookupFunc(u->op);
      if (!funcName) {
        astParseError(stringc << "unknown function code " << u->op);
      }

      AExprFunc *fn = new AExprFunc(funcName);
      fn->args.append(astParseExpr(env, prod, u->exp, trees));

      return fn;
    }

    ASTNEXTC(E_binary, b) {
      char const *funcName = AExprFunc::lookupFunc(b->op);
      if (!funcName) {
        astParseError(stringc << "unknown function code " << b->op);
      }

      AExprFunc *fn = new AExprFunc(funcName);
      fn->args.append(astParseExpr(env, prod, b->left,  trees));
      fn->args.append(astParseExpr(env, prod, b->right, trees));

      return fn;
    }

    ASTNEXTC(E_cond, c) {
      AExprFunc *fn = new AExprFunc("if");
      fn->args.append(astParseExpr(env, prod, c->test,    trees));
      fn->args.append(astParseExpr(env, prod, c->thenExp, trees));
      fn->args.append(astParseExpr(env, prod, c->elseExp, trees));

      return fn;
    }

    ASTENDCASEC
  }

  xfailure("bad type code");
  return NULL;    // silence warning
}


// 'node' is a function node; if the function is one we recognize
// as special, handle it; otherwise return NULL
AExprNode *checkSpecial(Environment &env, Production *prod,
                        E_funCall const *call)
{
  // I'm removing this entirely because it's a little difficult to
  // translate to my new AST format (the argument has to be a literal
  // int, but my AST allows any expr), and I have no uses of it
  // (currently) in cc.gr
  #if 0
  // for now, this is only special
  if (call->fundName.equals("sequence")) {
    // convenience
    int &seq = env.sequenceVal;
    ASTList<ExprAST> const &args = call->args;

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
  #endif // 0

  // nothing else is special
  return NULL;
}


// ----------------------- parser support ---------------------
// Bison parser calls this to get a token
int grampar_yylex(union YYSTYPE *lvalp, void *parseParam)
{
  ParseParams *par = (ParseParams*)parseParam;
  GrammarLexer &lexer = par->lexer;

  int code = lexer.yylexInc();

  try {
    // yield semantic values for some things
    // note that the yielded semantic value must be consistent with
    // what is declared for these token types in grampar.y
    switch (code) {
      case TOK_INTEGER:
        lvalp->num = lexer.integerLiteral;
        break;

      case TOK_STRING:
        lvalp->str = new LocString(lexer.curLoc(), lexer.stringLiteral);
        break;

      case TOK_NAME:
        lvalp->str = new LocString(lexer.curLoc(), lexer.curToken());
        break;

      case TOK_FUN_BODY:
        lvalp->str = new LocString(lexer.curLoc(), lexer.curFuncBody());
        break;

      case TOK_DECL_BODY:
        lvalp->str = new LocString(lexer.curLoc(), lexer.curDeclBody());
        break;

      case TOK_FUNDECL_BODY: {
        lvalp->funDecl = new FB_funDecl(new LocString(lexer.curLoc(), lexer.curDeclName()),
                                        new LocString(lexer.curLoc(), lexer.curDeclBody()));

        #if 0
        // grab the declaration body and put it into a leaf
        ASTNode *declBody =
          new ASTStringLeaf(lexer.curDeclBody(), lexer.curLoc());

        // grab the declared function name and put it into another leaf
        ASTNode *declName =
          new ASTNameLeaf(lexer.curDeclName(), lexer.curLoc());

        // wrap them into another ast node
        *lvalp = AST2(AST_FUNDECL, declName, declBody);
        #endif // 0

        break;
      }

      default:
        lvalp->str = NULL;        // any attempt to use will segfault
    }
  }
  catch (xBase &x) {
    // e.g. malformed fundecl
    cout << lexer.curLocStr() << ": " << x << endl;
    
    // optimistically try just skipping the bad token
    return grampar_yylex(lvalp, parseParam);
  }

  return code;
}


void grampar_yyerror(char const *message, void *parseParam)
{
  ParseParams *par = (ParseParams*)parseParam;
  cout << message << " at " << par->lexer.curLocStr() << endl;
}


// ---------------- external interface -------------------
bool isGramlexEmbed(int code);     // defined in gramlex.lex

StringTable stringTable;

void readGrammarFile(Grammar &g, char const *fname)
{
  // no idea what this is ..
  //ASTNode::typeToString = astTypeToString;

  if (tracingSys("yydebug")) {
    yydebug = true;
  }

  Owner<GrammarLexer> lexer;
  Owner<ifstream> in;
  if (fname == NULL) {
    // stdin
    lexer = new GrammarLexer(isGramlexEmbed, stringTable);
  }
  else {
    // file
    in = new ifstream(fname);
    if (!*in) {
      xsyserror("open", stringc << "error opening input file " << fname);
    }
    lexer = new GrammarLexer(isGramlexEmbed, stringTable, fname, in.xfr());
  }

  ParseParams params(*lexer);

  traceProgress() << "parsing grammar source..\n";
  int retval = yyparse(&params);
  if (retval == 0) {
    // make sure the tree gets deleted
    Owner<GrammarAST> treeTop(params.treeTop);

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
    
    // hmm.. I'd like to restore this functionality...
    //if (ASTNode::nodeCount > 0) {
    //  cout << "leaked " << ASTNode::nodeCount << " AST nodes\n";
    //} 
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
