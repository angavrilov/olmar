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
    nontermDecls()
{}

Environment::Environment(Environment &prev)
  : g(prev.g),
    prevEnv(&prev),
    nontermDecls(prev.nontermDecls)
{}

Environment::~Environment()
{}


// -------------------- XASTParse --------------------
STATICDEF string XASTParse::
  constructMsg(LocString const &tok, char const *msg)
{
  if (tok.validLoc()) {
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
void astParseTerminals(Environment &env, Terminals const &terms);
void astParseDDM(Environment &env, ConflictHandlers &ddm,
                 ASTList<SpecFunc> const &funcs, bool mergeOk);
void astParseNonterm(Environment &env, NontermDecl const *nt);
void astParseProduction(Environment &env, Nonterminal *nonterm,
                        ProdDecl const *prod);


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

  // stash verbatim code in the grammar
  g.verbatim = treeTop->verbatimCode;

  // parse parameter
  g.parseParamPresent = treeTop->param->present;
  g.parseParamType = treeTop->param->type;
  g.parseParamName = treeTop->param->name;

  // process token declarations
  astParseTerminals(env, *(treeTop->terms));

  // process nonterminals
  FOREACH_ASTLIST(NontermDecl, treeTop->nonterms, iter) {
    NontermDecl const *nt = iter.data();

    {
      // new environment since it can contain a grouping construct
      // (at this very moment it actually can't because there is no syntax..)
      Environment newEnv(env);

      // parse it
      astParseNonterm(newEnv, nt);
    }

    // add this decl to our running list (in the original environment)
    env.nontermDecls.add(nt->name, const_cast<NontermDecl*>(nt));
  }
}


Terminal *astParseToken(Environment &env, LocString const &name)
{
  Terminal *t = env.g.findTerminal(name);
  if (!t) {
    astParseError(name, "undeclared token");
  }
  return t;
}


void astParseTerminals(Environment &env, Terminals const &terms)
{
  // basic declarations
  {
    FOREACH_ASTLIST(TermDecl, terms.decls, iter) {
      TermDecl const &term = *(iter.data());

      // process the terminal declaration
      int code = term.code;
      StringRef name = term.name;
      trace("grampar") << "token: code=" << code
                       << ", name=" << name << endl;

      if (!env.g.declareToken(term.name, code, term.alias)) {
        astParseError(term.name, "token already declared");
      }
    }
  }

  // type annotations
  {                  
    FOREACH_ASTLIST(TermType, terms.types, iter) {
      TermType const &type = *(iter.data());
      trace("grampar") << "token type: name=" << type.name
                       << ", type=" << type.type << endl;

      // look up the name
      Terminal *t = astParseToken(env, type.name);
      if (t->type) {
        astParseError(type.name, "this token already has a type");
      }

      // annotate with declared type
      t->type = type.type;

      // parse the dup/del/merge spec
      astParseDDM(env, t->ddm, type.funcs, false /*mergeOk*/);
    }
  }

  // precedence specifications
  {
    int level = 0;
    FOREACH_ASTLIST(PrecSpec, terms.prec, iter) {
      PrecSpec const &spec = *(iter.data());
      level++;

      FOREACH_ASTLIST(LocString, spec.tokens, tokIter) {
        LocString const &tokName = *(tokIter.data());
        trace("grampar") << "prec: " << toString(spec.kind)
                         << " " << tokName;

        // look up the token
        Terminal *t = astParseToken(env, tokName);
        if (t->precedence) {
          astParseError(tokName, 
            stringc << tokName << " already has a specified precedence");
        }

        // apply spec
        t->precedence = level;
        t->associativity = spec.kind;
      }
    }
  }
}


void astParseDDM(Environment &env, ConflictHandlers &ddm,
                 ASTList<SpecFunc> const &funcs, bool mergeOk)
{
  FOREACH_ASTLIST(SpecFunc, funcs, iter) {
    SpecFunc const &func = *(iter.data());
    int numFormals = func.formals.count();

    if (func.name.equals("dup")) {
      if (numFormals != 1) {
        astParseError(func.name, "'dup' function must have one formal parameter");
      }
      ddm.dupParam = func.nthFormal(0);
      ddm.dupCode = func.code;
    }

    else if (func.name.equals("del")) {
      if (numFormals == 0) {
        // not specified is ok, since it means the 'del' function
        // doesn't use its parameter
        ddm.delParam = NULL;
      }
      else if (numFormals == 1) {
        ddm.delParam = func.nthFormal(0);
      }
      else {
        astParseError(func.name, "'del' function must have either zero or one formal parameters");
      }
      ddm.delCode = func.code;
    }

    else if (func.name.equals("merge")) {
      if (mergeOk) {
        if (numFormals != 2) {
          astParseError(func.name, "'merge' function must have two formal parameters");
        }
        ddm.mergeParam1 = func.nthFormal(0);
        ddm.mergeParam2 = func.nthFormal(1);
        ddm.mergeCode = func.code;
      }
      else {
        astParseError(func.name, "'merge' can only be applied to nonterminals");
      }
    }

    else if (func.name.equals("keep")) {
      if (mergeOk) {
        if (numFormals != 1) {
          astParseError(func.name, "'keep' function must have one formal parameter");
        }
        ddm.keepParam = func.nthFormal(0);
        ddm.keepCode = func.code;
      }
      else {
        astParseError(func.name, "'keep' can only be applied to nonterminals");
      }
    }

    else {
      astParseError(func.name,
        stringc << "unrecognized spec function \"" << func.name << "\"");
    }
  }
}


void astParseNonterm(Environment &env, NontermDecl const *nt)
{
  LocString const &name = nt->name;

  // check for already declared
  if (env.nontermDecls.isMapped(name)) {
    astParseError(name, "nonterminal already declared");
  }

  // make the Grammar object to represent the new nonterminal
  Nonterminal *nonterm = env.g.getOrMakeNonterminal(name);
  nonterm->type = nt->type;

  // iterate over the productions
  FOREACH_ASTLIST(ProdDecl, nt->productions, iter) {
    astParseProduction(env, nonterm, iter.data());
  }

  // parse dup/del/merge
  astParseDDM(env, nonterm->ddm, nt->funcs, true /*mergeOk*/);
}


void astParseProduction(Environment &env, Nonterminal *nonterm,
                        ProdDecl const *prodDecl)
{
  // build a production; use 'this' as the tag for LHS elements
  Production *prod = new Production(nonterm, "this");

  // put the code into it
  prod->action = prodDecl->actionCode;

  // deal with RHS elements
  FOREACH_ASTLIST(RHSElt, prodDecl->rhs, iter) {
    RHSElt const *n = iter.data();
    LocString symName;
    LocString symTag;
    bool isString = false;
    bool isPrec = false;

    // pull varius info out of the AST node
    ASTSWITCHC(RHSElt, n) {
      ASTCASEC(RH_name, tname) {
        symName = tname->name;
        symTag = tname->tag;
      }

      ASTNEXTC(RH_string, ts) {
        symName = ts->str;
        symTag = ts->tag;
        isString = true;
      }

      ASTNEXTC(RH_prec, p) {
        // apply the specified precedence
        prod->precedence = astParseToken(env, p->tokName)->precedence;

        // and require that this is the last RHS element
        iter.adv();
        if (!iter.isDone()) {
          astParseError(p->tokName,
            "precedence spec must be last thing in a production "
            "(before the action code)");
        }
        isPrec = true;
      }

      ASTENDCASEC
    }

    if (isPrec) {
      break;     // last element anyway
    }

    // see which (if either) thing this name already is
    Terminal *term = env.g.findTerminal(symName);
    Nonterminal *nonterm = env.g.findNonterminal(symName);
    xassert(!( term && nonterm ));     // better not be both!

    // a syntax rule
    if (isString  &&  !term) {
      astParseError(symName, "terminals must be declared");
    }

    // whenever we see a terminal, copy its precedence spec to
    // the production; thus, the last symbol appearing in the
    // production will be the one that gives the precedence
    if (term) {
      prod->precedence = term->precedence;
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

    if (s->isEmptyString) {
      // "empty" is a syntactic convenience; it doesn't get
      // added to the production
    }
    else {
      // add it to the production
      prod->append(s, symTag);
    }
  }

  // after constructing the production we need to do this
  // update: no we don't -- GrammarAnalysis takes care of it (and
  // complains if we do)
  //prod->finished();

  // add production to grammar
  env.g.addProduction(prod);
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

      case TOK_LIT_CODE:
        lvalp->str = new LocString(lexer.curLoc(), lexer.curFuncBody());
        break;

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

void readGrammarFile(Grammar &g, char const *fname)
{
  if (tracingSys("yydebug")) {
    yydebug = true;
  }

  Owner<GrammarLexer> lexer;
  Owner<ifstream> in;
  if (fname == NULL) {
    // stdin
    lexer = new GrammarLexer(isGramlexEmbed, grammarStringTable);
  }
  else {
    // file
    in = new ifstream(fname);
    if (!*in) {
      xsyserror("open", stringc << "error opening input file " << fname);
    }
    lexer = new GrammarLexer(isGramlexEmbed, grammarStringTable, fname, in.xfr());
  }

  ParseParams params(*lexer);

  traceProgress() << "parsing grammar source..\n";
  int retval = grampar_yyparse(&params);
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
  if (argc < 2) {
    cout << "usage: " << argv[0] << " [-tr flags] filename.gr\n";
    cout << "  interesting trace flags:\n";
    cout << "    keep-tmp      do not delete the temporary files\n";
    //cout << "    cat-grammar   print the ascii rep to the screen\n";
    return 0;
  }

  traceAddSys("progress");
  TRACE_ARGS();

  bool printCode = true;

  // read the file
  Grammar g1;
  readGrammarFile(g1, argv[1]);

  // and print the grammar
  char const g1Fname[] = "grammar.g1.tmp";
  traceProgress() << "printing initial grammar to " << g1Fname << "\n";
  {
    ofstream out(g1Fname);
    g1.printSymbolTypes(out);
    g1.printProductions(out, printCode);
  }

  //if (tracingSys("cat-grammar")) {
    system("cat grammar.g1.tmp");
  //}

  // before using 'xfer' we have to tell it about the string table
  flattenStrTable = &grammarStringTable;

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
    g2.printSymbolTypes(out);
    g2.printProductions(out, printCode);
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
