// glrtree.cc
// code for glrtree.h

#include "glrtree.h"     // this module


// convenient indented output
ostream &doIndent(ostream &os, int i)
{
  while (i--) {
    os << " ";
  }
  return os;
}

#define IND doIndent(os, indent)


// ---------------------- TreeNode --------------------------
int TreeNode::numTreeNodesAllocd=0;
int TreeNode::maxTreeNodesAllocd=0;


TreeNode::TreeNode()
{
  INC_HIGH_WATER(numTreeNodesAllocd, maxTreeNodesAllocd);
}


TreeNode::~TreeNode()
{
  numTreeNodesAllocd--;
}


TerminalNode const &TreeNode::asTermC() const
{
  xassert(isTerm());
  return reinterpret_cast<TerminalNode const &>(*this);

  // random note: I don't use dynamic_cast because I do not
  // like RTTI, and I definitely do not trust g++'s
  // implementation of it
}


NonterminalNode const &TreeNode::asNontermC() const
{
  xassert(isNonterm());
  return reinterpret_cast<NonterminalNode const &>(*this);
}


TreeNode const *TreeNode::walkTree(WalkFn func, void *extra) const
{
  if (func(this, extra)) {
    return this;
  }
  else {
    return NULL;
  }
}  


string TreeNode::unparseString() const
{
  // get terms
  SObjList<TerminalNode> terms;
  getGroundTerms(terms);

  // render as string
  stringBuilder sb;

  int ct=0;
  SFOREACH_OBJLIST(TerminalNode, terms, term) {
    if (ct++ > 0) {
      sb << " ";      // token separator
    }
    sb << term.data()->token->unparseString();
  }

  return sb;
}


string terminalLocString(TerminalNode const *term)
{
  if (term) {
    return term->token->loc.toString();
  }
  else {
    return "(?loc)";
  }
}


string TreeNode::locString() const
{
  return terminalLocString(getLeftmostTerminalC());
}


STATICDEF void TreeNode::printAllocStats()
{
  cout << "tree nodes: " << numTreeNodesAllocd
       << ", max tree nodes: " << maxTreeNodesAllocd
       << endl;
}     


// ------------------- TerminalNode -------------------------
TerminalNode::TerminalNode(Lexer2Token const *tk, Terminal const *tc)
  : token(tk),
    terminalClass(tc)
{}


TerminalNode::~TerminalNode()
{}


Symbol const *TerminalNode::getSymbolC() const
{
  return terminalClass;
}


AttrValue TerminalNode::getAttrValue(AttrName name) const      
{
  xfailure("getAttrValue: terminals do not have attributes");
}

void TerminalNode::setAttrValue(AttrName name, AttrValue value)
{
  xfailure("setAttrValue: terminals do not have attributes");
}


void TerminalNode::printParseTree(ostream &os, int indent) const
{
  // I am a leaf
  IND << token->toString() << endl;
}


void TerminalNode::ambiguityReport(ostream &) const
{
  // no ambiguities at a terminal!
}


TerminalNode const *TerminalNode::getLeftmostTerminalC() const
{
  // base case of recursion
  return this;
}


void TerminalNode::getGroundTerms(SObjList<TerminalNode> &dest) const
{
  dest.append(const_cast<TerminalNode*>(this));
    // highly nonideal constness...
}

int TerminalNode::numGroundTerms() const
{
  return 1;
}


// ------------------ NonterminalNode -----------------------
NonterminalNode::NonterminalNode(Reduction *red)
{
  // add the first reduction
  addReduction(red);
}


NonterminalNode::~NonterminalNode()
{}


void NonterminalNode::addReduction(Reduction *red)
{
  // verify this one is consistent with others
  if (reductions.isNotEmpty()) {
    xassert(red->production->left == getLHS());
  }

  reductions.append(red);
}


Nonterminal const *NonterminalNode::getLHS() const
{
  return reductions.firstC()->production->left;
}


Reduction const *NonterminalNode::onlyC() const
{
  if (reductions.count() != 1) {
    THROW(XAmbiguity(this, "in only()"));
  }

  return reductions.firstC();
}


int NonterminalNode::onlyProductionIndex() const
{
  return onlyC()->production->prodIndex;
}

TreeNode const *NonterminalNode::getOnlyChild(int childNum) const
{
  return onlyC()->children.nthC(childNum);
}

Lexer2Token const &NonterminalNode::getOnlyChildToken(int childNum) const
{
  Lexer2Token const *ret = getOnlyChild(childNum)->asTermC().token;
  xassert(ret);
  return *ret;
}


Symbol const *NonterminalNode::getSymbolC() const
{
  return getLHS();
}


AttrValue NonterminalNode::getAttrValue(AttrName name) const
{
  // get the first child's opinion
  AttrValue val = reductions.firstC()->getAttrValue(name);

  // if there are alternative parses, for now I'm keeping the old
  // policy that they must agree on the value of all attributes
  for (int i=1; i<reductions.count(); i++) {
    // get another child's opinion
    AttrValue val2 = reductions.nthC(i)->getAttrValue(name);
    if (val != val2) {
      THROW(XAmbiguity(this, "unequal alternative child attributes"));
    }
  }

  // consensus!
  return val;
}

void NonterminalNode::setAttrValue(AttrName name, AttrValue value)
{
  // this is mainly because I don't have inherited attributes, so I
  // should only ever set attributes in reductions that are being
  // built at the time, i.e. there is no containing NonterminalNode
  // for them
  xfailure("setAttrValue: setting of attributes via Nonterminal isn't supported");
}


TreeNode const *NonterminalNode::walkTree(WalkFn func, void *extra) const
{
  TreeNode const *n;

  // me
  n = TreeNode::walkTree(func, extra);
  if (n) { return n; }

  // alternatives for children; for now, just walk all alternatives
  // equally
  FOREACH_OBJLIST(Reduction, reductions, red) {
    n = red.data()->walkTree(func, extra);
    if (n) { return n; }
  }
  return NULL;
}


void NonterminalNode::printParseTree(ostream &os, int indent) const
{
  int parses = reductions.count();
  if (parses == 1) {
    // I am unambiguous
    reductions.firstC()->printParseTree(os, indent);
  }

  else {
    // I am ambiguous
    IND << parses << " ALTERNATIVE PARSES for nonterminal "
        << getLHS()->name << ":\n";
    indent += 2;

    int ct=0;
    FOREACH_OBJLIST(Reduction, reductions, red) {
      ct++;
      IND << "---- alternative " << ct << " ----\n";
      red.data()->printParseTree(os, indent);
    }
  }
}


void NonterminalNode::ambiguityReport(ostream &os) const
{
  // am I ambiguous?
  if (reductions.count() > 1) {
    // we want to print where this occurs in the input, so get
    // the leftmost token of the first interpretation (which will
    // be the same as leftmost in other interpretations)
    TerminalNode const *leftmost = getLeftmostTerminalC();
    if (leftmost == NULL) {
      // don't have location info if there are no terminals...
      os << "empty string (loc?) can be";
    }
    else {
      os << "line " << leftmost->token->loc.line
         << ", col " << leftmost->token->loc.col
         << " \"" << unparseString()
         << "\" : " << getLHS()->name
         << " can be";
    }

    // print alternatives
    int ct=0;
    FOREACH_OBJLIST(Reduction, reductions, red) {
      if (ct++ > 0) {
        os << " or";
      }
      os << " " << red.data()->production->rhsString();
    }

    os << endl;
  }

  // are any of my children ambiguous?
  FOREACH_OBJLIST(Reduction, reductions, red) {
    red.data()->ambiguityReport(os);
  }

}


TerminalNode const *NonterminalNode::getLeftmostTerminalC() const
{
  // all reductions (if there are more than one) will have same
  // answer for this question
  return reductions.firstC()->getLeftmostTerminalC();
}


void NonterminalNode::getGroundTerms(SObjList<TerminalNode> &dest) const
{
  // all reductions will yield same sequence (at least I think so!)
  return reductions.firstC()->getGroundTerms(dest);
}

int NonterminalNode::numGroundTerms() const
{
  return reductions.firstC()->numGroundTerms();
}


// ---------------------- Reduction -------------------------
Reduction::Reduction(Production const *prod)
  : production(prod)
{}


Reduction::~Reduction()
{}


AttrValue Reduction::getAttrValue(AttrName name) const
{                  
  // hack...
  if (0==strcmp(name, "numGroundTerms")) {
    return numGroundTerms();
  }

  return attr.get(name);
}

void Reduction::setAttrValue(AttrName name, AttrValue value)
{
  attr.set(name, value);
}


TreeNode const *Reduction::walkTree(TreeNode::WalkFn func, void *extra) const
{
  // walk children
  SFOREACH_OBJLIST(TreeNode, children, iter) {
    TreeNode const *n = iter.data()->walkTree(func, extra);
    if (n) { return n; }
  }
  return NULL;
}


void Reduction::printParseTree(ostream &os, int indent) const
{
  // print the production that was used to reduce
  // debugging: print address too, as a clumsy uniqueness identifier
  IND << *(production)
      << "   %attr " << attr
      //<< " [" << (void*)production << "]"
      << endl;

  // print children
  indent += 2;
  SFOREACH_OBJLIST(TreeNode, children, child) {
    child.data()->printParseTree(os, indent);
  }
}


void Reduction::ambiguityReport(ostream &os) const
{
  SFOREACH_OBJLIST(TreeNode, children, child) {
    child.data()->ambiguityReport(os);
  }
}


string Reduction::locString() const
{
  return terminalLocString(getLeftmostTerminalC());
}


TerminalNode const *Reduction::getLeftmostTerminalC() const
{
  // since some nonterminals derive empty, we walk the list until
  // we find a nonempty entry
  for (int i=0; i < children.count(); i++) {
    TerminalNode const *node = children.nthC(i)->getLeftmostTerminalC();
    if (node) {
      return node;    // got it
    }
  }
  
  // all children derived empty, so 'this' derives empty
  return NULL;
}


void Reduction::getGroundTerms(SObjList<TerminalNode> &dest) const
{
  SFOREACH_OBJLIST(TreeNode, children, child) {
    child.data()->getGroundTerms(dest);
  }
}

int Reduction::numGroundTerms() const
{
  int sum = 0;
  SFOREACH_OBJLIST(TreeNode, children, child) {
    sum += child.data()->numGroundTerms();
  }
  return sum;
}


// -------------------- ParseTree -------------
ParseTree::ParseTree()
  : treeTop(NULL),
    treeNodes()
{}

ParseTree::~ParseTree()
{}


// --------------------- AttrContext -------------------
AttrContext::AttrContext(Reduction *r1, Reduction *r2)
{
  red[0] = r1;
  red[1] = r2;
}


Reduction const &AttrContext::reductionC(int which) const
{
  xassert(which==0 || which==1);
  xassert(red[which] != NULL);
  return *(red[which]);
}


// -------------------- XAmbiguity -------------------
STATICDEF string XAmbiguity::makeWhy(NonterminalNode const *n, char const *m)
{
  stringBuilder sb;
  sb << "Ambiguity (" << m << ") at " << n->locString()
     << " between productions:";

  FOREACH_OBJLIST(Reduction, n->reductions, iter) {
    sb << " (" << iter.data()->production->toString() << ")";
  }

  return sb;
}


XAmbiguity::XAmbiguity(NonterminalNode const *n, char const *m)
  : xBase(makeWhy(n, m)),
    node(n),
    message(m)
{}

XAmbiguity::XAmbiguity(XAmbiguity const &obj)
  : xBase(obj),
    DMEMB(node),
    DMEMB(message)
{}

XAmbiguity::~XAmbiguity()
{}

