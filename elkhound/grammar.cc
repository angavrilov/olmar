// grammar.cc
// code for grammar.h

#include "grammar.h"   // this module
#include "syserr.h"    // xsyserror
#include "strtokp.h"   // StrtokParse

#include <stdarg.h>    // variable-args stuff
#include <stdio.h>     // FILE, etc.
#include <ctype.h>     // isupper


// print a variable value
#define PVAL(var) os << " " << #var "=" << var;


// ---------------------- Symbol --------------------
Symbol::~Symbol()
{}


void Symbol::print(ostream &os) const
{
  os << name << ":";
  PVAL(isTerm);
}


Terminal const &Symbol::asTerminalC() const
{
  xassert(isTerminal());
  return (Terminal&)(*this);
}

Nonterminal const &Symbol::asNonterminalC() const
{			 
  xassert(isNonterminal());
  return (Nonterminal&)(*this);
}


// -------------------- Terminal ------------------------
void Terminal::print(ostream &os) const
{
  os << "[" << termIndex << "] ";
  Symbol::print(os);
}


// ----------------- Nonterminal ------------------------
Nonterminal::Nonterminal(char const *name)
  : Symbol(name, false /*terminal*/),
    ntIndex(-1),
    cyclic(false)
{}

Nonterminal::~Nonterminal()
{}


void printTerminalSet(ostream &os, TerminalList const &list)
{
  os << "{";
  int ct = 0;
  for (TerminalListIter term(list); !term.isDone(); term.adv(), ct++) {
    if (ct > 0) {
      os << ", ";
    }
    os << term.data()->name;
  }
  os << "}";
}


void Nonterminal::print(ostream &os) const
{
  os << "[" << ntIndex << "] ";
  Symbol::print(os);

  // cyclic?
  if (cyclic) {
    os << " (cyclic!)";
  }

  // first
  os << " first=";
  printTerminalSet(os, first);

  // follow
  os << " follow=";
  printTerminalSet(os, follow);
}


// -------------------- Production -------------------------
Production::Production(Nonterminal *L)
  : left(L),
    numDotPlaces(-1),    // so the check in getDProd will fail
    dprods(NULL)
{}

Production::~Production()
{
  if (dprods) {
    delete[] dprods;
  }
}


int Production::rhsLength() const
{
  if (!right.isEmpty() &&
      right.nthC(0)->isEmptyString) {
    return 0;    // length is considered 0 for 'blah -> empty'
  }
  else {
    return right.count();
  }
}


void Production::append(Symbol *sym)
{
  right.append(sym);
}


void Production::finished()
{
  xassert(dprods == NULL);    // otherwise we leak

  // compute 'dprods'
  numDotPlaces = rhsLength()+1;
  dprods = new DottedProduction[numDotPlaces];

  INTLOOP(dotPosn, 0, numDotPlaces) {
    dprods[dotPosn].prod = this;
    dprods[dotPosn].dot = dotPosn;
  }

  // The decision to represent dotted productions this way is driven
  // by a couple factors.  Note that the principal alternative is to
  // store (prod,dotPosn) pairs explicitly (presumably because two
  // 16-bit values are sufficient, so it fits in a word), or even
  // to try to store only dotPosn, and infer prod from context.
  //
  // First, letting each dotted production have a unique representation
  // and therefore a unique address means that pointers to these can
  // easily be stored in my list-as-set classes, with equality checks
  // being simple pointer comparisons.  (This could be supported fairly
  // easily, though, using list-as-set of word.)
  //
  // Second, should there ever arise the desire to store additional
  // info with each dotted production, that option is easy to support.
  // (So I guess this is the real reason.)
}


DottedProduction const *Production::getDProdC(int dotPlace) const
{
  xassert(0 <= dotPlace && dotPlace < numDotPlaces);
  return &dprods[dotPlace];
}


void Production::print(ostream &os) const
{
  os << toString();
}


string Production::toString() const
{
  stringBuilder sb;

  sb << left->name << " ->";

  for (SymbolListIter iter(right); !iter.isDone(); iter.adv()) {
    sb << " " << iter.data()->name;
  }

  return sb;
}


// ----------------- DottedProduction ------------------
bool DottedProduction::isDotAtEnd() const
{
  return dot == prod->rhsLength();
}


Symbol const *DottedProduction::symbolBeforeDotC() const
{
  return prod->right.nthC(0);
}

Symbol const *DottedProduction::symbolAfterDotC() const
{
  return prod->right.nthC(dot);
}


void DottedProduction::print(ostream &os) const
{
  os << prod->left->name << " ->";

  int position = 0;
  for (SymbolListIter iter(prod->right);
       !iter.isDone(); iter.adv(), position++) {
    if (position == dot) {
      os << " .";
    }
    os << " " << iter.data()->name;
  }
  if (position == dot) {
    os << " .";
  }
}


// ----------------- ItemSet -------------------
ItemSet::ItemSet(int anId, int numTerms, int numNonterms)
  : id(anId),
    terms(numTerms),
    nonterms(numNonterms)
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
}


Symbol const *ItemSet::getStateSymbolC() const
{
  SFOREACH_DOTTEDPRODUCTION(items, item) {
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

							    
// return the reductions that are ready in this state, given
// that the next symbol is 'lookahead'
void ItemSet::getPossibleReductions(ProductionList &reductions,
                                    Terminal const *lookahead) const
{
  // for each item
  SFOREACH_DOTTEDPRODUCTION(items, itemIter) {
    DottedProduction const *item = itemIter.data();

    // it has to have the dot at the end
    if (!item->isDotAtEnd()) {
      continue;
    }

    // and the follow of its LHS must include 'lookahead'
    // NOTE: this is the difference between LR(0) and SLR(1) --
    //       LR(0) would not do this check, while SLR(1) does
    if (!item->prod->left->follow.contains(lookahead)) {    // (constness)
      continue;
    }

    // ok, this one's ready
    reductions.append(item->prod);			    // (constness)
  }
}


void ItemSet::print(ostream &os) const
{
  os << "ItemSet " << id << ":\n";

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
}


void ItemSet::writeGraph(ostream &os) const
{
  // node: n <name> <desc>
  os << "\nn ItemSet" << id << " ItemSet" << id << "/";
    // rest of desc will follow

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


// ------------------ Grammar -----------------
Grammar::Grammar()
  : startSymbol(NULL),
    emptyString("empty")
{
  emptyString.isEmptyString = true;
}


Grammar::~Grammar()
{}


int Grammar::numTerminals() const
{
  return terminals.count();
}

int Grammar::numNonterminals() const
{                                
  // everywhere, we regard emptyString as a nonterminal
  return nonterminals.count() + 1;
}


void Grammar::printProductions(ostream &os) const
{
  os << "Grammar productions:\n";
  for (ObjListIter<Production> iter(productions);
       !iter.isDone(); iter.adv()) {
    os << "  ";
    iter.data()->print(os);
    os << endl;
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
      cerr << "error parsing line " << line << endl;
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
