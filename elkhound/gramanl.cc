// gramanl.cc
// code for gramanl.h

#include "gramanl.h"     // this module

#include "bit2d.h"       // Bit2d
#include "strtokp.h"     // StrtokParse


GrammarAnalysis::GrammarAnalysis()
  : derivable(NULL),
    indexedNonterms(NULL),
    indexedTerms(NULL),
    initialized(false),
    nextItemSetId(0),    // [ASU] starts at 0 too
    cyclic(false)
{}


GrammarAnalysis::~GrammarAnalysis()
{
  if (indexedNonterms != NULL) {
    delete indexedNonterms;
  }

  if (indexedTerms != NULL) {
    delete indexedTerms;
  }

  if (derivable != NULL) {
    delete derivable;
  }
}


void GrammarAnalysis::printProductions(ostream &os) const
{
  if (cyclic) {
    os << "(cyclic!) ";
  }
  Grammar::printProductions(os);
}


void printSymbols(ostream &os, ObjList<Symbol> const &list)
{
  for (ObjListIter<Symbol> iter(list);
       !iter.isDone(); iter.adv()) {
    os << "  " << *(iter.data()) << endl;
  }
}


bool GrammarAnalysis::addDerivable(Nonterminal const *left, Nonterminal const *right)
{
  return addDerivable(left->ntIndex, right->ntIndex);
}

bool GrammarAnalysis::addDerivable(int left, int right)
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
    Nonterminal *NT = indexedNonterms[left];    // ==right
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


bool GrammarAnalysis::canDerive(Nonterminal const *left, Nonterminal const *right) const
{
  return canDerive(left->ntIndex, right->ntIndex);
}

bool GrammarAnalysis::canDerive(int left, int right) const
{
  return 1 == derivable->get(point(left, right));
}


void GrammarAnalysis::initDerivableRelation()
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


bool GrammarAnalysis::canDeriveEmpty(Nonterminal const *nonterm) const
{
  return canDerive(nonterm, &emptyString);
}


bool GrammarAnalysis::sequenceCanDeriveEmpty(SymbolList const &list) const
{
  SymbolListIter iter(list);
  return iterSeqCanDeriveEmpty(iter);
}

bool GrammarAnalysis::iterSeqCanDeriveEmpty(SymbolListIter iter) const
{
  // look through the sequence beginning with 'iter'; if any members cannot
  // derive emptyString, fail
  for (; !iter.isDone(); iter.adv()) {
    if (iter.data()->isTerminal()) {
      return false;    // terminals can't derive emptyString
    }

    if (!canDeriveEmpty(&( iter.data()->asNonterminalC() ))) {
      return false;    // nonterminal that can't derive emptyString
    }
  }

  return true;
}


bool GrammarAnalysis::firstIncludes(Nonterminal const *NT, Terminal const *term) const
{
  return NT->first.contains(term);
}

bool GrammarAnalysis::addFirst(Nonterminal *NT, Terminal *term)
{
  return NT->first.prependUnique(term);

  // regarding non-constness of 'term':
  // highly nonideal.. the problem is that by using annotations in
  // the structures themselves, I have a hard time saying that I
  // intend to modify the annotations but not the "key" data...
  // this cast is really a symptom of that too.. (and, perhaps, also
  // that I don't have a List class that promises to never permit
  // modification of the pointed-to data.. but it's not clear I'd
  // be better of using it here even if I had it)
}


bool GrammarAnalysis::followIncludes(Nonterminal const *NT, Terminal const *term) const
{
  return NT->follow.contains(term);
}

// returns true if Follow(NT) is changed by adding 'term' to it
bool GrammarAnalysis::addFollow(Nonterminal *NT, Terminal *term)
{
  return NT->follow.prependUnique(term);
}


// ----------------- Grammar algorithms --------------------------
void GrammarAnalysis::initializeAuxData()
{
  // at the moment, calling this twice leaks memory
  xassert(!initialized);


  // map: ntIndex -> Nonterminal*
  indexedNonterms = new Nonterminal* [numNonterminals()];

  // fill it
  indexedNonterms[emptyStringIndex] = &emptyString;
  int index = emptyStringIndex;
  emptyString.ntIndex = index++;

  for (ObjListMutator<Nonterminal> sym(nonterminals);
       !sym.isDone(); index++, sym.adv()) {
    indexedNonterms[index] = sym.data();    // map: index to symbol
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

  
  // dotted productions
  MUTATE_EACH_PRODUCTION(productions, prod) {
    prod.data()->finished();
  }


  // mark the grammar as initialized
  initialized = true;
}


void GrammarAnalysis::computeWhatCanDeriveWhat()
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

        if (rightSym.data()->isTerminal()) {
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

            if (afterRightSym.data()->isTerminal()  ||
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
                 << indexedNonterms[u]->name << " ->* "
                 << indexedNonterms[w]->name << "\n";
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
void GrammarAnalysis::computeFirst()
{
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

      // compute First(RHS-sequence)
      TerminalList firstOfRHS;
      firstOfSequence(firstOfRHS, prod->right);

      // add everything in First(RHS-sequence) to First(LHS)
      SMUTATE_EACH_TERMINAL(firstOfRHS, iter) {
       	changes += true==addFirst(LHS, iter.data());
      }
    } // for (productions)
  } // while (changes)
}


// 'sequence' isn't const because we need to hand pointers over to
// the 'destList', which isn't const; similarly for 'this'
// (what I'd like here is to say that 'sequence' and 'this' are const
// if 'destList' can't modify the things it contains)
void GrammarAnalysis::firstOfSequence(TerminalList &destList, SymbolList &sequence)
{
  SymbolListMutator iter(sequence);
  firstOfIterSeq(destList, iter);
}

// similar to above, 'sym' needs to be a mutator
void GrammarAnalysis::firstOfIterSeq(TerminalList &destList, SymbolListMutator sym)
{
  int numTerms = numTerminals();     // loop invariant

  // for each sequence member such that all
  // preceeding members can derive emptyString
  for (; !sym.isDone(); sym.adv()) {
    // LHS -> x alpha   means x is in First(LHS)
    if (sym.data()->isTerminal()) {
      destList.append(&( sym.data()->asTerminal() ));
      break;    // stop considering RHS members since a terminal
		// effectively "hides" all further symbols from First
    }

    // sym must be a nonterminal
    Nonterminal const &nt = sym.data()->asNonterminalC();

    // anything already in nt's First should be added to destList:
    // for each element in First(nt)
    for (int t=0; t<numTerms; t++) {
      if (firstIncludes(&nt, indexedTerms[t])) {
       	destList.append(indexedTerms[t]);
      }
    }

    // if nt can't derive emptyString, then it blocks further
    // consideration of right-hand side members
    if (!canDeriveEmpty(&nt)) {
      break;
    }
  } // for (RHS members)
}


void GrammarAnalysis::computeFollow()
{
  int numTerms = numTerminals();     // loop invariant

  // loop until no changes
  int changes = 1;
  while (changes > 0) {
    changes = 0;

    // 'mutate' is needed because adding 'term' to the follow of 'nt'
    // needs a mutable 'term' and 'nt'

    // for each production
    MUTATE_EACH_PRODUCTION(productions, prodIter) {
      Production *prod = prodIter.data();

      // for each RHS nonterminal member
      SMUTATE_EACH_SYMBOL(prod->right, rightSym) {
        if (rightSym.data()->isTerminal()) continue;

        // convenient alias
        Nonterminal &rightNT = rightSym.data()->asNonterminal();
        
        // I'm not sure what it means to compute Follow(emptyString),
        // so let's just not do so
        if (&rightNT == &emptyString) {
          continue;
        }

        // an iterator pointing to the symbol just after
        // 'rightSym' will be useful below
       	SymbolListMutator afterRightSym(rightSym);
       	afterRightSym.adv();    // NOTE: 'isDone()' may be true now

	// rule 1:
	// if there is a production A -> alpha B beta, then
	// everything in First(beta) is in Follow(B)
        {
	  // compute First(beta)
	  TerminalList firstOfBeta;
       	  firstOfIterSeq(firstOfBeta, afterRightSym);

  	  // put those into Follow(rightNT)
  	  SMUTATE_EACH_TERMINAL(firstOfBeta, term) {
  	    changes += true==
  	      addFollow(&rightNT, term.data());
  	  }
        }

  	// rule 2:
  	// if there is a production A -> alpha B, or a
    	// production A -> alpha B beta where beta ->* empty,
       	// then everything in Follow(A) is in Follow(B)
        if (iterSeqCanDeriveEmpty(afterRightSym)) {
          // for each element in Follow(LHS)
          for (int t=0; t<numTerms; t++) {
            if (followIncludes(prod->left, indexedTerms[t])) {
	      changes += true==
                addFollow(&rightNT, indexedTerms[t]);
            }
          } // for each in Follow(LHS)
        }

      } // for each RHS nonterminal member
    } // for each production
  } // until no changes
}


// [ASU] alg 4.4, p.190
void GrammarAnalysis::computePredictiveParsingTable()
{
  int numTerms = numTerminals();
  int numNonterms = numNonterminals();

  // the table will be a 2d array of lists of productions
  ProductionList *table = new ProductionList[numTerms * numNonterms];     // (owner)
  #define TABLE(term,nt) table[(term) + (nt)*numNonterms]

  // for each production 'prod' (non-const iter because adding them
  // to ProductionList, which doesn't promise to not change them)
  MUTATE_EACH_PRODUCTION(productions, prodIter) {
    Production *prod = prodIter.data();

    // for each terminal 'term' in First(RHS)
    TerminalList firsts;
    firstOfSequence(firsts, prod->right);
    SFOREACH_TERMINAL(firsts, term) {
      // add 'prod' to table[LHS,term]
      TABLE(prod->left->ntIndex,
            term.data()->termIndex).prependUnique(prod);
    }

    // if RHS ->* emptyString, ...
    if (sequenceCanDeriveEmpty(prod->right)) {
      // ... then for each terminal 'term' in Follow(LHS), ...
      SFOREACH_TERMINAL(prod->left->follow, term) {
        // ... add 'prod' to table[LHS,term]
        TABLE(prod->left->ntIndex,
              term.data()->termIndex).prependUnique(prod);
      }
    }
  }


  // print the resulting table:
  // for each nonterminal
  INTLOOP(nonterm, 0, numNonterms) {
    cout << "Row " << indexedNonterms[nonterm]->name << ":\n";

    // for each terminal
    INTLOOP(term, 0, numTerms) {
      cout << "  Column " << indexedTerms[term]->name << ":";

      // for each production in table[nonterm,term]
      SFOREACH_PRODUCTION(TABLE(nonterm,term), prod) {
        cout << "   ";
        prod.data()->print(cout);
      }

      cout << endl;
    }
  }

  // cleanup
  #undef TABLE
  delete[] table;
}


// [ASU] figure 4.33, p.223
void GrammarAnalysis::itemSetClosure(DProductionList &itemSet)
{
  // while no changes
  int changes = 1;
  while (changes > 0) {
    changes = 0;

    // for each item A -> alpha . B beta in itemSet
    SFOREACH_DOTTEDPRODUCTION(itemSet, itemIter) {          // (constness ok)
      DottedProduction const *item = itemIter.data();

      // get the symbol B (the one right after the dot)
      if (item->isDotAtEnd()) continue;
      Symbol const *B = item->symbolAfterDotC();
      if (B->isTerminal()) continue;

      // for each production B -> gamma
      MUTATE_EACH_PRODUCTION(productions, prod) {	    // (constness)
	if (prod.data()->left != B) continue;

        // add B -> . gamma to the itemSet if not already there
        changes += true==
          itemSet.appendUnique(
            prod.data()->getDProd(0 /*dot placement*/));    // (constness)

      }	// for each production
    } // for each item
  } // while changes
}
  

// -------------- START of construct LR item sets -------------------
ItemSet *GrammarAnalysis::makeItemSet()
{
  return new ItemSet(nextItemSetId++, numTerminals(), numNonterminals());
}

void GrammarAnalysis::disposeItemSet(ItemSet *is)
{
  // we assume we're only doing this right after making it, as the
  // point of this exercise is to avoid fragmenting the id space
  nextItemSetId--;
  xassert(is->id == nextItemSetId);
  delete is;
}


// yield a new itemset by moving the dot across the productions
// in 'source' that have 'symbol' to the right of the dot; then
// compute the closure
ItemSet *GrammarAnalysis::moveDot(ItemSet const *source, Symbol const *symbol)
{
  ItemSet *ret = makeItemSet();

  // for each item
  SFOREACH_DOTTEDPRODUCTION(source->items, dprodi) {
    DottedProduction const *dprod = dprodi.data();

    if (dprod->isDotAtEnd() ||
        dprod->symbolAfterDotC() != symbol) {
      continue;    // can't move dot
    }		  
    
    // move the dot
    DottedProduction *dotMoved =
      dprod->prod->getDProd(dprod->dot + 1);   	  // (constness!)

    // add the new item to the itemset I'm building
    ret->items.append(dotMoved);
  }
  
  // for now, verify we actually got something; though it would
  // be easy to simply return null (after dealloc'ing ret)
  xassert(ret->items.isNotEmpty());

  // compute closure of resulting set
  itemSetClosure(ret->items);

  // return built itemset
  return ret;
}


// if 'list' contains 'itemSet', return the equivalent copy
// in 'list'; otherwise, return NULL
// 'list' is non-const because might return an element of it
ItemSet *GrammarAnalysis::findItemSetInList(ObjList<ItemSet> &list,
                                            ItemSet const *itemSet)
{
  // inefficiency: using iteration to check set membership

  MUTATE_EACH_OBJLIST(ItemSet, list, iter) {
    if (itemSetsEqual(iter.data(), itemSet)) {
      return iter.data();
    }
  }
  return NULL;
}


// true if 'small' is a subset of 'big'
bool GrammarAnalysis::itemSetContainsItemSet(ItemSet const *big,
                                             ItemSet const *small)
{
  SFOREACH_DOTTEDPRODUCTION(small->items, iter) {
    if (!big->items.contains(iter.data())) {
      return false;
    }
  }
  return true;
}

bool GrammarAnalysis::itemSetsEqual(ItemSet const *is1, ItemSet const *is2)
{
  // inefficiency: n^2 set equality test
  // inefficiency: linear set membership tests ('contains()')

  // check that each is a subset of the other
  return itemSetContainsItemSet(is1, is2) &&
         itemSetContainsItemSet(is2, is1);
}


// [ASU] fig 4.34, p.224
// puts the finished parse tables into 'itemSetsDone'
void GrammarAnalysis::constructLRItemSets(ObjList<ItemSet> &itemSetsDone)
{
  // set of item sets
  ObjList<ItemSet> itemSetsPending;    // yet to be processed
    // (those in 'itemSetsDone' have all outgoing links processed)

  // start by constructing closure of first production
  // (basically assumes first production has start symbol
  // on LHS, and no other productions have the start symbol
  // on LHS)
  {
    ItemSet *is = makeItemSet();       	     // (owner)
    is->items.append(productions.nth(0)->    // first production's ..
                       getDProd(0));	     //   .. first dot placement
    itemSetClosure(is->items);

    // this makes the initial pending itemSet
    itemSetsPending.append(is);		     // (ownership transfer)
  }


  // for each pending item set
  while (!itemSetsPending.isEmpty()) {
    ItemSet *itemSet = itemSetsPending.removeAt(0);    // dequeue   (owner; ownership transfer)

    // put it in the done set; note that we must do this *before*
    // the processing below, to properly handle self-loops
    itemSetsDone.append(itemSet);                      // (ownership transfer; 'itemSet' becomes serf)

    // for each production in the item set where the
    // dot is not at the right end
    SFOREACH_DOTTEDPRODUCTION(itemSet->items, dprodIter) {
      DottedProduction const *dprod = dprodIter.data();
      if (dprod->isDotAtEnd()) continue;

      // get the symbol 'sym' after the dot (next to be shifted)
      Symbol const *sym = dprod->symbolAfterDotC();
	
      // if we already have a transition for this symbol,
      // there's nothing more to be done
      if (itemSet->transitionC(sym) != NULL) {
        continue;
      }

      // fixed: adding transition functions fixes this
      // inefficiency: if several productions have X to the
      // left of the dot, then we will 'moveDot' each time

      // compute the itemSet produced by moving the dot
      // across 'sym' and taking the closure
      ItemSet *withDotMoved = moveDot(itemSet, sym);

      // inefficiency: we go all the way to materialize the
      // itemset before checking whether we already have it

      // see if we already have it, in either set
      ItemSet *already = findItemSetInList(itemSetsPending, withDotMoved);
      if (already == NULL) {
        already = findItemSetInList(itemSetsDone, withDotMoved);
      }

      // have it?
      if (already != NULL) {
        // we already have it, so throw away one we made
        disposeItemSet(withDotMoved);     // deletes 'withDotMoved'

        // and use existing one for setting the transition function
        withDotMoved = already;
      }
      else {
        // we don't already have it, so add it to 'pending'
        itemSetsPending.append(withDotMoved);
      }

      // setup the transition function
      itemSet->setTransition(sym, withDotMoved);

    } // for each item
  } // for each item set


  // print each item set
  FOREACH_OBJLIST(ItemSet, itemSetsDone, itemSet) {
    itemSet.data()->print(cout);
  }
}


// this is mostly [ASU] algorithm 4.7, p.218-219.  however,
// I've modified it to store one less item on the state stack
// (not really as optimization, just it made more sense to
// me that way)
void GrammarAnalysis::lrParse(ObjList<ItemSet> &itemSets, char const *input)
{
  // tokenize the input
  StrtokParse tok(input, " \t");

  // parser state
  int currentToken = 0;               // index of current token
  ItemSet *state = itemSets.nth(0);   // current parser state
  SObjList<ItemSet> stateStack;       // stack of parser states
  SObjList<Symbol> symbolStack;       // stack of shifted symbols

  // for each token of input
  while (currentToken < tok) {
    // map the token text to a symbol
    Terminal *symbol = findTerminal(tok[currentToken]);     // (constness)

    // see where a shift would go
    ItemSet *shiftDest = state->transition(symbol);

    // get all possible reductions where 'sym' is in Follow(LHS)
    ProductionList reductions;
    state->getPossibleReductions(reductions, symbol);

    // case analysis
    if (shiftDest != NULL &&
        reductions.isEmpty()) {            // unambiguous shift
      // push current state and symbol
      stateStack.prepend(state);
      symbolStack.prepend(symbol);

      // move to new state
      state = shiftDest;

      // and to next input symbol
      currentToken++;

      // debugging
      cout << "moving to state " << state->id
           << " after shifting symbol " << symbol->name << endl;
    }

    else if (shiftDest == NULL &&
             reductions.count() == 1) {	   // unambiguous reduction
      // get the production we're reducing by
      Production *prod = reductions.nth(0);

      // it is here that an action or tree-building step would
      // take place

      // pop as many symbols off stack as there are symbols on
      // the right-hand side of 'prod'; and pop one less as many
      // states off state stack
      INTLOOP(i, 1, prod->rhsLength()) {
        stateStack.removeAt(0);
        symbolStack.removeAt(0);
      }
      symbolStack.removeAt(0);

      // in [ASU] terms, the act of forgetting 'state' is the
      // "extra" state pop I didn't do above

      // get state now on stack top
      state = stateStack.nth(0);

      // get state we're going to move to (it's like a shift
      // on prod's LHS)
      ItemSet *gotoDest = state->transition(prod->left);

      // push prod's LHS (as in a shift)
      symbolStack.prepend(prod->left);

      // move to new state
      state = gotoDest;
			  
      // again, setting 'state' is what [ASU] accomplishes with
      // a push

      // debugging
      cout << "moving to state " << state->id
           << " after reducing by rule " << *prod << endl;
    }

    else if (shiftDest == NULL &&
	     reductions.isEmpty()) {       // no transition: syntax error
      cout << "no actions defined for symbol " << symbol->name
           << " in state " << state->id << endl;
      break;       // stop parsing
    }

    else {                                 // local ambiguity
      cout << "conflict for symbol " << symbol->name
           << " in state " << state->id
           << "; possible actions:\n";

      if (shiftDest) {
        cout << "  shift, and move to state " << shiftDest->id << endl;
      }

      SFOREACH_PRODUCTION(reductions, prod) {
        cout << "  reduce by rule " << *(prod.data()) << endl;
      }

      break;       // stop parsing
    }
  }

  // print final contents of stack; if the parse was successful,
  // I want to see what remains; if not, it's interesting anyway
  cout << "final contents of stacks (right is top):\n";
  stateStack.reverse();	   // more convenient for printing
  symbolStack.reverse();

  cout << "  state stack:";
  SFOREACH_OBJLIST(ItemSet, stateStack, stateIter) {
    cout << " " << stateIter.data()->id;
  }
  cout << " current=" << state->id;   // print current state too
  cout << endl;

  cout << "  symbol stack:";
  SFOREACH_SYMBOL(symbolStack, sym) {
    cout << " " << sym.data()->name;
  }
  cout << endl;
}

// --------------- END of construct LR item sets -------------------


// ---------------------------- main --------------------------------
void pretendUsed(...)
{}


void GrammarAnalysis::exampleGrammar()
{
  // for now, let's use a hardcoded grammar

  

  #if 0
    // grammar 4.13 of [ASU] (p.191)
    parseLine("Start  ->  S $                ");
    parseLine("S  ->  i E t S S'   |  a      ");
    parseLine("S' ->  e S          |  empty  ");
    parseLine("E  ->  b                      ");
  #endif // 0


  #if 0
    // grammar 4.11 of [ASU] (p.189), designed to show First and Follow
    parseLine("S  ->  E $                ");
    parseLine("E  ->  T E'               ");
    parseLine("E' ->  + T E'  | empty    ");
    parseLine("T  ->  F T'               ");
    parseLine("T' ->  * F T'  | empty    ");
    parseLine("F  ->  ( E )   | id       ");
  #endif // 0


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

  #if 1
    // [ASU] grammar 4.19, p.222: demonstrating LR sets-of-items construction
    parseLine("E' ->  E $                ");
    parseLine("E  ->  E + T  |  T        ");
    parseLine("T  ->  T * F  |  F        ");
    parseLine("F  ->  ( E )  |  id       ");

    char const *input[] = {
      " id                 $",
      " id + id            $",
      " id * id            $",
      " id + id * id       $",
      " id * id + id       $",
      " ( id + id ) * id   $",
      " id + id + id       $",
      " id + ( id + id )   $"
    };
  #endif // 0/1

  #if 0
    // [ASU] grammar 4.20, p.229: more sets-of-items
    parseLine("S' ->  S $                 ");
    parseLine("S  ->  L = R               ");
    parseLine("S  ->  R                   ");
    parseLine("L  ->  * R                 ");
    parseLine("L  ->  id                  ");
    parseLine("R  ->  L                   ");

    char const *input[] = {
      " id                 $",
      " id = id            $",
      " * id = id          $",
      " id = * id          $",
      " * id = * id        $",
      " * * id = * * id    $"
    };
  #endif // 0


  // verify we got what we expected
  printProductions(cout);

  // precomputations
  initializeAuxData();
  computeWhatCanDeriveWhat();
  computeFirst();
  computeFollow();

  // print results
  cout << "Terminals:\n";
  printSymbols(cout, toObjList(terminals));
  cout << "Nonterminals:\n";
  cout << "  " << emptyString << endl;
  printSymbols(cout, toObjList(nonterminals));

  derivable->print();
    
  // testing closure
  {  
    // make a singleton set out of the first production, and
    // with the dot at the start
    DProductionList itemSet;
    DottedProduction *kernel = productions.nth(0)->getDProd(0);  // (serf)
    itemSet.append(kernel);

    // compute its closure
    itemSetClosure(itemSet);

    // print it
    cout << "Closure of: ";
    kernel->print(cout);
    cout << endl;

    SFOREACH_DOTTEDPRODUCTION(itemSet, dprod) {
      cout << "  ";
      dprod.data()->print(cout);
      cout << endl;
    }
  }

  
  // LR stuff
  ObjList<ItemSet> itemSets;
  constructLRItemSets(itemSets);
  
  INTLOOP(i, 0,	(int)TABLESIZE(input)) {
    cout << "------ parsing: `" << input[i] << "' -------\n";
    lrParse(itemSets, input[i]);
  }

  // another analysis
  //computePredictiveParsingTable();

  // silence warnings
  //pretendUsed(a,b,c,d,e, S,A,B,C,D);
}


int main()
{
  GrammarAnalysis g;
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
