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


// ---------------------- Symbol --------------------
Symbol::~Symbol()
{}


Symbol::Symbol(Flatten &flat)
  : name(flat),
    isTerm(false),
    isEmptyString(false)
{}

void Symbol::xfer(Flatten &flat)
{
  // have to break constness to unflatten
  const_cast<string&>(name).xfer(flat);
  flat.xferBool(const_cast<bool&>(isTerm));
  flat.xferBool(const_cast<bool&>(isEmptyString));
}


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
Terminal::Terminal(Flatten &flat)
  : Symbol(flat),
    alias(flat)
{}

void Terminal::xfer(Flatten &flat)
{
  Symbol::xfer(flat);
  flat.xferInt(termIndex);
  
  // arguable that for my purposes I don't need this; but
  // it seems pretty easy to do it anyway ..
  alias.xfer(flat);
}


void Terminal::print(ostream &os) const
{
  os << "[" << termIndex << "] ";
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
    attributes(),
    superclasses(),
    funDecls(),
    funPrefixes(),
    declarations(),
    disambFuns(),
    constructor(),
    destructor(),
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

  xferObjList(flat, attributes);
}

void Nonterminal::xferSerfs(Flatten &flat, Grammar &g)
{
  // xfer 'superclasses' (I probably don't need this info, but
  // I wanted to try reading/writing them anyway)
  xferSObjList(flat, superclasses, g.nonterminals);

  // skip literal code: funDecls, funPrefixes, declarations, 
  // disambFuns, constructor, destructor

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


bool Nonterminal::hasSuperclass(Nonterminal const *nt) const
{
  SFOREACH_OBJLIST(Nonterminal, superclasses, iter) {
    if (iter.data() == nt  ||
        iter.data()->hasSuperclass(nt)) {
      return true;
    }
  }
  return false;
}


bool Nonterminal::hasAttribute(char const *attr) const
{
  FOREACH_OBJLIST(string, attributes, iter) {
    if (0==strcmp(iter.data()->pcharc(), attr)) {
      return true;
    }
  }
  return false;
}


// -------------------- Production -------------------------
Production::Production(Nonterminal *L, char const *Ltag)
  : left(L),
    right(),
    leftTag(Ltag),
    rightTags(),
    conditions(),
    actions(),
    treeCompare(NULL),
    functions(),
    numDotPlaces(-1),    // so the check in getDProd will fail
    dprods(NULL),
    prodIndex(-1)
{}

Production::~Production()
{
  if (treeCompare) {
    delete treeCompare;
  }
  if (dprods) {
    delete[] dprods;
  }
}


Production::Production(Flatten &flat)
  : left(NULL),
    treeCompare(NULL),
    dprods(NULL)
{}

void Production::xfer(Flatten &flat)
{
  leftTag.xfer(flat);
  xferObjList(flat, rightTags);

  conditions.xfer(flat);
  actions.xfer(flat);

  // it's not used yet
  computedValue(flat, treeCompare, (AExprNode*)NULL);

  // skip literal code: functions

  flat.xferInt(prodIndex);
}

void Production::xferSerfs(Flatten &flat, Grammar &g)
{
  // must break constness in xfer

  xferSerfPtrToList(flat, const_cast<Nonterminal*&>(left),
                          g.nonterminals);

  // xfer 'right'
  {
    ObjList<Symbol> *lists[2];
    lists[0] = & (ObjList<Symbol>&)(g.terminals);
    lists[1] = & (ObjList<Symbol>&)(g.nonterminals);
    xferSObjList_multi(flat, right, lists, 2);
  }

  // compute 'numDotPlaces' and 'dprods'
  if (flat.reading()) {
    finished();
  }
  
  // to allow pointers to 'dprods', register them
  loopi(numDotPlaces) {
    flat.noteOwner(dprods+i);
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
    dprods[dotPosn].setProdAndDot(this, dotPosn);
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
  ObjListIter<string> tagIter(rightTags);
  int index=1;
  for(; !tagIter.isDone(); tagIter.adv(), index++) {
    if (tagCompare( *(tagIter.data()) , tag )) {
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
  return *(rightTags.nthC(index));
}


Symbol const *Production::symbolByIndexC(int index) const
{
  // check LHS
  if (index == 0) {
    return left;
  }

  // find index in RHS list
  index--;
  return right.nthC(index);
}


DottedProduction const *Production::getDProdC(int dotPlace) const
{
  xassert(0 <= dotPlace && dotPlace < numDotPlaces);
  return &dprods[dotPlace];
}


void Production::checkRefs() const
{
  try {
    actions.check(this);
    conditions.check(this);
  }
  catch (xBase &x) {
    THROW(xBase(stringc << "in " << toString()
                        << ", " << x.why() ));
  }
}


void Production::print(ostream &os) const
{
  os << toString();
}


string Production::toString() const
{
  // LHS "->" RHS
  // 9/27/00: don't print tag for LHS
  return stringc << left->name << " -> " << rhsString();
}


string Production::rhsString() const
{
  stringBuilder sb;

  if (right.isNotEmpty()) {
    // print the RHS symbols
    ObjListIter<string> tagIter(rightTags);
    SymbolListIter symIter(right);
    int ct=0;
    for(; !symIter.isDone() && !tagIter.isDone();
          symIter.adv(), tagIter.adv()) {
      if (ct++ > 0) {
        sb << " ";
      }

      string symName;
      if (symIter.data()->isNonterminal()) {
        symName = symIter.data()->name;
      }
      else {
        // print terminals as aliases if possible
        symName = symIter.data()->asTerminalC().toString();
      }

      // print tag if present
      sb << taggedName(symName, *(tagIter.data()));
    }

    // verify both lists were same length
    xassert(symIter.isDone() && tagIter.isDone());
  }

  else {
    // empty RHS
    sb << "empty";
  }

  return sb;
}


string Production::toStringMore(bool printActions, bool printCode) const
{
  stringBuilder sb;
  sb << toString() << "\n";
  
  if (printActions) {
    sb << actions.toString(this)
       << conditions.toString(this)
       ;
  }

  if (printCode) {
    for (LitCodeDict::Iter iter(functions);
         !iter.isDone(); iter.next()) {
      sb << "  fun " << iter.key() << "{ "
         << iter.value()->code << " }\n";
    }
  }

  return sb;
}


// ----------------- DottedProduction ------------------
void DottedProduction::setProdAndDot(Production *p, int d) /*mutable*/
{
  // I prefer this hack to either:
  //   - letting the members be exposed
  //   - protecting them with clumsy accessor functions
  const_cast<Production*&>(prod) = p;
  const_cast<int&>(dot) = d;
  const_cast<bool&>(dotAtEnd) = (dot == prod->rhsLength());
  
  // it's worth mentioning: the reason for all of this is to be
  // able to compute 'dotAtEnd', which it turns out was a
  // significant time consumer according to the profiler
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


// ------------------ Grammar -----------------
Grammar::Grammar()
  : startSymbol(NULL),
    emptyString("empty", true /*isEmptyString*/),
    semanticsPrologue(NULL),
    semanticsEpilogue(NULL),
    treeNodeBaseClass("NonterminalNode")
{}


Grammar::~Grammar()
{
  if (semanticsPrologue) {
    delete semanticsPrologue;
  }
  if (semanticsEpilogue) {
    delete semanticsEpilogue;
  }
}


void Grammar::xfer(Flatten &flat)
{
  // owners
  flat.checkpoint(0xC7AB4D86);
  xferObjList(flat, nonterminals);
  xferObjList(flat, terminals);
  xferObjList(flat, productions);

  // emptyString is const

  // skip the literal code and the base class, because the intent is
  // to read/write grammars *after* the C++ code has been emitted:
  // 'semanticsPrologue', 'semanticsEpilogue', 'treeNodeBaseClass'

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


void Grammar::printProductions(ostream &os, bool actions, bool code) const
{
  os << "Grammar productions:\n";
  for (ObjListIter<Production> iter(productions);
       !iter.isDone(); iter.adv()) {
    os << " " << iter.data()->toStringMore(actions, code);
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
  #if 0   // I no longer check this, because most of my 
          // base classes are empty of productions
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
      xfailure(stringc << "nonterminal " << nt.data()->name
                       << " has no rules");
    }
  }
  #endif // 0
  
  // verify referential integrity in actions/conditions
  FOREACH_PRODUCTION(productions, prod) {
    prod.data()->checkRefs();
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


// -------------- obsolete parsing code ------------------
// the present syntax defined for grammars is not ideal, and the code
// to parse it shouldn't be in this file; I'll fix these problems at
// some point..


bool Grammar::readFile(char const *fname)
{
  FILE *fp = fopen(fname, "r");
  if (!fp) {
    xsyserror("fopen", stringb("opening " << fname));
  }

  // state between lines
  SObjList<Production> lastProductions;      // the last one(s) we created

  char buf[256];
  int line = 0;
  bool ok = true;
  while (fgets(buf, 256, fp)) {
    line++;

    if (!parseLine(buf, lastProductions)) {
      cerr << "error parsing line " << line << endl;
      ok = false;
      break;    // stop after first error
    }
  }

  if (fclose(fp) != 0) {
    xsyserror("fclose");
  }
  
  return ok;
}


// parse a possibly-tagged symbol name into 'name' and 'tag'
// syntax is "tag:name" or just "name"
void parseTaggedName(string &name, string &tag, char const *tagged)
{
  StrtokParse tok(tagged, ":");
  if (tok < 1) {
    xfailure("tagged name consisting only of colons!");
  }
  if (tok > 2) {
    xfailure("tagged name with too many tags");
  }

  if (tok == 1) {
    tag = NULL;
    name = tok[0];
  }
  else {
    tag = tok[0];
    name = tok[1];
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

    // bison-compatible token definition
    if (0==strcmp(tok[0], "%token")) {
      if (tok != 3  &&  tok != 4) {
        cout << "directive should be: %token <symbolName> <code> [<alias>]\n";
        return false;
      }

      return declareToken(tok[1], atoi(tok[2]), tok>3? tok[3] : NULL);
    }

    // action or condition?
    if (0==strcmp(tok[0], "%action") || 0==strcmp(tok[0], "%condition")) {
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

    // some unknown directive
    if (tok[0][0] == '%') {
      cout << "unknown directive: " << tok[0] << endl;
      return false;
    }

    // going to parse a production, so clear the list
    lastProductions.removeAll();

    if (!parseProduction(lastProductions, tok)) {
      return false;    // parse error, message already printed
    }

    // add the productions
    SMUTATE_EACH_PRODUCTION(lastProductions, prod) {
      addProduction(prod.data());
    }

    // ok
    return true;
  }

  catch (xBase &x) {
    cout << "parse error: " << x << endl;
    return false;
  }
}


// parse %action or %condition
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
    xfailure("parseAnAction called with wrong directive!");
    return false;      // silence warning
  }
}


// given the name of a symbol (terminal or nonterminal) as it
// appears in the grammar input file, return the symbol named,
// and the tag (or NULL); returns NULL if there is an error
Symbol *Grammar::parseGrammarSymbol(char const *token, string &tag)
{
  // assume there will be no tag
  tag = NULL;

  // terminal?
  if (findTerminalC(token)) {
    return findTerminal(token);
  }

  // empty string
  else if (0==strcmp(token, "empty")) {
    return &emptyString;
  }

  // nonterminal (has to start with an uppercase letter)
  else if (strchr(token, ':') || isupper(token[0])) {
    string name;
    parseTaggedName(name, tag, token);
    return getOrMakeNonterminal(name);
  }

  // old code for automatically defining new tokens
  //if (token[0] == '"') {
  //  return getOrMakeTerminal(parseQuotedString(token));
  //}

  // error
  else {
    cout << "not a valid symbol (tokens must be declared): " << token << endl;
    return NULL;
  }
}


// given a token sequence representing one or more productions, parse
// them and put the productions into 'prods' (does NOT add them to
// the Grammar's main 'productions' list); return false on parse error
bool Grammar::parseProduction(ProductionList &prods, StrtokParse const &tok)
{
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
      prods.append(prod);

      // start another
      prod = new Production(LHS, leftTag);
    }

    else {
      // terminal or nonterminal
      Symbol *sym = parseGrammarSymbol(tok[i], tag /*out*/);
      if (!sym) {
        return false;
      }

      if (sym == &emptyString) {
        // new policy is to not include it
      }
      else {
        // add it to the production
        prod->append(sym, tag);
      }
    }
  }

  // done, so add the production
  prods.append(prod);
  return true;
}



