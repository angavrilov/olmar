// gramanl.cc
// code for gramanl.h

#include "gramanl.h"     // this module

#include "bit2d.h"       // Bit2d
#include "strtokp.h"     // StrtokParse
#include "syserr.h"      // xsyserror
#include "trace.h"       // tracing system
#include "nonport.h"     // getMilliseconds
#include "crc.h"         // crc32
#include "flatutil.h"    // Flatten, xfer helpers
#include "grampar.h"     // readGrammarFile
#include "emitcode.h"    // EmitCode
#include "strutil.h"     // replace

#include <fstream.h>     // ofstream


int itemDiff(DottedProduction const *left, DottedProduction const *right, void*);

// ----------------- ItemSet -------------------
ItemSet::ItemSet(int anId, int numTerms, int numNonterms)
  : kernelItems(),
    nonkernelItems(),
    termTransition(NULL),      // inited below
    nontermTransition(NULL),   // inited below
    terms(numTerms),
    nonterms(numNonterms),
    dotsAtEnd(NULL),
    numDotsAtEnd(0),
    id(anId),
    BFSparent(NULL)
{
  allocateTransitionFunction();
}

void ItemSet::allocateTransitionFunction()
{
  termTransition = new ItemSet* [terms];
  nontermTransition = new ItemSet* [nonterms];

  INTLOOP(t, 0, terms) {
    termTransition[t] = (ItemSet*)NULL;      // means no transition on t
  }
  INTLOOP(n, 0, nonterms) {
    nontermTransition[n] = (ItemSet*)NULL;
  }
}


ItemSet::~ItemSet()
{
  delete[] termTransition;
  delete[] nontermTransition;

  if (dotsAtEnd) {
    delete[] dotsAtEnd;
  }
}


ItemSet::ItemSet(Flatten &flat)
  : termTransition(NULL),
    nontermTransition(NULL),
    dotsAtEnd(NULL),
    numDotsAtEnd(0),
    BFSparent(NULL)
{}


Production *getNthProduction(Grammar *g, int n)
{
  if (0 <= n && n < g->productions.count()) {
    return g->productions.nth(n);
  }
  else {
    // my access path functions' contract is to
    // return NULL on any error (as opposed to, say,
    // an exception or assertion failure); this serves two
    // purposes:
    //   - the writing code can use it to determine the
    //     maximum value of 'n'
    //   - the reading code can use it to validate 'n',
    //     since that comes from the input file
    return NULL;
  }
}

DottedProduction *getNthDottedProduction(Production *p, int n)
{
  if (0 <= n && n < (p->rhsLength() + 1)) {
    return p->getDProd(n);
  }
  else {
    return NULL;
  }
}


void ItemSet::xfer(Flatten &flat)
{
  flat.xferInt(terms);
  flat.xferInt(nonterms);

  // numDotsAtEnd and kernelItemsCRC are computed from
  // other data
  // NEW: but computing them requires the items, which I'm omitting

  flat.xferInt(numDotsAtEnd);
  flat.xferLong((long&)kernelItemsCRC);

  flat.xferInt(id);
}


int ticksComputeNonkernel = 0;

void ItemSet::xferSerfs(Flatten &flat, GrammarAnalysis &g)
{
  #if 1
    // 'kernelItems' and 'nonkernelItems': each one accessed as
    //   g.productions.nth(???)->getDProd(???)
    xferSObjList_twoLevelAccess(
      flat,
      kernelItems,               // serf list
      static_cast<Grammar*>(&g), // root of access path
      getNthProduction,          // first access path link
      getNthDottedProduction);   // second access path link

    #if 1
      xferSObjList_twoLevelAccess(
        flat,
        nonkernelItems,            // serf list
        static_cast<Grammar*>(&g), // root of access path
        getNthProduction,          // first access path link
        getNthDottedProduction);   // second access path link
    #else
      // instead of the above, let's try computing the nonkernel items
      if (flat.reading()) {
        int start = getMilliseconds();
        g.itemSetClosure(*this);
        ticksComputeNonkernel += (getMilliseconds() - start);
      }
    #endif
  #endif // 0

  // these need to be sorted for 'changedItems'; but since
  // we're sorting by *address*, that's not necessarily
  // preserved across read/write
  // NEW: it should be stable now
  //kernelItems.insertionSort(itemDiff);


  // transition functions
  if (flat.reading()) {
    allocateTransitionFunction();
  }
  INTLOOP(t, 0, terms) {
    //xferNullableSerfPtrToList(flat, termTransition[t], g.itemSets);
    xferNullableSerfPtr(flat, termTransition[t]);
  }
  INTLOOP(n, 0, nonterms) {
    //xferNullableSerfPtrToList(flat, nontermTransition[n], g.itemSets);
    xferNullableSerfPtr(flat, nontermTransition[n]);
  }


  // dotsAtEnd, numDotsAtEnd, kernelItemsCRC
  //if (flat.reading()) {
  //  changedItems();
  //}

  if (flat.reading()) {
    dotsAtEnd = new DottedProduction const * [numDotsAtEnd];
  }
  INTLOOP(p, 0, numDotsAtEnd) {
    #if 0
    xferSerfPtr_twoLevelAccess(
      flat,
      const_cast<DottedProduction*&>(dotsAtEnd[p]),   // serf
      static_cast<Grammar*>(&g), // root of access path
      getNthProduction,          // first access path link
      getNthDottedProduction);   // second access path link
    #endif // 0
    xferSerfPtr(flat, dotsAtEnd[p]);
  }

  xferNullableSerfPtrToList(flat, BFSparent, g.itemSets);
}


Symbol const *ItemSet::getStateSymbolC() const
{
  // need only check kernel items since all nonkernel items
  // have their dots at the left side
  SFOREACH_DOTTEDPRODUCTION(kernelItems, item) {
    if (! item.data()->isDotAtStart() ) {
      return item.data()->symbolBeforeDotC();
    }
  }
  return NULL;
}


int ItemSet::bcheckTerm(int index)
{
  xassert(0 <= index && index < terms);
  return index;
}

int ItemSet::bcheckNonterm(int index)
{
  xassert(0 <= index && index < nonterms);
  return index;
}

ItemSet *&ItemSet::refTransition(Symbol const *sym)
{
  if (sym->isTerminal()) {
    Terminal const &t = sym->asTerminalC();
    return termTransition[bcheckTerm(t.termIndex)];
  }
  else {
    Nonterminal const &nt = sym->asNonterminalC();
    return nontermTransition[bcheckNonterm(nt.ntIndex)];
  }
}


ItemSet const *ItemSet::transitionC(Symbol const *sym) const
{
  return const_cast<ItemSet*>(this)->refTransition(sym);
}


void ItemSet::setTransition(Symbol const *sym, ItemSet *dest)
{
  refTransition(sym) = dest;
}

                                           
// arbitrary integer unique to every symbol and preserved
// across read/write
int symbolIndex(Symbol const *s)
{
  if (s->isTerminal()) {
    // make terminals negative since otherwise they'd
    // collide with nonterminals
    return -( s->asTerminalC().termIndex );
  }
  else {
    return s->asNonterminalC().ntIndex;
  }
}


// compare two items in an arbitrary (but deterministic) way so that
// sorting will always put a list of items into the same order, for
// comparison purposes
int itemDiff(DottedProduction const *a, DottedProduction const *b, void*)
{
  // since I don't make copies of dotted productions, we
  // can detect equality immediately
  if (a == b) {
    return 0;
  }

  // I want the sorting order to be preserved across read/write,
  // so I do a real compare now, not just address diff

  // 'dot'
  int ret = a->dot - b->dot;
  if (ret) { return ret; }

  Production const *aProd = a->prod;
  Production const *bProd = b->prod;

  // LHS index
  ret = aProd->left->ntIndex - bProd->left->ntIndex;
  if (ret) { return ret; }

  // RHS indices
  RHSEltListIter aIter(aProd->right);
  RHSEltListIter bIter(bProd->right);

  while (!aIter.isDone() && !bIter.isDone()) {
    ret = symbolIndex(aIter.data()->sym) - symbolIndex(bIter.data()->sym);
    if (ret) { return ret; }
    
    aIter.adv();
    bIter.adv();
  }

  if (aIter.isDone() && !bIter.isDone()) {
    return -1;
  }
  if (!aIter.isDone() && bIter.isDone()) {
    return 1;
  }

  cout << "a: "; a->print(cout);
  cout << "\nb: "; b->print(cout);
  cout << endl;

  // this can be caused if the grammar input file actually
  // has the same production listed twice
  xfailure("two dotted productions with diff addrs are equal!\n");
}


void ItemSet::addKernelItem(DottedProduction *item)
{
  // add it
  kernelItems.appendUnique(item);

  // sort the items to facilitate equality checks
  kernelItems.insertionSort(itemDiff);
  
  changedItems();
}


bool ItemSet::operator==(ItemSet const &obj) const
{                                          
  // since common case is disequality, check the 
  // CRCs first, and only do full check if they
  // match
  if (kernelItemsCRC == obj.kernelItemsCRC) {
    // since nonkernel items are entirely determined by kernel
    // items, and kernel items are sorted, it's sufficient to
    // check for kernel list equality
    return kernelItems.equalAsPointerLists(obj.kernelItems);
  }
  else {
    // can't possibly be equal if CRCs differ
    return false;
  }
}


void ItemSet::addNonkernelItem(DottedProduction *item)
{
  nonkernelItems.appendUnique(item);
  changedItems();
}


void ItemSet::getAllItems(DProductionList &dest) const
{
  dest = kernelItems;
  dest.appendAll(nonkernelItems);
}


void ItemSet::throwAwayItems()
{
  kernelItems.removeAll();
  nonkernelItems.removeAll();
}


// return the reductions that are ready in this state, given
// that the next symbol is 'lookahead'
void ItemSet::getPossibleReductions(ProductionList &reductions,
                                    Terminal const *lookahead,
                                    bool parsing) const
{
  // for each item with dot at end
  loopi(numDotsAtEnd) {
    DottedProduction const *item = dotsAtEnd[i];

    // the follow of its LHS must include 'lookahead'
    // NOTE: this is the difference between LR(0) and SLR(1) --
    //       LR(0) would not do this check, while SLR(1) does
    if (!item->prod->left->follow.contains(lookahead)) {    // (constness)
      if (parsing && tracingSys("parse")) {
	trace("parse") << "not reducing by " << *(item->prod)
       	       	       << " because `" << lookahead->name
		       << "' is not in follow of "
		       << item->prod->left->name << endl;
      }
      continue;
    }

    // ok, this one's ready
    reductions.append(item->prod);                          // (constness)
  }
}


void ItemSet::changedItems()
{
  // -- recompute dotsAtEnd --
  // collect all items
  DProductionList items;
  getAllItems(items);

  // count number with dots at end
  int count = 0;
  {
    SFOREACH_DOTTEDPRODUCTION(items, itemIter) {
      DottedProduction const *item = itemIter.data();

      if (item->isDotAtEnd()) {
        count++;
      }
    }
  }

  // get array of right size
  if (dotsAtEnd  &&  count == numDotsAtEnd) {
    // no need to reallocate, already correct size
  }
  else {
    // throw old away
    if (dotsAtEnd) {
      delete[] dotsAtEnd;
    }

    // allocate new array
    numDotsAtEnd = count;
    dotsAtEnd = new DottedProduction const * [numDotsAtEnd];
  }

  // fill array
  int index = 0;
  SFOREACH_DOTTEDPRODUCTION(items, itemIter) {
    DottedProduction const *item = itemIter.data();

    if (item->isDotAtEnd()) {
      dotsAtEnd[index] = item;
      index++;
    }
  }

  // verify both loops executed same number of times
  xassert(index == count);


  // -- compute CRC of kernel items --
  // put all pointers into a single buffer
  // (assumes they've already been sorted!)
  int numKernelItems = kernelItems.count();
  DottedProduction const **array = 
    new DottedProduction const * [numKernelItems];      // (owner ptr to array of serf ptrs)
  index = 0;
  SFOREACH_DOTTEDPRODUCTION(kernelItems, kitem) {
    array[index] = kitem.data();

    if (index > 0) {
      // may as well check sortedness and
      // uniqueness
      xassert(itemDiff(array[index], array[index-1], NULL) > 0);
    }

    index++;
  }

  // CRC the buffer
  kernelItemsCRC = crc32((unsigned char const*)array, 
                         sizeof(array[0]) * numKernelItems);

  // trash the array
  delete[] array;
}


void ItemSet::print(ostream &os, GrammarAnalysis const &g) const
{
  os << "ItemSet " << id << ":\n";

  // collect all items
  DProductionList items;
  getAllItems(items);

  // for each item
  SFOREACH_DOTTEDPRODUCTION(items, itemIter) {
    DottedProduction const *dprod = itemIter.data();

    // print its text
    os << "  ";
    dprod->print(os);
    os << "      ";

    // print any transitions on its after-dot symbol
    if (!dprod->isDotAtEnd()) {
      ItemSet const *is = transitionC(dprod->symbolAfterDotC());
      if (is == NULL) {
        os << "(no transition?!?!)";
      }
      else {
        os << "--> " << is->id;
      }
    }
    os << endl;
  }

  // print transition function directly, since I'm now throwing
  // away items sometimes
  for (int t=0; t<terms; t++) {
    if (termTransition[t]) {
      os << "  on terminal " << g.getTerminal(t)->name 
         << " go to " << termTransition[t]->id << endl;
    }
  }

  for (int n=0; n<nonterms; n++) {
    if (nontermTransition[n]) {
      os << "  on nonterminal " << g.getNonterminal(n)->name 
         << " go to " << nontermTransition[n]->id << endl;
    }
  }
  
  for (int p=0; p<numDotsAtEnd; p++) {
    os << "  can reduce by " << dotsAtEnd[p]->prod->toString() << endl;
  }
}


void ItemSet::writeGraph(ostream &os) const
{
  // node: n <name> <desc>
  os << "\nn ItemSet" << id << " ItemSet" << id << "/";
    // rest of desc will follow

  // collect all items
  DProductionList items;
  getAllItems(items);

  // for each item, print the item text
  SFOREACH_DOTTEDPRODUCTION(items, itemIter) {
    DottedProduction const *dprod = itemIter.data();

    // print its text
    os << "   ";
    dprod->print(os);
    os << "/";      // line separator in my node format
  }
  os << endl;

  // print transitions on terminals
  INTLOOP(t, 0, terms) {
    if (termTransition[t] != NULL) {
      os << "e ItemSet" << id
         << " ItemSet" << termTransition[t]->id << endl;
    }
  }

  // print transitions on nonterminals
  INTLOOP(nt, 0, nonterms) {
    if (nontermTransition[nt] != NULL) {
      os << "e ItemSet" << id
         << " ItemSet" << nontermTransition[nt]->id << endl;
    }
  }
}


// ------------------------ GrammarAnalysis --------------------
GrammarAnalysis::GrammarAnalysis()
  : derivable(NULL),
    indexedNonterms(NULL),
    indexedTerms(NULL),
    numNonterms(0),
    numTerms(0),
    productionsByLHS(NULL),
    initialized(false),
    nextItemSetId(0),    // [ASU] starts at 0 too
    startState(NULL),
    cyclic(false),
    symOfInterest(NULL)
{}


GrammarAnalysis::~GrammarAnalysis()
{
  if (indexedNonterms != NULL) {
    delete indexedNonterms;
  }

  if (indexedTerms != NULL) {
    delete indexedTerms;
  }

  if (productionsByLHS != NULL) {
    // empties all lists automatically because of "[]"
    delete[] productionsByLHS;
  }

  if (derivable != NULL) {
    delete derivable;
  }
}


Terminal const *GrammarAnalysis::getTerminal(int index) const
{
  xassert((unsigned)index < (unsigned)numTerms);
  return indexedTerms[index];
}

Nonterminal const *GrammarAnalysis::getNonterminal(int index) const
{
  xassert((unsigned)index < (unsigned)numNonterms);
  return indexedNonterms[index];
}


void GrammarAnalysis::xfer(Flatten &flat)
{
  Grammar::xfer(flat);

  xferOwnerPtr(flat, derivable);

  // delay indexed[Non]Terms, productionsByLHS,
  // and initialized

  flat.xferInt(nextItemSetId);

  xferObjList(flat, itemSets);
  xferSerfPtrToList(flat, startState, itemSets);

  flat.xferBool(cyclic);

  // don't bother xferring 'symOfInterest', since it's
  // only used for debugging

  // now do the easily-computable stuff
  computeIndexedNonterms();
  computeIndexedTerms();
  computeProductionsByLHS();

  // do serfs after because if I want to compute the
  // nonkernel items instead of storing them, I need
  // the indices
  MUTATE_EACH_OBJLIST(ItemSet, itemSets, iter) {
    iter.data()->xferSerfs(flat, *this);
  }

  flat.xferBool(initialized);
}


void GrammarAnalysis::
  printProductions(ostream &os, bool printCode) const
{
  if (cyclic) {
    os << "(cyclic!) ";
  }
  Grammar::printProductions(os, printCode);
}


void GrammarAnalysis::
  printProductionsAndItems(ostream &os, bool printCode) const
{
  printProductions(os, printCode);

  FOREACH_OBJLIST(ItemSet, itemSets, iter) {
    iter.data()->print(os, *this);
  }
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
      trace("derivable")
        << "discovered that " << NT->name << " ->+ "
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


bool GrammarAnalysis::sequenceCanDeriveEmpty(RHSEltList const &list) const
{
  RHSEltListIter iter(list);
  return iterSeqCanDeriveEmpty(iter);
}

bool GrammarAnalysis::iterSeqCanDeriveEmpty(RHSEltListIter iter) const
{
  // look through the sequence beginning with 'iter'; if any members cannot
  // derive emptyString, fail
  for (; !iter.isDone(); iter.adv()) {
    if (iter.data()->sym->isTerminal()) {
      return false;    // terminals can't derive emptyString
    }

    if (!canDeriveEmpty(&( iter.data()->sym->asNonterminalC() ))) {
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
void GrammarAnalysis::computeIndexedNonterms()
{
  // map: ntIndex -> Nonterminal*
  numNonterms = numNonterminals();
  indexedNonterms = new Nonterminal* [numNonterms];

  // fill it
  indexedNonterms[emptyStringIndex] = &emptyString;
  int index = emptyStringIndex;
  emptyString.ntIndex = index++;

  for (ObjListMutator<Nonterminal> sym(nonterminals);
       !sym.isDone(); index++, sym.adv()) {
    indexedNonterms[index] = sym.data();    // map: index to symbol
    sym.data()->ntIndex = index;            // map: symbol to index
  }
}


void GrammarAnalysis::computeIndexedTerms()
{
  // map: termIndex -> Terminal*
  // the ids have already been assigned; but I'm going to continue
  // to insist on a contiguous space starting at 0
  numTerms = numTerminals();
  indexedTerms = new Terminal* [numTerms];
  loopi(numTerminals()) {
    indexedTerms[i] = NULL;      // used to track id duplication
  }
  for (ObjListMutator<Terminal> sym(terminals);
       !sym.isDone(); sym.adv()) {
    int index = sym.data()->termIndex;   // map: symbol to index
    if (indexedTerms[index] != NULL) {
      xfailure(stringc << "terminal index collision at index " << index);
    }
    indexedTerms[index] = sym.data();    // map: index to symbol
  }
}


void GrammarAnalysis::computeProductionsByLHS()
{
  // map: nonterminal -> productions with that nonterm on LHS
  productionsByLHS = new SObjList<Production> [numNonterms];
  {
    MUTATE_EACH_PRODUCTION(productions, prod) {        // (constness)
      int LHSindex = prod.data()->left->ntIndex;
      xassert(LHSindex < numNonterms);

      productionsByLHS[LHSindex].append(prod.data());
    }
  }
}


void GrammarAnalysis::initializeAuxData()
{
  // at the moment, calling this twice leaks memory
  xassert(!initialized);

  computeIndexedNonterms();
  computeIndexedTerms();

  computeProductionsByLHS();

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

      // since I don't include 'empty' explicitly in my rules, I won't
      // conclude that anything can derive empty, which is a problem;
      // so I special-case it here
      if (prod->right.isEmpty()) {
	addDerivable(prod->left, &emptyString);
        continue;      	// no point in looping over RHS symbols since there are none
      }

      // iterate over RHS symbols, seeing if the LHS can derive that
      // RHS symbol (by itself)
      for (RHSEltListIter rightSym(prod->right);
           !rightSym.isDone(); rightSym.adv()) {

        if (rightSym.data()->sym->isTerminal()) {
          // if prod->left derives a string containing a terminal,
          // then it can't derive any nontermial alone (using this
          // production, at least) -- empty is considered a nonterminal
          break;
        }

        // otherwise, it's a nonterminal
        Nonterminal const &rightNT = rightSym.data()->sym->asNonterminalC();

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
          RHSEltListIter afterRightSym(rightSym);
          bool restDeriveEmpty = true;
          for (afterRightSym.adv();    // *after* right symbol
               !afterRightSym.isDone(); afterRightSym.adv()) {

            if (afterRightSym.data()->sym->isTerminal()  ||
                  // if it's a terminal, it can't derive emptyString
                !canDeriveEmpty(&( afterRightSym.data()->sym->asNonterminalC() ))) {
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

            trace("derivable") 
              << "discovered (by production): " << prod->left->name
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
            trace("derivable") 
              << "discovered (by closure step): "
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
void GrammarAnalysis::firstOfSequence(TerminalList &destList, 
                                      RHSEltList &sequence)
{
  RHSEltListMutator iter(sequence);
  firstOfIterSeq(destList, iter);
}

// similar to above, 'sym' needs to be a mutator
void GrammarAnalysis::firstOfIterSeq(TerminalList &destList,
                                     RHSEltListMutator sym)
{
  int numTerms = numTerminals();     // loop invariant

  // for each sequence member such that all
  // preceeding members can derive emptyString
  for (; !sym.isDone(); sym.adv()) {
    // LHS -> x alpha   means x is in First(LHS)
    if (sym.data()->sym->isTerminal()) {
      destList.append(&( sym.data()->sym->asTerminal() ));
      break;    // stop considering RHS members since a terminal
                // effectively "hides" all further symbols from First
    }

    // sym must be a nonterminal
    Nonterminal const &nt = sym.data()->sym->asNonterminalC();

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
      MUTATE_EACH_OBJLIST(Production::RHSElt, prod->right, rightSym) {
        if (rightSym.data()->sym->isTerminal()) continue;

        // convenient alias
        Nonterminal &rightNT = rightSym.data()->sym->asNonterminal();

        // I'm not sure what it means to compute Follow(emptyString),
        // so let's just not do so
        if (&rightNT == &emptyString) {
          continue;
        }

        // an iterator pointing to the symbol just after
        // 'rightSym' will be useful below
        RHSEltListMutator afterRightSym(rightSym);
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
            if (addFollow(&rightNT, term.data())) {
              changes++;
              if (&rightNT == symOfInterest) {
                trace("follow-sym")
                  << "Follow(" << rightNT.name
                  << "): adding " << term.data()->name
                  << " by first(RHS-tail) of " << *prod
                  << endl;
              }
            }
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
              if (addFollow(&rightNT, indexedTerms[t])) {
                changes++;
                if (&rightNT == symOfInterest) {
                  trace("follow-sym")
                    << "Follow(" << rightNT.name
                    << "): adding " << indexedTerms[t]->name
                    << " by follow(LHS) of " << *prod
                    << endl;
                }
              }
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


  // print the resulting table
  ostream &os = trace("pred-table") << endl;

  // for each nonterminal
  INTLOOP(nonterm, 0, numNonterms) {
    os << "Row " << indexedNonterms[nonterm]->name << ":\n";

    // for each terminal
    INTLOOP(term, 0, numTerms) {
      os << "  Column " << indexedTerms[term]->name << ":";

      // for each production in table[nonterm,term]
      SFOREACH_PRODUCTION(TABLE(nonterm,term), prod) {
        os << "   ";
        prod.data()->print(os);
      }

      os << endl;
    }
  }

  // cleanup
  #undef TABLE
  delete[] table;
}


// [ASU] figure 4.33, p.223
void GrammarAnalysis::itemSetClosure(ItemSet &itemSet)
{
  // while no changes
  int changes = 1;
  while (changes > 0) {
    changes = 0;

    // a place to store the items we plan to add
    DProductionList newItems;

    // grab the current set of items
    DProductionList items;
    itemSet.getAllItems(items);

    // for each item A -> alpha . B beta in itemSet
    SFOREACH_DOTTEDPRODUCTION(items, itemIter) {            // (constness ok)
      DottedProduction const *item = itemIter.data();

      // get the symbol B (the one right after the dot)
      if (item->isDotAtEnd()) continue;
      Symbol const *B = item->symbolAfterDotC();
      if (B->isTerminal()) continue;
      int nontermIndex = B->asNonterminalC().ntIndex;

      // for each production B -> gamma
      SMUTATE_EACH_PRODUCTION(productionsByLHS[nontermIndex], prod) {           // (constness)
        // invariant of the indexed productions list
        xassert(prod.data()->left == B);

        // plan to add B -> . gamma to the itemSet, if not already there
        DottedProduction *dp = prod.data()->getDProd(0 /*dot placement*/);     // (constness)
        if (!items.contains(dp)) {
          newItems.append(dp);
        }
      } // for each production
    } // for each item

    // add the new items (we don't do this while iterating because my
    // iterator interface says not to, even though it would work with
    // my current implementation)
    SMUTATE_EACH_DOTTEDPRODUCTION(newItems, item) {
      itemSet.addNonkernelItem(item.data());	            // (constness)
      changes++;
    }
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
// in 'source' that have 'symbol' to the right of the dot; do *not*
// compute the closure
ItemSet *GrammarAnalysis::moveDotNoClosure(ItemSet const *source, Symbol const *symbol)
{
  ItemSet *ret = makeItemSet();

  DProductionList items;
  source->getAllItems(items);

  // for each item
  int appendCt=0;
  SFOREACH_DOTTEDPRODUCTION(items, dprodi) {
    DottedProduction const *dprod = dprodi.data();

    if (dprod->isDotAtEnd() ||
        dprod->symbolAfterDotC() != symbol) {
      continue;    // can't move dot
    }

    // move the dot
    DottedProduction *dotMoved =
      dprod->prod->getDProd(dprod->dot + 1);      // (constness!)

    // add the new item to the itemset I'm building
    ret->addKernelItem(dotMoved);
    appendCt++;
  }

  // for now, verify we actually got something; though it would
  // be easy to simply return null (after dealloc'ing ret)
  xassert(appendCt > 0);

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


STATICDEF bool GrammarAnalysis::itemSetsEqual(ItemSet const *is1, ItemSet const *is2)
{
  return *is1 == *is2;
}


// [ASU] fig 4.34, p.224
// puts the finished parse tables into 'itemSetsDone'
void GrammarAnalysis::constructLRItemSets()
{
  // sets of item sets
  ObjList<ItemSet> itemSetsPending;            // yet to be processed

  ObjList<ItemSet> &itemSetsDone = itemSets;   // all outgoing links processed
    // (renamed-by-reference to make role clearer here)

  // start by constructing closure of first production
  // (basically assumes first production has start symbol
  // on LHS, and no other productions have the start symbol
  // on LHS)
  {
    ItemSet *is = makeItemSet();              // (owner)
    startState = is;
    is->addKernelItem(productions.nth(0)->    // first production's ..
                        getDProd(0));         //   .. first dot placement
    itemSetClosure(*is);

    // this makes the initial pending itemSet
    itemSetsPending.append(is);              // (ownership transfer)
  }

  // for each pending item set
  while (!itemSetsPending.isEmpty()) {
    ItemSet *itemSet = itemSetsPending.removeAt(0);    // dequeue   (owner; ownership transfer)

    // put it in the done set; note that we must do this *before*
    // the processing below, to properly handle self-loops
    itemSetsDone.append(itemSet);                      // (ownership transfer; 'itemSet' becomes serf)

    DProductionList items;
    itemSet->getAllItems(items);

    // for each production in the item set where the
    // dot is not at the right end
    SFOREACH_DOTTEDPRODUCTION(items, dprodIter) {
      DottedProduction const *dprod = dprodIter.data();
      if (dprod->isDotAtEnd()) continue;

      // get the symbol 'sym' after the dot (next to be shifted)
      Symbol const *sym = dprod->symbolAfterDotC();

      // if we already have a transition for this symbol,
      // there's nothing more to be done
      if (itemSet->transitionC(sym) != NULL) {
        continue;
      }

      // fixed: adding transition functions fixes this:
	// inefficiency: if several productions have X to the
	// left of the dot, then we will 'moveDot' each time

      // compute the itemSet produced by moving the dot
      // across 'sym'; but don't take closure yet since
      // we first want to check whether it is already present
      ItemSet *withDotMoved = moveDotNoClosure(itemSet, sym);

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
        // we don't already have it; finish it by computing its closure
        itemSetClosure(*withDotMoved);

        // then add it to 'pending'
        itemSetsPending.append(withDotMoved);
      }

      // setup the transition function
      itemSet->setTransition(sym, withDotMoved);

    } // for each item
  } // for each item set


  // do the BFS now, since we want to print the sample inputs
  // in the loop that follows
  traceProgress(2) << "BFS tree on transition graph...\n";
  computeBFSTree();


  if (tracingSys("item-sets")) {
    // print each item set
    FOREACH_OBJLIST(ItemSet, itemSetsDone, itemSet) {
      ostream &os = trace("item-sets")
        << "State " << itemSet.data()->id
        << ", sample input: " << sampleInput(itemSet.data())
        << endl
        << "  and left context: " << leftContextString(itemSet.data())
        << endl
        ;

      itemSet.data()->print(os, *this);
    }
  }


  // write this info to a graph applet file
  ofstream out("lrsets.g");
  if (!out) {
    xsyserror("ofstream open");
  }
  out << "# lr sets in graph form\n";

  FOREACH_OBJLIST(ItemSet, itemSetsDone, itemSet) {
    itemSet.data()->writeGraph(out);
  }
}

// --------------- END of construct LR item sets -------------------


Symbol const *GrammarAnalysis::
  inverseTransitionC(ItemSet const *source, ItemSet const *target) const
{
  // for each symbol..
  FOREACH_TERMINAL(terminals, t) {
    // see if it is the one
    if (source->transitionC(t.data()) == target) {
      return t.data();
    }
  }

  FOREACH_NONTERMINAL(nonterminals, nt) {
    if (source->transitionC(nt.data()) == target) {
      return nt.data();
    }
  }

  xfailure("GrammarAnalysis::inverseTransitionC: no transition from source to target");
  return NULL;     // silence warning
}


// --------------- LR support -------------------
// find and print all the conflicts that would be reported by
// an SLR(1) parser; this is a superset of the conflicts reported
// by bison, which is LALR(1); found conflicts are printed with
// trace("conflict")
void GrammarAnalysis::findSLRConflicts() const
{
  // for every item set..
  FOREACH_OBJLIST(ItemSet, itemSets, itemSet) {
  
    // we want to print something special for the first conflict
    // in a state, so track which is first
    bool conflictAlready = false;

    // for every input symbol..
    FOREACH_TERMINAL(terminals, t) {
      // check it
      bool con = checkSLRConflicts(itemSet.data(), t.data(), conflictAlready);
      conflictAlready = conflictAlready || con;
    }
  }
}


// given a parser state and an input symbol determine if we will fork
// the parse stack; return true if there is a conflict
bool GrammarAnalysis::checkSLRConflicts(ItemSet const *state, Terminal const *sym,
                                        bool conflictAlready) const
{
  // see where a shift would go
  ItemSet const *shiftDest = state->transitionC(sym);

  // get all possible reductions where 'sym' is in Follow(LHS)
  ProductionList reductions;
  state->getPossibleReductions(reductions, sym, false /*parsing*/);

  // case analysis
  if (shiftDest != NULL &&
      reductions.isEmpty()) {
    // unambiguous shift; ok
  }

  else if (shiftDest == NULL &&
           reductions.count() == 1) {
    // unambiguous reduction; ok
  }

  else if (shiftDest == NULL &&
           reductions.isEmpty()) {
    // no transition: syntax error
    // presumably it's really an input error, not a grammar issue,
    // so I'd rather not see these
  }

  else {
    // local ambiguity
    if (!conflictAlready) {
      trace("conflict")
        << "--------- state " << state->id << " ----------\n"
        << "left context: " << leftContextString(state)
        << endl
        << "sample input: " << sampleInput(state)
        << endl
        ;
    }

    trace("conflict")
      << "conflict for symbol " << sym->name
      << endl;

    if (shiftDest) {
      trace("conflict") << "  shift, and move to state " << shiftDest->id << endl;
    }

    SFOREACH_PRODUCTION(reductions, prod) {
      trace("conflict") << "  reduce by rule " << *(prod.data()) << endl;
    }

    return true;    // found conflict
  }
  
  return false;     // no conflict
}


// given an LR transition graph, compute the BFS tree on top of it
// and set the parent links to record the tree
void GrammarAnalysis::computeBFSTree()
{
  // for the BFS, we need a queue of states yet to be processed, and a
  // pile of 'done' states
  SObjList<ItemSet> queue;
  SObjList<ItemSet> done;

  // initial entry in queue is root of BFS tree
  queue.append(startState);

  // it will be convenient to have all the symbols in a single list
  // for iteration purposes
  SymbolList allSymbols;       	  // (const list)
  {
    FOREACH_TERMINAL(terminals, t) {
      allSymbols.append(const_cast<Terminal*>(t.data()));
    }
    FOREACH_NONTERMINAL(nonterminals, nt) {
      allSymbols.append(const_cast<Nonterminal*>(nt.data()));
    }
  }

  // loop until the queue is exhausted
  while (queue.isNotEmpty()) {
    // dequeue first element
    ItemSet *source = queue.removeAt(0);

    // mark it as done so we won't consider any more transitions to it
    done.append(source);

    // for each symbol...
    SFOREACH_SYMBOL(allSymbols, sym) {
      // get the transition on this symbol
      ItemSet *target = source->transition(sym.data());

      // if the target is done or already enqueued, or there is no
      // transition on this symbol, we don't need to consider it
      // further
      if (target == NULL ||
          done.contains(target) || 
          queue.contains(target)) {
        continue;
      }

      // the source->target link just examined is the first time
      // we've encounted 'target', so that link becomes the BFS
      // parent link
      target->BFSparent = source;

      // finally, enqueue the target so we'll explore its targets too
      queue.append(target);
    }
  }
}

// --------------- END of LR support -------------------


// --------------- sample inputs -------------------
// yield a sequence of names of symbols (terminals and nonterminals) that
// will lead to the given state, from the start state
string GrammarAnalysis::leftContextString(ItemSet const *state) const
{
  SymbolList ctx;
  leftContext(ctx, state);                // get as list
  return symbolSequenceToString(ctx);	  // convert to string
}


// yield the left-context as a sequence of symbols
// CONSTNESS: want output as list of const pointers
void GrammarAnalysis::leftContext(SymbolList &output,
                                  ItemSet const *state) const
{
  // since we have the BFS tree, generating sample input (at least, if
  // it's allowed to contain nonterminals) is a simple matter of walking
  // the tree towards the root

  // for each parent..
  while (state->BFSparent) {
    // get that parent
    ItemSet *parent = state->BFSparent;

    // find a symbol on which we would transition from the parent
    // to the current state
    Symbol const *sym = inverseTransitionC(parent, state);

    // prepend that symbol's name to our current context
    output.prepend(const_cast<Symbol*>(sym));

    // move to our parent and repeat
    state = parent;
  }
}


// compare two-element quantities where one dominates and the other
// is only for tie-breaking; return true if a's quantities are fewer
// (candidate for adding to a library somewhere)
int priorityFewer(int a_dominant, int b_dominant,
                  int a_recessive, int b_recessive)
{
  return (a_dominant < b_dominant) ||
       	 ((a_dominant == b_dominant) && (a_recessive < b_recessive));
}


// sample input (terminals only) that can lead to a state
string GrammarAnalysis::sampleInput(ItemSet const *state) const
{
  // get left-context as terminals and nonterminals
  SymbolList symbols;
  leftContext(symbols, state);

  // reduce the nonterminals to terminals
  TerminalList terminals;
  if (!rewriteAsTerminals(terminals, symbols)) {
    return string("(failed to reduce!!)");
  }
  
  // convert to a string
  return terminalSequenceToString(terminals);
}


// given a sequence of symbols (terminals and nonterminals), use the
// productions to rewrite it as a (hopefully minimal) sequence of
// terminals only; return true if it works, false if we get stuck
// in an infinite loop
// CONSTNESS: ideally, 'output' would contain const ptrs to terminals
bool GrammarAnalysis::rewriteAsTerminals(TerminalList &output, SymbolList const &input) const
{
  // we detect looping by noticing if we ever reduce the same
  // nonterminal more than once in a single vertical recursive slice
  // (which is necessary and sufficient for looping, since we have a
  // context-*free* grammar)
  NonterminalList reducedStack;      // starts empty

  // start the recursive version
  return rewriteAsTerminalsHelper(output, input, reducedStack);
}


// (nonterminals and terminals) -> terminals
bool GrammarAnalysis::
  rewriteAsTerminalsHelper(TerminalList &output, SymbolList const &input,
                           NonterminalList &reducedStack) const
{
  // walk down the input list, creating the output list by copying
  // terminals and reducing nonterminals
  SFOREACH_SYMBOL(input, symIter) {
    Symbol const *sym = symIter.data();

    if (sym->isEmptyString) {
      // easy; no-op
    }

    else if (sym->isTerminal()) {
      // no sweat, just copy it (er, copy the pointer)
      output.append(const_cast<Terminal*>(&sym->asTerminalC()));
    }

    else {
      // not too bad either, just reduce it, sticking the result
      // directly into our output list
      if (!rewriteSingleNTAsTerminals(output, &sym->asNonterminalC(),
                                      reducedStack)) {
        // oops..
        return false;
      }
    }
  }

  // ok!
  return true;
}


// nonterminal -> terminals
// CONSTNESS: want 'reducedStack' to be list of const ptrs
bool GrammarAnalysis::
  rewriteSingleNTAsTerminals(TerminalList &output, Nonterminal const *nonterminal,
                             NonterminalList &reducedStack) const
{
  // have I already reduced this?
  if (reducedStack.contains(nonterminal)) {
    // we'd loop if we continued
    trace("rewrite") << "aborting rewrite of " << nonterminal->name
                     << " because of looping\n";
    return false;
  }

  // add myself to the stack
  reducedStack.prepend(const_cast<Nonterminal*>(nonterminal));

  // look for best rule to use
  Production const *best = NULL;

  // for each rule with 'nonterminal' on LHS ...
  FOREACH_PRODUCTION(productions, prodIter) {
    Production const *prod = prodIter.data();
    if (prod->left != nonterminal) continue;

    // if 'prod' has 'nonterminal' on RHS, that would certainly
    // lead to looping (though it's not the only way -- consider
    // mutual recursion), so don't even consider it
    if (prod->rhsHasSymbol(nonterminal)) {
      continue;
    }

    // no champ yet?
    if (best == NULL) {
      best = prod;
      continue;
    }

    // compare new guy to existing champ
    if (priorityFewer(prod->numRHSNonterminals(), best->numRHSNonterminals(),
		      prod->rhsLength(), best->rhsLength())) {
      // 'prod' is better
      best = prod;
    }
  }

  // I don't expect this... either the NT doesn't have any rules,
  // or all of them are recursive (which means the language doesn't
  // have any finite sentences)
  if (best == NULL) {
    trace("rewrite") << "couldn't find suitable rule to reduce "
		     << nonterminal->name << "!!\n";
    return false;
  }

  // now, the chosen rule provides a RHS, which is a sequence of
  // terminals and nonterminals; recursively reduce that sequence
  SymbolList bestRHS;
  best->getRHSSymbols(bestRHS);
  bool retval = rewriteAsTerminalsHelper(output, bestRHS, reducedStack);

  // remove myself from stack
  Nonterminal *temp = reducedStack.removeAt(0);
  xassert((temp == nonterminal) || !retval);
    // make sure pushes are paired with pops properly (unless we're just
    // bailing out of a failed attempt, in which case some things might
    // not get popped)

  // and we succeed only if the recursive call succeeded
  return retval;
}

// --------------- END of sample inputs -------------------


// this is mostly [ASU] algorithm 4.7, p.218-219: an SLR(1) parser
// however, I've modified it to store one less item on the state stack
// (not really as optimization, just it made more sense to me that
// way)
void GrammarAnalysis::lrParse(char const *input)
{
  // tokenize the input
  StrtokParse tok(input, " \t");

  // parser state
  int currentToken = 0;               // index of current token
  ItemSet *state = startState;        // current parser state
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
    state->getPossibleReductions(reductions, symbol, true /*parsing*/);

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
      trace("parse")
        << "moving to state " << state->id
        << " after shifting symbol " << symbol->name << endl;
    }

    else if (shiftDest == NULL &&
             reductions.count() == 1) {    // unambiguous reduction
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
      trace("parse")
        << "moving to state " << state->id
        << " after reducing by rule " << *prod << endl;
    }

    else if (shiftDest == NULL &&
             reductions.isEmpty()) {       // no transition: syntax error
      trace("parse")
        << "no actions defined for symbol " << symbol->name
        << " in state " << state->id << endl;
      break;       // stop parsing
    }

    else {                                 // local ambiguity
      trace("parse")
        << "conflict for symbol " << symbol->name
        << " in state " << state->id
        << "; possible actions:\n";

      if (shiftDest) {
        trace("parse") << "  shift, and move to state " << shiftDest->id << endl;
      }

      SFOREACH_PRODUCTION(reductions, prod) {
        trace("parse") << "  reduce by rule " << *(prod.data()) << endl;
      }

      break;       // stop parsing
    }
  }

  // print final contents of stack; if the parse was successful,
  // I want to see what remains; if not, it's interesting anyway
  trace("parse") << "final contents of stacks (right is top):\n";
  stateStack.reverse();    // more convenient for printing
  symbolStack.reverse();

  ostream &os = trace("parse") << "  state stack:";
  SFOREACH_OBJLIST(ItemSet, stateStack, stateIter) {
    os << " " << stateIter.data()->id;
  }
  os << " current=" << state->id;   // print current state too
  os << endl;

  trace("parse") << "  symbol stack:";
  SFOREACH_SYMBOL(symbolStack, sym) {
    os << " " << sym.data()->name;
  }
  os << endl;
}


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
    #if 0
      // [ASU] grammar 4.19, p.222: demonstrating LR sets-of-items construction
      parseLine("E' ->  E $                ");
      parseLine("E  ->  E + T  |  T        ");
      parseLine("T  ->  T * F  |  F        ");
      parseLine("F  ->  ( E )  |  id       ");
    #else
      // get that grammar from a file instead
      readGrammarFile(*this, "asu419.gr");
    #endif

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
  printProductions(trace("grammar") << endl);


  // run analyses
  runAnalyses();


  // do some test parses
  INTLOOP(i, 0, (int)TABLESIZE(input)) {
    trace("parse") << "------ parsing: `" << input[i] << "' -------\n";
    lrParse(input[i]);
  }
}


void GrammarAnalysis::runAnalyses()
{
  checkWellFormed();

  // precomputations
  traceProgress(2) << "init...\n";
  initializeAuxData();

  traceProgress(2) << "derivability relation...\n";
  computeWhatCanDeriveWhat();

  traceProgress(2) << "first...\n";
  computeFirst();

  traceProgress(2) << "follow...\n";
  computeFollow();

  // print results
  {
    ostream &tracer = trace("terminals") << "Terminals:\n";
    printSymbols(tracer, toObjList(terminals));
  }
  {
    ostream &tracer = trace("nonterminals") << "Nonterminals:\n";
    tracer << "  " << emptyString << endl;
    printSymbols(tracer, toObjList(nonterminals));
  }

  if (tracingSys("derivable")) {
    derivable->print();
  }

  // testing closure
  #if 0
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
  #endif // 0


  // LR stuff
  traceProgress(2) << "LR item sets...\n";
  constructLRItemSets();

  traceProgress(2) << "SLR conflicts...\n";
  findSLRConflicts();


  // if we want to print, do so before throwing away the items
  if (tracingSys("itemsets")) {
    printProductionsAndItems(cout, true /*code*/);
  }

  // experiment: do I need the itemSet items during parsing?
  #if 1
  //cout << "throwing away items\n";
  MUTATE_EACH_OBJLIST(ItemSet, itemSets, iter) {
    iter.data()->throwAwayItems();
  }
  #endif // 0


  // another analysis
  //computePredictiveParsingTable();

  // silence warnings
  //pretendUsed(a,b,c,d,e, S,A,B,C,D);
}


// ------------------ emitting action code -----------------------
// prototypes for this section
void emitActionCode(Grammar &g, char const *fname, char const *srcFname);
void emitActions(Grammar &g, EmitCode &out);
void emitUserCode(EmitCode &out, LocString const &code);
void emitDupDelMerge(Grammar &g, EmitCode &out);
void emitDDMInlines(EmitCode &out, Symbol const &sym, bool mergeOk);
void emitSwitchCode(EmitCode &out, char const *signature, char const *switchVar,
                    ObjList<Symbol> const &syms, int whichFunc,
                    char const *templateCode, char const *actUpon);


// yield the name of the inline function for this production; naming
// design motivated by desire to make debugging easier
string actionFuncName(Production const &prod)
{
  return stringc << "action" << prod.prodIndex
                 << "_" << prod.left->name;
}


// emit the user's action code to a file
void emitActionCode(Grammar &g, char const *fname, char const *srcFname)
{
  EmitCode out(fname);
  if (!out) {
    throw_XOpen(fname);
  }

  out << "// " << fname << "\n";
  out << "// *** DO NOT EDIT BY HAND ***\n";
  out << "// automatically generated by gramanl, from " << srcFname << "\n";
  out << "\n";
  out << "#include <assert.h>      // assert\n";
  out << "#include \"useract.h\"     // SemanticValue\n";
  out << "\n";

  // insert user's verbatim code at top
  if (g.verbatim[0]) {
    out << lineDirective(g.verbatim);
    out << g.verbatim << "\n";
    out << restoreLine;
    out << "\n";
  }

  out << "// ------------------- actions ---------------\n";
  emitActions(g, out);
  out << "\n";
  out << "\n";

  emitDupDelMerge(g, out);
}


void emitActions(Grammar &g, EmitCode &out)
{
  // iterate over productions, emitting inline action functions
  {FOREACH_OBJLIST(Production, g.productions, iter) {
    Production const &prod = *(iter.data());

    // there's no syntax for a typeless nonterminal, so this shouldn't
    // be triggerable by the user
    xassert(prod.left->type);

    // put the production in comments above the defn
    out << "// " << prod.toString() << "\n";

    out << "static inline " << prod.left->type << " "
        << actionFuncName(prod) << "(";

    // iterate over RHS elements, emitting formals for each with a tag
    int ct=0;
    FOREACH_OBJLIST(Production::RHSElt, prod.right, rhsIter) {
      Production::RHSElt const &elt = *(rhsIter.data());
      if (elt.tag.length() == 0) continue;

      if (ct++ > 0) {
        out << ", ";
      }

      if (!elt.sym->type) {
        cout << "Production with action code at " << prod.action.locString()
             << " has tag `" << elt.tag << "' on a symbol with no type.\n";
        out << "__error_no_type__";     // will make compiler complain
      }
      else {
        out << elt.sym->type;
      }

      // the tag becomes the formal parameter's name
      out << " " << elt.tag;
    }

    out << ")\n";
    out << "{\n";

    // now insert the user's code, to execute in this environment of
    // properly-typed semantic values; the final brace is on the same
    // line so errors reported at the last brace go to user code
    out << lineDirective(prod.action);
    out << prod.action << " }\n";

    out << restoreLine;
    out << "\n";
  }}

  out << "\n";

  // main action function; calls the inline functions emitted above
  out << "SemanticValue doReductionAction(int productionId, SemanticValue *semanticValues)\n";
  out << "{\n";
  out << "  switch (productionId) {\n";

  // iterate over productions
  FOREACH_OBJLIST(Production, g.productions, iter) {
    Production const &prod = *(iter.data());

    out << "    case " << prod.prodIndex << ":\n";
    out << "      return (SemanticValue)" << actionFuncName(prod) << "(";

    // iterate over RHS elements, emitting arguments for each with a tag
    int index = -1;    // index into 'semanticValues'
    int ct=0;
    FOREACH_OBJLIST(Production::RHSElt, prod.right, rhsIter) {
      Production::RHSElt const &elt = *(rhsIter.data());

      // we have semantic values in the array for all RHS elements,
      // even if they didn't get a tag
      index++;

      if (elt.tag.length() == 0) continue;

      if (ct++ > 0) {
        out << ", ";
      }

      // cast SemanticValue to proper type
      out << "(" << elt.sym->type << ")(semanticValues[" << index << "])";
    }

    out << ");\n";
  }

  out << "    default:\n";
  out << "      assert(!\"invalid production code\");\n";
  out << "      return (SemanticValue)0;   // silence warning\n";
  out << "  }\n";
  out << "}\n";
}


void emitUserCode(EmitCode &out, LocString const &code)
{
  out << "{\n";
  out << lineDirective(code);
  out << code << " }\n";

  out << restoreLine;
  out << "\n";
}


void emitDupDelMerge(Grammar &g, EmitCode &out)
{
  out << "// ---------------- dup/del/merge nonterminals ---------------\n";
  // emit inlines for dup/del/merge of nonterminals
  FOREACH_OBJLIST(Nonterminal, g.nonterminals, ntIter) {
    emitDDMInlines(out, *(ntIter.data()), true /*mergeOk*/);
  }

  // emit dup-nonterm
  emitSwitchCode(out,
    "SemanticValue duplicateNontermValue(int nontermId, SemanticValue sval)",
    "nontermId",
    (ObjList<Symbol> const&)g.nonterminals,
    0 /*dupCode*/,
    "      return (SemanticValue)dup_$symName(($symType)sval);\n",
    "duplicate nonterm");

  // emit del-nonterm
  emitSwitchCode(out,
    "void deallocateNontermValue(int nontermId, SemanticValue sval)",
    "nontermId",
    (ObjList<Symbol> const&)g.nonterminals,
    1 /*delCode*/,
    "      del_$symName(($symType)sval);\n"
    "      return;\n",
    "deallocate nonterm");

  // emit merge-nonterm
  emitSwitchCode(out,
    "SemanticValue mergeAlternativeParses(int nontermId, SemanticValue left,\n"
    "                                     SemanticValue right)",
    "nontermId",
    (ObjList<Symbol> const&)g.nonterminals,
    2 /*mergeCode*/,
    "      return (SemanticValue)merge_$symName(($symType)left, ($symType)right);\n",
    "merge nonterm");
  out << "\n";


  out << "// ---------------- dup/del terminals ---------------\n";
  // emit inlines for dup/del of terminals
  FOREACH_OBJLIST(Terminal, g.terminals, termIter) {
    emitDDMInlines(out, *(termIter.data()), false /*mergeOk*/);
  }

  // emit dup-term
  emitSwitchCode(out,
    "SemanticValue duplicateTerminalValue(int termId, SemanticValue sval)",
    "termId",
    (ObjList<Symbol> const&)g.terminals,
    0 /*dupCode*/,
    "      return (SemanticValue)dup_$symName(($symType)sval);\n",
    "duplicate terminal");

  // emit del-term
  emitSwitchCode(out,
    "void deallocateTerminalValue(int termId, SemanticValue sval)",
    "termId",
    (ObjList<Symbol> const&)g.terminals,
    1 /*delCode*/,
    "      del_$symName(($symType)sval);\n"
    "      return;\n",
    "deallocate terminal");
}


void emitDDMInlines(EmitCode &out, Symbol const &sym, bool mergeOk)
{
  if (sym.ddm.dupCode) {
    out << "static inline " << sym.type << "  dup_" << sym.name
        << "(" << sym.type << " " << sym.ddm.dupParam << ") ";
    emitUserCode(out, sym.ddm.dupCode);
  }

  if (sym.ddm.delCode) {
    out << "static inline void del_" << sym.name
        << "(" << sym.type << " " << (sym.ddm.delParam? sym.ddm.delParam : "") << ") ";
    emitUserCode(out, sym.ddm.delCode);
  }

  if (mergeOk && sym.ddm.mergeCode) {
    out << "static inline " << sym.type << "  merge_" << sym.name
        << "(" << sym.type << " " << sym.ddm.mergeParam1
        << ", " << sym.type << " " << sym.ddm.mergeParam2 << ") ";
    emitUserCode(out, sym.ddm.mergeCode);
  }
}

void emitSwitchCode(EmitCode &out, char const *signature, char const *switchVar,
                    ObjList<Symbol> const &syms, int whichFunc,
                    char const *templateCode, char const *actUpon)
{
  out << signature << "\n"
         "{\n"
         "  switch (" << switchVar << ") {\n";

  FOREACH_OBJLIST(Symbol, syms, symIter) {
    Symbol const &sym = *(symIter.data());

    if (whichFunc==0 && sym.ddm.dupCode ||
        whichFunc==1 && sym.ddm.delCode ||
        whichFunc==2 && sym.ddm.mergeCode) {
      out << "    case " << sym.getTermOrNontermIndex() << ":\n";
      out << replace(replace(templateCode, "$symName", sym.name),
                     "$symType", sym.type);
    }
  }

  out << "    default:\n"
         "      cout << \"there is no action to " << actUpon << " id \"\n"
         "           << " << switchVar << " << endl;\n"
         "      abort();\n"
         "  }\n"
         "}\n"
         "\n";
}


// ------------------------- main --------------------------
#ifdef GRAMANL_MAIN

#include "grampar.h"      // readGrammarFile
#include "bflatten.h"     // BFlatten
#include <stdio.h>        // remove
#include <stdlib.h>       // system


int main(int argc, char **argv)
{
  char const *progName = argv[0];
  TRACE_ARGS();
  traceAddSys("progress");

  bool testRW = false;
  if (argc >= 2 &&
      0==strcmp(argv[1], "--testRW")) {
    testRW = true;
    argc--;
    argv++;
  }

  if (argc != 2) {
    cout << "usage: " << progName << " [-tr traceFlags] [--testRW] prefix\n"
            "  processes prefix.gr to make prefix.{h,cc,bin}\n";
    return 0;
  }
  string prefix = argv[1];

  
  bool printCode = true;

  string grammarFname = stringc << prefix << ".gr";
  GrammarAnalysis g;
  readGrammarFile(g, grammarFname);
  g.printProductions(trace("grammar") << endl);

  g.runAnalyses();

  // print some stuff to a test file
  char const g1Fname[] = "gramanl.g1.tmp";
  if (testRW) {
    traceProgress() << "printing ascii grammar " << g1Fname << endl;
    {
      ofstream out(g1Fname);
      g.printProductionsAndItems(out, printCode);
    }
  }

  // emit some C++ code
  //string headerFname = stringc << prefix << ".h";
  string implFname = stringc << prefix << ".cc";
  traceProgress() << "emitting C++ code to "
                  //<< headerFname << " and "
                  << implFname << " ...\n";
  //emitSemFunImplFile(implFname, headerFname, &g);
  //emitSemFunDeclFile(headerFname, &g);

  emitActionCode(g, implFname, grammarFname);

  // write the analyzed grammar to a file
  string binFname = stringc << prefix << ".bin";
  traceProgress() << "writing binary grammar file " << binFname << endl;
  {
    BFlatten flatOut(binFname, false /*reading*/);
    g.xfer(flatOut);
  }

  if (testRW) {
    // read in the binary file
    traceProgress() << "reading binary grammar file " << binFname << endl;
    BFlatten flatIn(binFname, true /*reading*/);
    GrammarAnalysis g2;
    g2.xfer(flatIn);

    // print same stuff to another file
    char const g2Fname[] = "gramanl.g2.tmp";
    traceProgress() << "printing ascii grammar " << g2Fname << endl;
    {
      ofstream out(g2Fname);
      g2.printProductionsAndItems(out, printCode);
    }

    // diff 'em
    traceProgress() << "comparing ascii grammar\n";
    if (system(stringc << "diff " << g1Fname << " " << g2Fname) != 0) {
      cout << "the grammars differ!!\n";
      return 4;
    }

    // remove the temps
    if (!tracingSys("keep-tmp")) {
      remove(g1Fname);
      remove(g2Fname);
    }

    cout << "ticksComputeNonkernel: " << ticksComputeNonkernel << endl;    
    cout << "testRW SUCCESS!\n";
  }    

  else {
    // I want to know how long writing takes
    traceProgress() << "done\n";
  }

  return 0;
}

#endif // GRAMANL_MAIN


// ---------------------- trash ------------------
  #if 0
  out << "void mergeAlternativeParses(int nontermId, SemanticValue left,\n"
         "                            SemanticValue right)\n"
         "{\n"
         "  switch (nontermId) {\n";

  FOREACH_OBJLIST(Nonterminal, g.nonterminals, ntIter) {
    Nonterminal const &nt = *(ntIter.data());

    if (nt.ddm.mergeCode) {
      out << "    case " << nt.ntIndex << ":\n"
          << "      return (SemanticValue)merge_" << nt.name
          <<                "((" << nt.type << ")left, (" << nt.type << ")right);\n";
    }
  }

  out << "    default:\n"
         "      cout << \"there is no action to merge nonterm id \"\n"
         "           << nontermId << endl;\n"
         "      abort();\n"
         "  }\n"
         "}\n"
         "\n";
  #endif // 0
