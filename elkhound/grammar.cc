// grammar.cc
// code for grammar.h

#include "grammar.h"   // this module
#include "syserr.h"    // xsyserror
#include "strtokp.h"   // StrtokParse
#include "trace.h"     // trace

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


// --------------------- quoted strings -------------------------
// given a string that is surrounded by quotes, yield only the
// characters inside the quotes.  eventually, I will support
// using backslash escapes for special characters
string parseQuotedString(char const *text)
{
  if (!( text[0] == '"' &&
         text[strlen(text)-1] == '"' )) {
    xfailure(stringc << "quoted string is missing quotes: " << text);
  }

  // just strip the quotes
  return string(text+1, strlen(text)-2);
}

                  
// take a string, and return a quoted version of it
string quoteString(char const *text)
{
  // for now, simply surround it with quotes
  return stringc << "\"" << text << "\"";
}


// -------------------- Production -------------------------
Production::Production(Nonterminal *L, char const *Ltag)
  : left(L),
    right(),
    leftTag(Ltag),
    rightTags(),
    conditions(),
    actions(),
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


int Production::numRHSNonterminals() const
{
  int ct = 0;
  SFOREACH_SYMBOL(right, sym) {
    if (!sym.data()->isEmptyString &&
        sym.data()->isNonterminal()) {
      ct++;
    }
  }
  return ct;
}


void Production::append(Symbol *sym, char const *tag)
{
  // my new design decision (6/26/00 14:24) is to disallow the
  // emptyString nonterminal from explicitly appearing in the
  // productions
  xassert(!sym->isEmptyString);

  right.append(sym);
  rightTags.append(new string(tag));
}


void Production::finished()
{
  xassert(dprods == NULL);    // otherwise we leak
                                              
  // invariant check
  xassert(right.count() == rightTags.count());

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

             
// in my infinite wisdom, I have to representations of the empty
// string...
bool tagEqual(char const *tag1, char const *tag2)
{
  if ((tag1==NULL || tag1[0]==0) &&
      (tag2==NULL || tag2[0]==0)) {
    return true;     // both are some form of empty string
  }

  if (tag1==NULL || tag2==NULL) {
    return false;    // one is empty and other isn't (and I can't call strcmp)
  }
                  
  // neither is NULL, can use strcmp
  return 0==strcmp(tag1, tag2);
}


int Production::findTaggedSymbol(char const *name, char const *tag) const
{
  // check LHS
  if (left->name.equals(name) &&
      tagEqual(leftTag, tag)) {
    return 0;
  }

  // walk RHS list looking for a match
  ObjListIter<string> tagIter(rightTags);
  SymbolListIter symIter(right);
  int index=1;
  for(; !symIter.isDone() && !tagIter.isDone();
        symIter.adv(), tagIter.adv(), index++) {
    if (symIter.data()->name.equals(name) &&
        tagEqual(*(tagIter.data()), tag)) {
      return index;
    }
  }

  // verify same length
  xassert(symIter.isDone() && tagIter.isDone());

  // not found
  return -1;
}


// assemble a possibly tagged name for printing
string taggedName(char const *name, char const *tag)
{
  if (tag == NULL || tag[0] == 0) {
    return string(name);
  }
  else {
    return stringb(name << "." << tag);
  }
}


string Production::taggedSymbolName(int index) const
{
  // check LHS
  if (index == 0) {
    return taggedName(left->name, leftTag);
  }

  // find index in RHS list
  index--;
  return taggedName(right.nthC(index)->name,
                    *(rightTags.nthC(index)));
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
                            
  // LHS symbol and "->"
  sb << taggedName(left->name, leftTag) << " ->";

  if (right.isNotEmpty()) {
    // print the RHS symbols
    ObjListIter<string> tagIter(rightTags);
    SymbolListIter symIter(right);
    for(; !symIter.isDone() && !tagIter.isDone();
          symIter.adv(), tagIter.adv()) {
      if (symIter.data()->isNonterminal()) {
        // print the nonterminal as a name and optional tag
        sb << " " << taggedName(symIter.data()->name, *(tagIter.data()));
      }
      else {
        // print terminals in quotes
        sb << " " << quoteString(symIter.data()->name);
      }
    }

    // verify both lists were same length
    xassert(symIter.isDone() && tagIter.isDone());
  }

  else {
    // empty RHS
    sb << " empty";
  }

  return sb;
}


string Production::toStringWithActions() const
{
  stringBuilder sb;
  sb << toString() << "\n"
     << actions.toString(this) 
     << conditions.toString(this)
     ;
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
  : kernelItems(),
    nonkernelItems(),
    termTransition(NULL),      // inited below
    nontermTransition(NULL),   // inited below
    terms(numTerms),
    nonterms(numNonterms),
    id(anId),
    BFSparent(NULL)
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

            
// compare two items in an arbitrary (but deterministic) way so that
// sorting will always put a list of items into the same order, for
// comparison purposes
int itemDiff(DottedProduction const *left, DottedProduction const *right, void*)
{
  // memory address is sufficient for my purposes, since I avoid ever
  // creating two copies of a given item
  return (int)left - (int)right;
}


void ItemSet::addKernelItem(DottedProduction *item)
{
  // add it
  kernelItems.appendUnique(item);

  // sort the items to facilitate equality checks
  kernelItems.insertionSort(itemDiff);
}


bool ItemSet::operator==(ItemSet const &obj) const
{
  #if 1
  // since nonkernel items are entirely determined by kernel
  // items, and kernel items are sorted, it's sufficient to
  // check for kernel list equality
  return kernelItems.equalAsPointerLists(obj.kernelItems);

  #else   // experiment
  DProductionList myItems;
  getAllItems(myItems);

  DProductionList hisItems;
  obj.getAllItems(hisItems);

  return myItems.equalAsPointerSets(hisItems);
  #endif
}


void ItemSet::addNonkernelItem(DottedProduction *item)
{
  nonkernelItems.appendUnique(item);
}


void ItemSet::getAllItems(DProductionList &dest) const
{
  dest = kernelItems;
  dest.appendAll(nonkernelItems);
}


// return the reductions that are ready in this state, given
// that the next symbol is 'lookahead'
void ItemSet::getPossibleReductions(ProductionList &reductions,
                                    Terminal const *lookahead,
                                    bool parsing) const
{
  // collect all items
  DProductionList items;
  getAllItems(items);

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
      if (parsing) {
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


void ItemSet::print(ostream &os) const
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
    os << " " << iter.data()->toStringWithActions();
  }
}


void Grammar::addProduction(Nonterminal *lhs, Symbol *firstRhs, ...)
{
  va_list argptr;                   // state for working through args
  Symbol *arg;
  va_start(argptr, firstRhs);       // initialize 'argptr'

  Production *prod = new Production(lhs, NULL /*tag*/);
  prod->append(firstRhs, NULL /*tag*/);
  for(;;) {
    arg = va_arg(argptr, Symbol*);  // get next argument
    if (arg == NULL) {
      break;    // end of list
    }

    prod->append(arg, NULL /*tag*/);
  }

  addProduction(prod);
}


void Grammar::addProduction(Production *prod)
{
  // I used to add emptyString if there were 0 RHS symbols,
  // but I've now switched to not explicitly saying that

  productions.append(prod);
  
  // if the start symbol isn't defined yet, we can here
  // implement the convention that the LHS of the first
  // production is the start symbol
  if (startSymbol == NULL) {
    startSymbol = prod->left;
  }
}


// the present syntax defined for grammars is not ideal, and the code
// to parse it shouldn't be in this file; I'll fix these problems at
// some point..


void Grammar::readFile(char const *fname)
{
  FILE *fp = fopen(fname, "r");
  if (!fp) {
    xsyserror("fopen", stringb("opening " << fname));
  }

  // state between lines
  SObjList<Production> lastProductions;      // the last one(s) we created

  char buf[256];
  int line = 0;
  while (fgets(buf, 256, fp)) {
    line++;

    if (!parseLine(buf, lastProductions)) {
      cerr << "error parsing line " << line << endl;
      break;    // stop after first error
    }
  }

  if (fclose(fp) != 0) {
    xsyserror("fclose");
  }
}


// parse a possibly-tagged symbol name into 'name' and 'tag'
void parseTaggedName(string &name, string &tag, char const *tagged)
{
  StrtokParse tok(tagged, ".");
  if (tok < 1) {
    xfailure("tagged name consisting only of dots!");
  }
  if (tok > 2) {
    xfailure("tagged name with too many tags");
  }

  name = tok[0];
  if (tok == 2) {
    tag = tok[1];
  }
  else {
    tag = NULL;
  }
}


bool Grammar::parseLine(char const *preLine)
{
  SObjList<Production> dummy;
  return parseLine(preLine, dummy);
}


// returns false on parse error
bool Grammar::parseLine(char const *preLine, SObjList<Production> &lastProductions)
{
  // wrap it all in a try so we catch all parsing errors
  try {
    // strip comments
    string line(preLine);
    {
      char *p = strchr(line, '#');
      if (p != NULL) {
        *p = 0;    // truncate line there
      }
    }

    // parse on whitespace
    StrtokParse tok(line, " \t\n\r");
    if (tok == 0) {
      // blank line or comment
      return true;
    }

    // action or condition?
    if (tok[0][0] == '%') {
      if (lastProductions.isEmpty()) {
        cout << "action or condition must be preceeded by a production\n";
        return false;
      }

      // for now, I'm being a bit redundant in my syntax
      if (!(  0==strcmp(tok[1], "{")     &&
              0==strcmp(tok[tok-1], "}")    )) {
        cout << "action or condition must be surrounded by braces\n";
        return false;
      }

      // apply the rule to all of the productions in the previous line
      SMUTATE_EACH_OBJLIST(Production, lastProductions, iter) {
        if (!parseAnAction(tok[0], tok.reassemble(2, tok-2, line), iter.data())) {
          return false;
        }
      }           
      
      // success parsing the action or condition
      return true;
    }

    // going to parse a production, so clear the list
    lastProductions.removeAll();

    // check that the 2nd token is the "rewrites-as" symbol
    if (0!=strcmp(tok[1], "->")) {
      cout << "2nd token of production must be `->'\n";
      return false;
    }

    // get LHS token
    string name, tag;
    parseTaggedName(name, tag, tok[0]);
    string leftTag = tag;     // need to remember it in case we see '|'
    Nonterminal *LHS = getOrMakeNonterminal(name);

    // make a production
    Production *prod = new Production(LHS, leftTag);

    // process RHS symbols
    for (int i=2; i<tok; i++) {
      // alternatives -- syntactic sugar
      if (0==strcmp(tok[i], "|")) {
        // finish the current production
        addProduction(prod);
        lastProductions.append(prod);

        // start another
        prod = new Production(LHS, leftTag);
      }

      // terminal
      else if (tok[i][0] == '"') {
        prod->append(getOrMakeTerminal(parseQuotedString(tok[i])), NULL /*tag*/);
      }

      // empty string
      else if (0==strcmp(tok[i], "empty")) {
        // don't actually add a symbol for this
      }

      // nonterminal
      else if (isupper(tok[i][0])) {
        parseTaggedName(name, tag, tok[i]);
        Symbol *sym = getOrMakeNonterminal(name);
        prod->append(sym, tag);
      }

      // error
      else {
        cout << "not a valid symbol: " << tok[i] << endl;
        return false;
      }
    }

    // done, so add the production
    addProduction(prod);
    lastProductions.append(prod);

    // ok
    return true;
  }
  
  catch (xBase &x) {
    cout << "parse error: " << x << endl;
    return false;
  }
}


bool Grammar::parseAnAction(char const *keyword, char const *insideBraces,
                            Production *lastProduction)
{
  if (0==strcmp(keyword, "%action")) {
    lastProduction->actions.parse(lastProduction, insideBraces);
    return true;
  }

  else if (0==strcmp(keyword, "%condition")) {
    lastProduction->conditions.parse(lastProduction, insideBraces);
    return true;
  }

  else {
    cout << "unknown action or condition keyword: " << keyword << endl;
    return false;
  }
}


// well-formedness check; prints complaints
void Grammar::checkWellFormed() const
{
  // verify every nonterminal has at least one rule
  FOREACH_NONTERMINAL(nonterminals, nt) {
    // does this nonterminal have a rule?
    bool hasRule = false;
    FOREACH_PRODUCTION(productions, prod) {
      if (prod.data()->left == nt.data()) {
        hasRule = true;
        break;
      }
    }

    if (!hasRule) {
      cout << "warning: nonterminal "
           << nt.data()->name
           << " has no rules\n";
    }
  }
}


// print the grammar in a form that Bison likes
void Grammar::printAsBison(ostream &os) const
{
  os << "/* automatically generated grammar */\n\n";

  os << "/* -------- tokens -------- */\n";
  FOREACH_TERMINAL(terminals, term) {
    // I'll surround all my tokens with quotes and see how Bison likes it
    os << "%token \"" << term.data()->name << "\"\n";
  }
  os << "\n\n";

  os << "/* -------- productions ------ */\n"
        "%%\n\n";
  // print every nonterminal's rules
  FOREACH_NONTERMINAL(nonterminals, nt) {
    // look at every rule where this nonterminal is on LHS
    bool first = true;
    FOREACH_PRODUCTION(productions, prod) {
      if (prod.data()->left == nt.data()) {

        if (first) {
          os << nt.data()->name << ":";
        }
        else {       
          os << "\n";
          INTLOOP(i, 0, nt.data()->name.length()) {
            os << " ";
          }
          os << "|";
        }

        // print RHS symbols
        SFOREACH_SYMBOL(prod.data()->right, sym) {
          if (sym.data() != &emptyString) {
            if (sym.data()->isTerminal()) {
              os << " \"" << sym.data()->name << "\"";
            }
            else {
              os << " " << sym.data()->name;
            }
          }
        }

        // or, if empty..
        if (prod.data()->rhsLength() == 0) {
          os << " /* empty */";
        }

        first = false;
      }
    }

    if (first) {
      // no rules..
      os << "/* no rules for " << nt.data()->name << " */";
    }

    os << "\n\n";
  }
}



// ------------------- symbol access -------------------
Nonterminal const *Grammar::findNonterminalC(char const *name) const
{
  // check for empty first, since it's not in the list
  if (emptyString.name.equals(name)) {
    return &emptyString;
  }

  FOREACH_NONTERMINAL(nonterminals, iter) {
    if (iter.data()->name.equals(name)) {
      return iter.data();
    }
  }
  return NULL;
}


Terminal const *Grammar::findTerminalC(char const *name) const
{
  FOREACH_TERMINAL(terminals, iter) {
    if (iter.data()->name.equals(name)) {
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


Symbol const *Grammar::
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

  xfailure("Grammar::inverseTransitionC: no transition from source to target");
  return NULL;     // silence warning
}


string symbolSequenceToString(SymbolList const &list)
{
  stringBuilder sb;   // collects output

  bool first = true;
  SFOREACH_SYMBOL(list, sym) {
    if (!first) {
      sb << " ";
    }
    sb << sym.data()->name;
    first = false;
  }

  return sb;
}


string terminalSequenceToString(TerminalList const &list)
{
  // this works because access is read-only
  return symbolSequenceToString(reinterpret_cast<SymbolList const&>(list));
}
