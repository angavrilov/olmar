// grammar.cc
// code for grammar.h

#include "grammar.h"   // this module
#include "bit2d.h"     // Bit2d
#include "syserr.h"    // xsyserror
#include "strtokp.h"   // StrtokParse

#include <stdarg.h>    // variable-args stuff
#include <iostream.h>  // cout, etc.
#include <stdio.h>     // FILE, etc.
#include <ctype.h>     // isupper


// print a variable value
#define PVAL(var) cout << " " << #var "=" << var;


// ---------------------- Symbol --------------------
Symbol::~Symbol()
{}


void Symbol::print() const
{
  cout << name << ":";
  PVAL(isTerminal);
}


void printSymbols(ObjList<Symbol> const &list)
{
  for (ObjListIter<Symbol> iter(list);
       !iter.isDone(); iter.adv()) {
    cout << "  ";
    iter.data()->print();
    cout << endl;
  }
}


Terminal const &Symbol::asTerminalC() const
{
  xassert(isTerminal);
  return (Terminal&)(*this);
}

Nonterminal const &Symbol::asNonterminalC() const
{
  xassert(!isTerminal);
  return (Nonterminal&)(*this);
}


// -------------------- Terminal ------------------------
void Terminal::print() const
{
  cout << "[" << termIndex << "] ";
  Symbol::print();
}


// ----------------- Nonterminal ------------------------
Nonterminal::Nonterminal(char const *name)
  : Symbol(name, false /*terminal*/),
    ntIndex(-1),
    cyclic(false)
{}

Nonterminal::~Nonterminal()
{}


void printTerminalSet(TerminalList const &list)
{
  cout << "{";
  int ct = 0;
  for (TerminalListIter term(list); !term.isDone(); term.adv(), ct++) {
    if (ct > 0) {
      cout << ", ";
    }
    cout << term.data()->name;
  }
  cout << "}";
}


void Nonterminal::print() const
{
  cout << "[" << ntIndex << "] ";
  Symbol::print();

  // cyclic?           
  if (cyclic) {
    cout << " (cyclic!)";
  }

  // first
  cout << " first=";
  printTerminalSet(first);

  // follow
  cout << " follow=";
  printTerminalSet(follow);
}


// -------------------- Production -------------------------
Production::~Production()
{}


void Production::print() const
{
  cout << left->name << " ->";

  for (SymbolListIter iter(right); !iter.isDone(); iter.adv()) {
    cout << " " << iter.data()->name;
  }
}


void DottedProduction::print() const
{
  cout << prod->left->name << " ->";

  int position = 0;
  for (SymbolListIter iter(prod->right);
       !iter.isDone(); iter.adv(), position++) {
    if (position == dot) {
      cout << " .";
    }
    cout << " " << iter.data()->name;
  }
  if (position == dot) {
    cout << " .";
  }
}


void Production::append(Symbol *sym)
{
  right.append(sym);
}



// ------------------ Grammar -----------------
Grammar::Grammar()
  : derivable(NULL),
    indexedNonTerms(NULL),
    indexedTerms(NULL),
    initialized(false),
    startSymbol(NULL),
    emptyString("empty"),
    cyclic(false)
{}


Grammar::~Grammar()
{
  if (indexedNonTerms != NULL) {
    delete indexedNonTerms;
  }

  if (indexedTerms != NULL) {
    delete indexedTerms;
  }

  if (derivable != NULL) {
    delete derivable;
  }
}


int Grammar::numTerminals() const
{
  return terminals.count();
}

int Grammar::numNonterminals() const
{                                
  // everywhere, we regard emptyString as a nonterminal
  return nonterminals.count() + 1;
}


void Grammar::printProductions() const
{                              
  if (cyclic) {
    cout << "(cyclic!) ";
  }
  cout << "Grammar productions:\n";
  for (ObjListIter<Production> iter(productions);
       !iter.isDone(); iter.adv()) {
    cout << "  ";
    iter.data()->print();
    cout << endl;
  }
}


void Grammar::addProduction(Nonterminal *lhs, Symbol *firstRhs, ...)
{
  va_list argptr;                   // state for working through args
  Symbol *arg;
  va_start(argptr, firstRhs);       // initialize 'argptr'

  Production *prod = new Production(lhs);
  prod->right.append(firstRhs);
  for(;;) {
    arg = va_arg(argptr, Symbol*);  // get next argument
    if (arg == NULL) {
      break;    // end of list
    }

    prod->right.append(arg);
  }

  addProduction(prod);
}


void Grammar::addProduction(Production *prod)
{
  // if the production doesn't have any RHS symbols, let's
  // support that as syntax for deriving emptyString, by
  // adding that explicitly here
  if (prod->right.count() == 0) {
    prod->append(&emptyString);
  }

  productions.append(prod);
  
  // if the start symbol isn't defined yet, we can here
  // implement the convention that the LHS of the first
  // production is the start symbol
  if (startSymbol == NULL) {
    startSymbol = prod->left;
  }
}


bool Grammar::addDerivable(Nonterminal const *left, Nonterminal const *right)
{
  return addDerivable(left->ntIndex, right->ntIndex);
}

bool Grammar::addDerivable(int left, int right)
{
  // Almost as an aside, I'd like to track cyclicity in grammars.
  // It's always true that N ->* N, because 0 steps are allowed.
  // A grammar is cyclic if N ->+ N, i.e. it derives itself in
  // 1 or more steps.
  //
  // We can detect that fairly easily by tracking calls to
  // this fn with left==right.  Since N ->* N in 0 steps is
  // recorded during init (and *not* by calling this fn), the
  // only calls to this with left==right will be when the
  // derivability code detects a nonzero-length path.

  if (left==right) {
    Nonterminal *NT = indexedNonTerms[left];    // ==right
    if (!NT->cyclic) {
      cout << "discovered that " << NT->name << " ->+ "
           << NT->name << " (i.e. is cyclic)\n";
      NT->cyclic = true;
      cyclic = true;     // for grammar as a whole

      // Even though we didn't know this already, it doesn't
      // constitute a change in the ->* relation (which is what the
      // derivability code cares about), so we do *not* report a
      // change for the cyclicty detection.
    }
  }

  // we only made a change, and hence should return true,
  // if there was a 0 here before
  return 0 == derivable->testAndSet(point(left, right));
}


bool Grammar::canDerive(Nonterminal const *left, Nonterminal const *right) const
{
  return canDerive(left->ntIndex, right->ntIndex);
}

bool Grammar::canDerive(int left, int right) const
{
  return 1 == derivable->get(point(left, right));
}


void Grammar::initDerivableRelation()
{
  // two-dimensional matrix to represent token derivabilities
  int numNonterms = numNonterminals();
  derivable = new Bit2d(point(numNonterms, numNonterms));

  // initialize it
  derivable->setall(0);
  loopi(numNonterms) {
    derivable->set(point(i,i));
      // every nonterminal can derive itself in 0 or more steps
      // (specifically, in 0 steps, at least)
      //
      // NOTE: we do *not* call addDerivable because that would
      // mess up the cyclicity detection logic
  }
}


bool Grammar::canDeriveEmpty(Nonterminal const *nonterm) const
{
  return canDerive(nonterm, &emptyString);
}


bool Grammar::firstIncludes(Nonterminal const *NT, Terminal const *term) const
{
  for (TerminalListIter iter(NT->first); !iter.isDone(); iter.adv()) {
    if (iter.data() == term) {
      return true;
    }
  }
  return false;
}

bool Grammar::addFirst(Nonterminal *NT, Terminal const *term)
{
  if (firstIncludes(NT, term)) {
    return false;   // no change
  }

  // highly nonideal.. the problem is that by using annotations in
  // the structures themselves, I have a hard time saying that I
  // intend to modify the annotations but not the "key" data...
  // this cast is really a symptom of that too.. (and, perhaps, also
  // that I don't have a List class that promises to never permit
  // modification of the pointed-to data.. but it's not clear I'd
  // be better of using it here even if I had it)
  
  NT->first.prepend(const_cast<Terminal*>(term));
  return true;      // changed it
}


void Grammar::readFile(char const *fname)
{
  FILE *fp = fopen(fname, "r");
  if (!fp) {
    xsyserror("fopen", stringb("opening " << fname));
  }

  char buf[256];
  int line = 0;
  while (fgets(buf, 256, fp)) {
    line++;   
    
    if (!parseLine(buf)) {
      cout << "error parsing line " << line << endl;
    }
  }

  if (fclose(fp) != 0) {
    xsyserror("fclose");
  }
}

// returns false on parse error
bool Grammar::parseLine(char const *line)
{
  StrtokParse tok(line, " \t\n\r");
  if (tok == 0 || tok[0][0] == '#') {
    // blank line or comment
    return true;
  }
		 
  // check that the 2nd token is the "rewrites-as" symbol
  if (0!=strcmp(tok[1], "->")) {
    return false;
  }

  // get LHS token
  Nonterminal *LHS = getOrMakeNonterminal(tok[0]);

  // make a production
  Production *prod = new Production(LHS);

  // process RHS symbols
  for (int i=2; i<tok; i++) {
    // alternatives -- syntactic sugar
    if (0==strcmp(tok[i], "|")) {
      // finish the current production
      addProduction(prod);

      // start another
      prod = new Production(LHS);
    }

    else {
      // normal symbol
      Symbol *sym = getOrMakeSymbol(tok[i]);
      prod->append(sym);
    }
  }

  // done, so add the production
  addProduction(prod);

  // ok
  return true;
}


// ------------------- symbol access -------------------
#define FOREACH_NONTERMINAL(list,iter) \
  for(ObjListIter<Nonterminal> iter(list); !iter.isDone(); iter.adv())

Nonterminal const *Grammar::findNonterminalC(char const *name) const
{
  // check for empty first, since it's not in the list
  if (emptyString.name == name) {
    return &emptyString;
  }

  FOREACH_NONTERMINAL(nonterminals, iter) {
    if (iter.data()->name == name) {
      return iter.data();
    }
  }
  return NULL;
}


#define FOREACH_TERMINAL(list,iter) \
  for(ObjListIter<Terminal> iter(list); !iter.isDone(); iter.adv())

Terminal const *Grammar::findTerminalC(char const *name) const
{
  FOREACH_TERMINAL(terminals, iter) {
    if (iter.data()->name == name) {
      return iter.data();
    }
  }
  return NULL;
}


Symbol const *Grammar::findSymbolC(char const *name) const
{
  // try nonterminals
  Nonterminal const *nt = findNonterminalC(name);
  if (nt) {
    return nt;
  }

  // now try terminals; if it fails, we fail
  return findTerminalC(name);
}



Nonterminal *Grammar::getOrMakeNonterminal(char const *name)
{
  Nonterminal *nt = findNonterminal(name);
  if (nt != NULL) {
    return nt;
  }
  
  nt = new Nonterminal(name);
  nonterminals.append(nt);
  return nt;
}

Terminal *Grammar::getOrMakeTerminal(char const *name)
{
  Terminal *term = findTerminal(name);
  if (term != NULL) {
    return term;
  }

  term = new Terminal(name);
  terminals.append(term);
  return term;
}

Symbol *Grammar::getOrMakeSymbol(char const *name)
{
  Symbol *sym = findSymbol(name);
  if (sym != NULL) {
    return sym;
  }

  // Since name is not already defined, we don't know whether
  // it will be a nonterminal or a terminal.  For now, I will
  // use the lexical convention that nonterminals are
  // capitalized and terminals are not.
  if (isupper(name[0])) {
    return getOrMakeNonterminal(name);
  }
  else {
    return getOrMakeTerminal(name);
  }
}



// ----------------- Grammar algorithms --------------------------
void Grammar::initializeAuxData()
{
  // at the moment, calling this twice leaks memory
  xassert(!initialized);


  // map: ntIndex -> Nonterminal*
  indexedNonTerms = new Nonterminal* [numNonterminals()];

  // fill it
  indexedNonTerms[emptyStringIndex] = &emptyString;
  int index = emptyStringIndex;
  emptyString.ntIndex = index++;

  for (ObjListMutator<Nonterminal> sym(nonterminals);
       !sym.isDone(); index++, sym.adv()) {
    indexedNonTerms[index] = sym.data();    // map: index to symbol
    sym.data()->ntIndex = index;            // map: symbol to index
  }


  // map: termIndex -> Terminal*
  indexedTerms = new Terminal* [numTerminals()];
  index = 0;
  for (ObjListMutator<Terminal> sym(terminals);
       !sym.isDone(); index++, sym.adv()) {
    indexedTerms[index] = sym.data();    // map: index to symbol
    sym.data()->termIndex = index;       // map: symbol to index
  }


  // initialize the derivable relation
  initDerivableRelation();


  // mark the grammar as initialized
  initialized = true;
}


void Grammar::computeWhatCanDeriveWhat()
{
  xassert(initialized);


  // iterate: propagate 'true' bits across the derivability matrix
  // (i.e. compute transitive closure on the canDerive relation)
  for (;;) {
    int changes = 0;       // for this iter, # of times we set a matrix bit

    // --------- first part: add new canDerive relations --------
    // loop over all productions
    for (ObjListIter<Production> prodIter(productions);
         !prodIter.isDone(); prodIter.adv()) {
      // convenient alias
      Production const *prod = prodIter.data();

      // iterate over RHS symbols, seeing if the LHS can derive that
      // RHS symbol (by itself)
      for (SymbolListIter rightSym(prod->right);
           !rightSym.isDone(); rightSym.adv()) {

        if (rightSym.data()->isTerminal) {
          // if prod->left derives a string containing a terminal,
          // then it can't derive any nontermial alone (using this
          // production, at least) -- empty is considered a nonterminal
          break;
        }

        // otherwise, it's a nonterminal
        Nonterminal const &rightNT = rightSym.data()->asNonterminalC();

        // check if we already know that LHS derives rightNT
        if (canDerive(prod->left, &rightNT)) {
          // we already know that prod->left derives rightSym,
          // so let's not check it again
        }

        else {
          // we are wondering if prod->left can derive rightSym.. for
          // this to be true, every symbol that comes after rightSym
          // must be able to derive emptySymbol (we've already verified
          // by now that every symbol to the *left* can derive empty)
          SymbolListIter afterRightSym(rightSym);
          bool restDeriveEmpty = true;
          for (afterRightSym.adv();    // *after* right symbol
               !afterRightSym.isDone(); afterRightSym.adv()) {

            if (afterRightSym.data()->isTerminal  ||
                  // if it's a terminal, it can't derive emptyString
                !canDeriveEmpty(&( afterRightSym.data()->asNonterminalC() ))) {
                  // this symbol can't derive empty string (or, we don't
                  // yet know that it can), so we conclude that prod->left
                  // can't derive rightSym
              restDeriveEmpty = false;
              break;
            }
          }

          if (restDeriveEmpty) {
            // we have discovered that prod->left can derive rightSym
            bool chgd = addDerivable(prod->left, &rightNT);
            xassert(chgd);    // above, we verified we didn't already know this

            changes++;

            cout << "discovered (by production): " << prod->left->name
                 << " ->* " << rightNT.name << "\n";
          }
        }

        // ok, we've considered prod->left deriving rightSym.  now, we
        // want to consider whether prod->left can derive any of the
        // symbols that follow rightSym in this production.  for this
        // to be true, rightSym itself must derive the emptyString
        if (!canDeriveEmpty(&rightNT)) {
          // it doesn't -- no point in further consideration of
          // this production
          break;
        }
      } // end of loop over RHS symbols
    } // end of loop over productions


    // -------- second part: compute closure over existing relations ------
    // I'll do this by computing R + R^2 -- that is, I'll find all
    // paths of length 2 and add an edge between their endpoints.
    // I do this, rather than computing the entire closure now, since
    // on the next iter I will add more relations and have to re-do
    // a full closure; iterative progress seems a better way.

    // I don't consider edges (u,u) because it messes up my cyclicty
    // detection logic.  (But (u,v) and (v,u) is ok, and in fact is
    // what I want, for detecting cycles.)

    // for each node u (except empty)
    int numNonterms = numNonterminals();
    for (int u=1; u<numNonterms; u++) {
      // for each edge (u,v) where u != v
      for (int v=0; v<numNonterms; v++) {
        if (u==v || !canDerive(u,v)) continue;

        // for each edge (v,w) where v != w
        for (int w=0; w<numNonterms; w++) {
          if (v==w || !canDerive(v,w)) continue;

          // add an edge (u,w), if there isn't one already
          if (addDerivable(u,w)) {
            changes++;
            cout << "discovered (by closure step): " 
                 << indexedNonTerms[u]->name << " ->* "
                 << indexedNonTerms[w]->name << "\n";
          }
        }
      }
    }


    // ------ finally: iterate until no changes -------
    if (changes == 0) {
      // didn't make any changes during the last iter, so
      // everything has settled
      break;
    }
  } // end of loop until settles


  // I used to do all closure here and no closure in the loop.
  // But that fails in cases where closure (when it reveals
  // more things that derive emptyString) yields new opportunities
  // for derives-relation discovery.  Therefore I now alternate
  // between them, and at the end, no closure is necessary.
}


// Compute, for each nonterminal, the "First" set, defined as:
//
//   First(N) = { x | N ->* x alpha }, where alpha is any sequence
//                                     of terminals and nonterminals
//
// If N can derive emptyString, I'm going to say that empty is
// *not* in First, despite what Aho/Sethi/Ullman says.  I do this
// because I have that information readily as my derivable relation,
// and because it violates the type system I've devised.
//
// I also don't "compute" First for terminals, since they are trivial
// (First(x) = {x}).
void Grammar::computeFirst()
{
  int numTerms = numTerminals();     // loop invariant

  // iterate, looking for new First members, until no changes
  int changes = 1;   // so the loop begins
  while (changes > 0) {
    changes = 0;

    // for each production
    for (ObjListMutator<Production> prodIter(productions);
         !prodIter.isDone(); prodIter.adv()) {
      // convenient aliases
      Production *prod = prodIter.data();
      Nonterminal *LHS = prod->left;
        // the list iter is mutating because I modify LHS's First list

      // for each right-hand side member such that all
      // preceeding RHS members can derive emptyString
      for (SymbolListIter rightSym(prod->right);
           !rightSym.isDone(); rightSym.adv()) {

        if (rightSym.data()->isTerminal) {
          // LHS -> x alpha   means x is in First(LHS)
          changes += true==addFirst(LHS, &( rightSym.data()->asTerminalC() ));
          break;    // stop considering RHS members since a terminal
                    // effectively "hides" all further symbols from First
        }

        // rightSym must be a nonterminal
        Nonterminal const &rightNT = rightSym.data()->asNonterminalC();

        // anything already in rightNT's First should be added to mine:
        // for each element in First(rightNT)
        for (int t=0; t<numTerms; t++) {
          if (firstIncludes(&rightNT, indexedTerms[t])) {
            // add it to First(LHS)
            changes += true==addFirst(LHS, indexedTerms[t]);
          }
        }

        // if rightNT can't derive emptyString, then it blocks further
        // consideration of right-hand side members
        if (!canDeriveEmpty(&rightNT)) {
          break;
        }
      } // for (RHS members)
    } // for (productions)
  } // while (changes)
}


void Grammar::computeFollow()
{
  





// ---------------------------- main --------------------------------
void pretendUsed(...)
{}


void Grammar::exampleGrammar()
{
  // for now, let's use a hardcoded grammar


  // grammar 4.11 of ASU (p.189), designed to show First and Follow
  parseLine("E  ->  T E'               ");
  parseLine("E' ->  + T E'  | empty    ");
  parseLine("T  ->  F T'               ");
  parseLine("T' ->  * F T'  | empty    ");
  parseLine("F  ->  ( E )   | id       ");


  #if 0
  // terminals: "a", "b", .. "e"
  char const termLetters[] = "abcde";
  Terminal *terms[5];
  loopi(5) {
    char s[2];          // will be e.g. "b\0"
    s[0] = termLetters[i];
    s[1] = 0;
    terms[i] = new Terminal(s);
    terminals.append(terms[i]);
  }

  // give then convenient names
  Terminal *a = terms[0];
  Terminal *b = terms[1];
  Terminal *c = terms[2];
  Terminal *d = terms[3];
  Terminal *e = terms[4];

  // nonterminals
  char const * const nontermNames[] = {
    "Start", "A", "B", "C", "D"
  };
  Nonterminal *nonterms[5];
  loopi(5) {
    nonterms[i] = new Nonterminal(nontermNames[i]);
    nonterminals.append(nonterms[i]);
  }

  // give them convenient names
  Nonterminal *S = nonterms[0];
  Nonterminal *A = nonterms[1];
  Nonterminal *B = nonterms[2];
  Nonterminal *C = nonterms[3];
  Nonterminal *D = nonterms[4];

  // start symbol
  startSymbol = S;

  // productions
  #define E  S
  #define Ep A
  #define T  B
  #define Tp C
  #define F  D
  #define plus a
  #define times b
  #define lparen c
  #define rparen d
  #define id e
  addProduction(E,  /* -> */   T, Ep,               NULL);
  addProduction(Ep, /* -> */   plus, T, Ep,         NULL);
  addProduction(Ep, /* -> */   &emptyString,        NULL);
  addProduction(T,  /* -> */   F, Tp,               NULL);
  addProduction(Tp, /* -> */   times, F, Tp,        NULL);
  addProduction(Tp, /* -> */   &emptyString,        NULL);
  addProduction(F,  /* -> */   lparen, E, rparen,   NULL);
  addProduction(F,  /* -> */   id,                  NULL);
  #endif // 0

  #if 0
  addProduction(S,  /* -> */   A, B, A, C,    NULL);
  addProduction(A,  /* -> */   B,             NULL);
  addProduction(B,  /* -> */   &emptyString,  NULL);
  addProduction(C,  /* -> */   A, D,          NULL);
  addProduction(D,  /* -> */   a,             NULL);
  #endif // 0

  #if 0
  addProduction(S,  /* -> */   A, B, A, C,    NULL);
  addProduction(A,  /* -> */   &emptyString,  NULL);
  addProduction(B,  /* -> */   C, D, C,       NULL);
  addProduction(C,  /* -> */   &emptyString,  NULL);
  addProduction(D,  /* -> */   a,             NULL);
  #endif // 0

  // verify we got what we expected
  printProductions();

  // precomputations
  initializeAuxData();
  computeWhatCanDeriveWhat();
  computeFirst();

  // print results
  cout << "Terminals:\n";
  printSymbols(toObjList(terminals));
  cout << "Nonterminals:\n";
  cout << "  ";
  emptyString.print();
  cout << endl;
  printSymbols(toObjList(nonterminals));

  derivable->print();
  
  // silence warnings
  //pretendUsed(a,b,c,d,e, S,A,B,C,D);
}


int main()
{
  Grammar g;
  g.exampleGrammar();
  return 0;
}



// ---------------------------------------------------------------------------
// ----------------------------- trash ---------------------------------------
// ---------------------------------------------------------------------------
  #if 0
  // compute the transitive closure of what we've got so far
  // e.g., if we've computed that A ->* B and B ->* C, write
  // down that A ->* C

  for (;;) {
    // loop until things settle
    int changes=0;

    loopi(numNonterms) {
      loopj(numNonterms) {
        if (i==j) {
          continue;    // ignore the diagonal
        }

        if (derivable->get(point(i,j))) {
          // nonterminal i derives nonterminal j, so anything that
          // nonterminal j can derive is derivable from i
          for (int row=0; row<numNonterms; row++) {
            if (derivable->get(point(j,row))) {           // if 'j' derive 'row'
              if (!derivable->testAndSet(point(i,row))) {   // then 'i' can derive 'row'
                cout << "discovered (by full closure): " << nonterms[i]->name
                     << " ->* " << nonterms[row]->name << "\n";
                changes++;     // and if that changed it, record it
              }
            }
          }
        }
      }
    }

    if (changes==0) {
      // settled
      break;
    }
  }
  #endif // 0


#if 0
void Grammar::computeWhichCanDeriveEmpty()
{
  // clear all the canDeriveEmpty flags
  for (ObjListMutator<Symbol> iter(terminals);
       !iter.isDone(); iter.adv()) {
    iter.data()->canDeriveEmpty = false;
  }
  for (ObjListMutator<Symbol> iter(nonterminals);
       !iter.isDone(); iter.adv()) {
    iter.data()->canDeriveEmpty = false;
  }

  // set the seed flag
  emptyString->canDeriveEmpty = true;

  // iterate: propagate canDeriveEmpty up the grammar
  for (;;) {
    int changes = 0;       // for this iter, times we set canDeriveEmpty

    // loop over all productions
    for (ObjListMutator<Production> prodIter(productions);
         !prodIter.isDone(); prodIter.adv()) {
      // convenient alias
      Production *prod = prodIter.data();
      if (prod->left->canDeriveEmpty) {
        continue;     // already set; skip it
      }

      // see if every symbol on the RHS can derive emptyString
      bool allDeriveEmpty = true;
      for (SymbolListIter sym(prod->right);
           !sym.isDone(); sym.adv()) {
        if (!sym.data()->canDeriveEmpty) {
          allDeriveEmpty = false;
          break;
        }
      }

      if (allDeriveEmpty) {
        prod->left->canDeriveEmpty = true;
        changes++;
      }
    }

    if (changes == 0) {
      // everything has settled; we're done
      break;
    }
  }
}
#endif // 0



