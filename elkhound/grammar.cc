// grammar.cc
// code for grammar.h

#include "grammar.h"   // this module
#include "syserr.h"    // xsyserror
#include "strtokp.h"   // StrtokParse
#include "trace.h"     // trace
#include "exc.h"       // xBase
#include "strutil.h"   // quoted, parseQuotedString
#include "flatten.h"   // Flatten
#include "flatutil.h"  // various xfer helpers

#include <stdarg.h>    // variable-args stuff
#include <stdio.h>     // FILE, etc.
#include <ctype.h>     // isupper
#include <stdlib.h>    // atoi


// print a variable value
#define PVAL(var) os << " " << #var "=" << var;


// ----------------- ConflictHandlers -----------------
ConflictHandlers::ConflictHandlers()
  : dupParam(NULL),
    delParam(NULL),
    mergeParam1(NULL),
    mergeParam2(NULL),
    cancelParam(NULL)
{}

ConflictHandlers::~ConflictHandlers()
{}

ConflictHandlers::ConflictHandlers(Flatten&)
  : dupParam(NULL),
    delParam(NULL),
    mergeParam1(NULL),
    mergeParam2(NULL),
    cancelParam(NULL)
{}

void ConflictHandlers::xfer(Flatten &flat)
{
  flattenStrTable->xfer(flat, dupParam);
  dupCode.xfer(flat);

  flattenStrTable->xfer(flat, delParam);
  delCode.xfer(flat);

  flattenStrTable->xfer(flat, mergeParam1);
  flattenStrTable->xfer(flat, mergeParam2);
  mergeCode.xfer(flat);

  flattenStrTable->xfer(flat, cancelParam);
  cancelCode.xfer(flat);
}


bool ConflictHandlers::anyNonNull() const
{
  return dupCode.isNonNull() ||
         delCode.isNonNull() ||
         mergeCode.isNonNull() ||
         cancelCode.isNonNull();
}


void ConflictHandlers::print(ostream &os) const
{
  if (dupCode.isNonNull()) {
    os << "    dup(" << dupParam << ") [" << dupCode << "]\n";
  }

  if (delCode.isNonNull()) {
    os << "    del(" << (delParam? delParam : "") << ") [" << delCode << "]\n";
  }

  if (mergeCode.isNonNull()) {
    os << "    merge(" << mergeParam1 << ", " << mergeParam2
       << ") [" << mergeCode << "]\n";
  }

  if (cancelCode.isNonNull()) {
    os << "    cancel(" << dupParam << ") [" << dupCode << "]\n";
  }
}


// ---------------------- Symbol --------------------
Symbol::~Symbol()
{}


Symbol::Symbol(Flatten &flat)
  : name(flat),
    isTerm(false),
    isEmptyString(false),
    type(NULL),
    ddm()
{}

void Symbol::xfer(Flatten &flat)
{
  // have to break constness to unflatten
  const_cast<string&>(name).xfer(flat);
  flat.xferBool(const_cast<bool&>(isTerm));
  flat.xferBool(const_cast<bool&>(isEmptyString));

  flattenStrTable->xfer(flat, type);
  ddm.xfer(flat);
}


int Symbol::getTermOrNontermIndex() const
{
  if (isTerminal()) {
    return asTerminalC().termIndex;
  }
  else {
    return asNonterminalC().ntIndex;
  }
}


void Symbol::print(ostream &os) const
{
  os << name;
  if (type) {
    os << "[" << type << "]";
  }
  os << ":";
  PVAL(isTerm);
}


void Symbol::printDDM(ostream &os) const
{
  // don't print anything if no handlers
  if (!ddm.anyNonNull()) return;

  // print with roughly the same syntax as input
  os << "  " << (isTerminal()? "token" : "nonterm");
  if (type) {
    os << "[" << type << "]";
  }
  os << " " << name << " {\n";

  ddm.print(os);

  os << "  }\n";
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
Terminal::Terminal(Flatten &flat)
  : Symbol(flat),
    alias(flat)
{}

void Terminal::xfer(Flatten &flat)
{
  Symbol::xfer(flat);

  alias.xfer(flat);        

  flat.xferInt(precedence);
  flat.xferInt((int&)associativity);

  flat.xferInt(termIndex);
}


void Terminal::print(ostream &os) const
{
  os << "[" << termIndex << "]";
  if (precedence) {
    os << "(" << ::toString(associativity) << " " << precedence << ")";
  }
  os << " ";
  Symbol::print(os);
}


string Terminal::toString() const
{
  if (alias.length() > 0) {
    return alias;
  }
  else {
    return name;
  }
}


// ----------------- Nonterminal ------------------------
Nonterminal::Nonterminal(char const *name, bool isEmpty)
  : Symbol(name, false /*terminal*/, isEmpty),
    ntIndex(-1),
    cyclic(false),
    first(),
    follow()
{}

Nonterminal::~Nonterminal()
{}


Nonterminal::Nonterminal(Flatten &flat)
  : Symbol(flat)
{}

void Nonterminal::xfer(Flatten &flat)
{
  Symbol::xfer(flat);
}

void Nonterminal::xferSerfs(Flatten &flat, Grammar &g)
{
  // annotation
  flat.xferInt(ntIndex);
  flat.xferBool(cyclic);
  xferSObjList(flat, first, g.terminals);
  xferSObjList(flat, follow, g.terminals);
}


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


// -------------------- Production::RHSElt -------------------------
Production::RHSElt::~RHSElt()
{}


Production::RHSElt::RHSElt(Flatten &flat)
  : sym(NULL),
    tag(flat)
{}

void Production::RHSElt::xfer(Flatten &flat)
{
  tag.xfer(flat);
}

void Production::RHSElt::xferSerfs(Flatten &flat, Grammar &g)
{
  xferSerfPtr(flat, sym);
}



// -------------------- Production -------------------------
Production::Production(Nonterminal *L, char const *Ltag)
  : left(L),
    leftTag(Ltag),
    right(),
    precedence(0),
    prodIndex(-1)
{}

Production::~Production()
{}


Production::Production(Flatten &flat)
  : left(NULL),
    leftTag(flat),
    action(flat)
{}

void Production::xfer(Flatten &flat)
{
  leftTag.xfer(flat);
  xferObjList(flat, right);
  action.xfer(flat);
  flat.xferInt(precedence);

  flat.xferInt(prodIndex);
}

void Production::xferSerfs(Flatten &flat, Grammar &g)
{
  // must break constness in xfer

  xferSerfPtrToList(flat, const_cast<Nonterminal*&>(left),
                          g.nonterminals);

  // xfer right's 'sym' pointers
  MUTATE_EACH_OBJLIST(RHSElt, right, iter) {
    iter.data()->xferSerfs(flat, g);
  }

  // compute derived data
  if (flat.reading()) {
    finished();
  }
}


int Production::rhsLength() const
{
  if (!right.isEmpty()) {
    // I used to have code here which handled this situation by returning 0;
    // since it should now never happen, I'll check that here instead
    xassert(!right.nthC(0)->sym->isEmptyString);
  }

  return right.count();
}


int Production::numRHSNonterminals() const
{
  int ct = 0;
  FOREACH_OBJLIST(RHSElt, right, iter) {
    if (iter.data()->sym->isNonterminal()) {
      ct++;
    }
  }
  return ct;
}


bool Production::rhsHasSymbol(Symbol const *sym) const
{
  FOREACH_OBJLIST(RHSElt, right, iter) {
    if (iter.data()->sym == sym) {
      return true;
    }
  }
  return false;
}


void Production::getRHSSymbols(SymbolList &output) const
{
  FOREACH_OBJLIST(RHSElt, right, iter) {
    output.append(iter.data()->sym);
  }
}


void Production::append(Symbol *sym, char const *tag)
{
  // my new design decision (6/26/00 14:24) is to disallow the
  // emptyString nonterminal from explicitly appearing in the
  // productions
  xassert(!sym->isEmptyString);

  right.append(new RHSElt(sym, tag));
}


void Production::finished()
{
  // this used to precompute dotted productions, but they're
  // now stored by the item sets in gramanl
}

                                               
// basically strcmp but without the segfaults when s1 or s2
// is null; return true if strings are equal
bool tagCompare(char const *s1, char const *s2)
{
  if (s1 == NULL  ||  s2 == NULL) {
    return s1 == s2;
  }
  else {
    return 0==strcmp(s1, s2);
  }
}


int Production::findTag(char const *tag) const
{
  // check LHS
  if (tagCompare(leftTag, tag)) {
    return 0;
  }

  // walk RHS list looking for a match
  ObjListIter<RHSElt> tagIter(right);
  int index=1;
  for(; !tagIter.isDone(); tagIter.adv(), index++) {
    if (tagCompare(tagIter.data()->tag, tag)) {
      return index;
    }
  }

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
    return stringb(tag << ":" << name);
  }
}


string Production::symbolTag(int index) const
{
  // check LHS
  if (index == 0) {
    return leftTag;
  }

  // find index in RHS list
  index--;
  return string(right.nthC(index)->tag);
}


Symbol const *Production::symbolByIndexC(int index) const
{
  // check LHS
  if (index == 0) {
    return left;
  }

  // find index in RHS list
  index--;
  return right.nthC(index)->sym;
}


#if 0
DottedProduction const *Production::getDProdC(int dotPlace) const
{
  xassert(0 <= dotPlace && dotPlace < numDotPlaces);
  return &dprods[dotPlace];
}    
#endif // 0


void Production::print(ostream &os) const
{
  os << toString();
}


string Production::toString(bool printType) const
{
  // LHS "->" RHS
  stringBuilder sb;
  sb << left->name;
  if (printType && left->type) {
    sb << "[" << left->type << "]";
  }
  sb << " -> " << rhsString();
  
  if (printType && precedence) {
    // take this as licence to print prec too
    sb << " %prec(" << precedence << ")";
  }
  return sb;
}


string Production::rhsString() const
{
  stringBuilder sb;

  if (right.isNotEmpty()) {
    // print the RHS symbols
    int ct=0;
    FOREACH_OBJLIST(RHSElt, right, iter) {
      RHSElt const &elt = *(iter.data());

      if (ct++ > 0) {
        sb << " ";
      }

      string symName;
      if (elt.sym->isNonterminal()) {
        symName = elt.sym->name;
      }
      else {
        // print terminals as aliases if possible
        symName = elt.sym->asTerminalC().toString();
      }

      // print tag if present
      sb << taggedName(symName, elt.tag);
    }
  }

  else {
    // empty RHS
    sb << "empty";
  }

  return sb;
}


string Production::toStringMore(bool printCode) const
{
  stringBuilder sb;
  sb << toString();

  if (printCode && !action.isNull()) {
    sb << "\t\t[" << action.strref() << "]";
  }

  sb << "\n";

  return sb;
}


// ------------------ Grammar -----------------
Grammar::Grammar()
  : startSymbol(NULL),
    emptyString("empty", true /*isEmptyString*/)
{}


Grammar::~Grammar()
{}


void Grammar::xfer(Flatten &flat)
{
  // owners
  flat.checkpoint(0xC7AB4D86);
  xferObjList(flat, nonterminals);
  xferObjList(flat, terminals);
  xferObjList(flat, productions);

  // emptyString is const

  // serfs
  flat.checkpoint(0x8580AAD2);

  MUTATE_EACH_OBJLIST(Nonterminal, nonterminals, nt) {
    nt.data()->xferSerfs(flat, *this);
  }
  MUTATE_EACH_OBJLIST(Production, productions, p) {
    p.data()->xferSerfs(flat, *this);
  }

  xferSerfPtrToList(flat, startSymbol, nonterminals);

  flat.checkpoint(0x2874DB95);
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


void Grammar::printSymbolTypes(ostream &os) const
{
  os << "Grammar terminals with types or precedence:\n";
  FOREACH_OBJLIST(Terminal, terminals, term) {
    Terminal const &t = *(term.data());
    t.printDDM(os);
    if (t.precedence) {
      os << "  " << t.name << " " << ::toString(t.associativity)
         << " %prec " << t.precedence << endl;
    }
  }

  os << "Grammar nonterminals with types:\n";
  FOREACH_OBJLIST(Nonterminal, nonterminals, nt) {
    nt.data()->printDDM(os);
  }
}


void Grammar::printProductions(ostream &os, bool code) const
{
  os << "Grammar productions:\n";
  for (ObjListIter<Production> iter(productions);
       !iter.isDone(); iter.adv()) {
    os << "  " << iter.data()->toStringMore(code);
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

  prod->prodIndex = productions.count();
  productions.append(prod);
  
  // if the start symbol isn't defined yet, we can here
  // implement the convention that the LHS of the first
  // production is the start symbol
  if (startSymbol == NULL) {
    startSymbol = prod->left;
  }
}


// add a token to those we know about
bool Grammar::declareToken(char const *symbolName, int code, char const *alias)
{
  // verify that this token hasn't been declared already
  if (findSymbolC(symbolName)) {
    cout << "token " << symbolName << " has already been declared\n";
    return false;
  }

  // create a new terminal class
  Terminal *term = getOrMakeTerminal(symbolName);

  // assign fields specified in %token declaration
  term->termIndex = code;
  term->alias = alias;

  return true;
}


// well-formedness check
void Grammar::checkWellFormed() const
{
  // after removing some things, now there's nothing to check...
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
        FOREACH_OBJLIST(Production::RHSElt, prod.data()->right, symIter) {
          Symbol const *sym = symIter.data()->sym;
          if (sym != &emptyString) {
            if (sym->isTerminal()) {
              os << " \"" << sym->name << "\"";
            }
            else {
              os << " " << sym->name;
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
    if (iter.data()->name.equals(name) ||
        iter.data()->alias.equals(name)) {
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


int Grammar::getProductionIndex(Production const *prod) const
{
  int ret = productions.indexOf(prod);
  xassert(ret != -1);
  return ret;
}


string symbolSequenceToString(SymbolList const &list)
{
  stringBuilder sb;   // collects output

  bool first = true;
  SFOREACH_SYMBOL(list, sym) {
    if (!first) {
      sb << " ";
    }

    if (sym.data()->isTerminal()) {
      sb << sym.data()->asTerminalC().toString();
    }
    else {
      sb << sym.data()->name;
    }
    first = false;
  }

  return sb;
}


string terminalSequenceToString(TerminalList const &list)
{
  // this works because access is read-only
  return symbolSequenceToString(reinterpret_cast<SymbolList const&>(list));
}


// ------------------ emitting C++ code ---------------------
#if 0     // not done
void Grammar::emitSelfCC(ostream &os) const
{
  os << "void buildGrammar(Grammar *g)\n"
        "{\n";

  FOREACH_OBJLIST(Terminal, terminals, termIter) {
    Terminal const *term = termIter.data();

    os << "g->declareToken(" << term->name
       << ", " << term->termIndex
       << ", " << quoted(term->alias)
       << ");\n";
  }

  FOREACH_OBJLIST(Nonterminal, nonterminals, ntIter) {
    Nonterminal const *nt = ntIter.data();

    os << ...
  }

  os << "}\n";

  // todo: more
}
#endif // 0


