// grammar.cc
// code for grammar.h

#include "grammar.h"   // this module
#include "bit2d.h"     // Bit2d

#include <stdarg.h>    // variable-args stuff
#include <iostream.h>  // cout, etc.


// print a variable value
#define PVAL(var) cout << " " << #var "=" << var;


// ---------------------- Symbol --------------------
void Symbol::print() const
{
  cout << "[" << ntindex << "] " << name << ":";
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


// -------------------- Production -------------------------
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


// ------------------ Grammar -----------------
Grammar::Grammar()
  : derivable(NULL),
    startSymbol(NULL)

{
  emptyString = new Symbol("empty", false /*terminal*/);
}


Grammar::~Grammar()
{
  delete emptyString;
  delete derivable;
}


void Grammar::printProductions() const
{
  cout << "Grammar productions:\n";
  for (ObjListIter<Production> iter(productions);
       !iter.isDone(); iter.adv()) {
    cout << "  ";
    iter.data()->print();
    cout << endl;
  }
}


void Grammar::addProduction(Symbol *lhs, Symbol *firstRhs, ...)
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

  productions.append(prod);
}


// ----------------- Grammar algorithms --------------------------
void Grammar::computeWhatCanDeriveWhat()
{
  // one-dimensional indexing structure for accessing nonterminals
  // (for the purposes of this algorithm, emptyString is a nonterminal)
  int numNonterms = nonterminals.count() + 1 /*emptyString*/;
  Symbol **nonterms = new Symbol* [numNonterms];       // (owner)

  // fill it
  {
    nonterms[emptyStringIndex] = emptyString;
    emptyString->ntindex = emptyStringIndex;
    int index=1;
    for (ObjListMutator<Symbol> sym(nonterminals);
         !sym.isDone(); index++, sym.adv()) {
      nonterms[index] = sym.data();    // map: index to symbol
      sym.data()->ntindex = index;     // map: symbol to index
    }
  }


  // two-dimensional to represent token derivabilities
  derivable = new Bit2d(point(numNonterms, numNonterms));

  // initialize it
  {
    derivable->setall(0);
    loopi(numNonterms) {
      derivable->set(point(i,i));
        // every nonterminal can derive itself in 0 or more steps
        // (specifically, in 0 steps, at least)
    }
  }


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

        if (derivable->get(point(prod->left->ntindex,
                                 rightSym.data()->ntindex))) {
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

            if (!derivable->get(point(afterRightSym.data()->ntindex,
                                      emptyStringIndex))) {
              // this symbol can't derive empty string (or, we don't
              // yet know that it can), so we conclude that prod->left
              // can't derive rightSym
              restDeriveEmpty = false;
              break;
            }
          }

          if (restDeriveEmpty) {
            // we have discovered that prod->left can derive rightSym
            derivable->set(point(prod->left->ntindex,
                                 rightSym.data()->ntindex));
            changes++;

            cout << "discovered (by production): " << prod->left->name
                 << " ->* " << rightSym.data()->name << "\n";
          }
        }

        // ok, we've considered prod->left deriving rightSym.  now, we
        // want to consider whether prod->left can derive any of the
        // symbols that follow rightSym in this production.  for this
        // to be true, rightSym itself must derive the emptyString
        if (!derivable->get(point(rightSym.data()->ntindex,
                                  emptyStringIndex))) {
          // it doesn't -- no point in further consideration of
          // this production
          break;
        }
      } // end of loop over RHS symbols
    } // end of loop over productions


    // -------- second part: compute closure over existing relations ------
    // I'll do this by computing R + R^2 -- that is, I'll find all
    // paths of length 2 and add an edge between their endpoings.
    // I do this, rather than computing the entire closure now, since
    // on the next iter I will add more relations and have to re-do
    // a full closure; iterative progress seems the better way.

    // for each node u (except empty)
    for (int u=1; u<numNonterms; u++) {
      // for each edge (u,v)
      for (int v=0; v<numNonterms; v++) {
        if (!derivable->get(point(u,v))) continue;

        // for each edge (v,w)
        for (int w=0; w<numNonterms; w++) {
          if (!derivable->get(point(v,w))) continue;

       	  // add an edge (u,w), if there isn't one already
       	  if (!derivable->get(point(u,w))) {
            derivable->set(point(u,w));
            changes++;
            cout << "discovered (by closure step): " << nonterms[u]->name
                 << " ->* " << nonterms[w]->name << "\n";
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
  // more things that derive emptyStrnig) yields new opportunities
  // for derives-relation discovery.  Therefore I now alternate
  // between them, and at the end, no closure is necessary.


  delete[] nonterms;
}


void Grammar::exampleGrammar()
{
  // for now, let's use a hardcoded grammar

  // terminals: "a", "b", .. "e"
  char const termLetters[] = "abcde";
  Symbol *terms[5];
  loopi(5) {
    char s[2];          // will be e.g. "b"
    s[0] = termLetters[i];
    s[1] = 0;
    terms[i] = new Symbol(s, true /*terminal*/);
    terminals.append(terms[i]);
  }

  // give then convenient names
  Symbol *a = terms[0];
  #if 0
    Symbol *b = terms[1];
    Symbol *c = terms[2];
    Symbol *d = terms[3];
    Symbol *e = terms[4];
  #endif // 0

  // nonterminals
  char const * const nontermNames[] = {
    "Start", "A", "B", "C", "D"
  };
  Symbol *nonterms[5];
  loopi(5) {
    nonterms[i] = new Symbol(nontermNames[i], false /*terminal*/);
    nonterminals.append(nonterms[i]);
  }

  // give them convenient names
  Symbol *S = nonterms[0];
  Symbol *A = nonterms[1];
  Symbol *B = nonterms[2];
  Symbol *C = nonterms[3];
  Symbol *D = nonterms[4];

  // start symbol
  startSymbol = S;

  // productions
  addProduction(S,  /* -> */   A, B, A, C,   NULL);
  addProduction(A,  /* -> */   B,            NULL);
  addProduction(B,  /* -> */   emptyString,  NULL);
  addProduction(C,  /* -> */   A, D,         NULL);
  addProduction(D,  /* -> */   a,            NULL);

  #if 0
  addProduction(S,  /* -> */   A, B, A, C,   NULL);
  addProduction(A,  /* -> */   emptyString,  NULL);
  addProduction(B,  /* -> */   C, D, C,      NULL);
  addProduction(C,  /* -> */   emptyString,  NULL);
  addProduction(D,  /* -> */   a,            NULL);
  #endif // 0

  // verify we got what we expected
  printProductions();

  // precomputations
  computeWhatCanDeriveWhat();

  // print results
  cout << "Terminals:\n";
  printSymbols(terminals);
  cout << "Nonterminals:\n";
  cout << "  ";
  emptyString->print();
  cout << endl;
  printSymbols(nonterminals);

  derivable->print();
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



