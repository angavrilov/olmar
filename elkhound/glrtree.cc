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
TreeNode::~TreeNode()
{}


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


string TreeNode::locString() const
{
  TerminalNode const *left = getLeftmostTerminalC();
  if (left) {
    return left->token->loc.toString();
  }
  else {
    return "(?loc)";
  }
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


Reduction const *NonterminalNode::only() const
{
  if (reductions.count() != 1) {
    THROW(XAmbiguity(this));
  }

  return reductions.firstC();
}


int NonterminalNode::onlyProductionIndex() const
{
  return only()->production->prodIndex;
}

TreeNode const *NonterminalNode::getOnlyChild(int childNum) const
{
  return only()->children.nthC(childNum);
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
    reductions.firstC()->printParseTree(attr, os, indent);
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
      red.data()->printParseTree(attr, os, indent);
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
  Reduction const *red = reductions.firstC();
  
  // since some nonterminals derive empty, we walk the list until
  // we find a nonempty entry
  for (int i=0; i < red->children.count(); i++) {
    TerminalNode const *node = red->children.nthC(i)->getLeftmostTerminalC();
    if (node) {
      return node;    // got it
    }
  }
  
  // all children derived empty, so 'this' derives empty
  return NULL;
}


void NonterminalNode::getGroundTerms(SObjList<TerminalNode> &dest) const
{
  // all reductions will yield same sequence (at least I think so!)
  return reductions.firstC()->getGroundTerms(dest);
}


// ---------------------- Reduction -------------------------
Reduction::Reduction(Production const *prod)
  : production(prod)
{}


Reduction::~Reduction()
{}


TreeNode const *Reduction::walkTree(TreeNode::WalkFn func, void *extra) const
{
  // walk children
  SFOREACH_OBJLIST(TreeNode, children, iter) {
    TreeNode const *n = iter.data()->walkTree(func, extra);
    if (n) { return n; }
  }
  return NULL;
}


void Reduction::printParseTree(Attributes const &attr,
                               ostream &os, int indent) const
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


void Reduction::getGroundTerms(SObjList<TerminalNode> &dest) const
{
  SFOREACH_OBJLIST(TreeNode, children, child) {
    child.data()->getGroundTerms(dest);
  }
}


// --------------------- AttrContext -------------------
AttrContext::~AttrContext()
{
  // common case is that red is, in fact, NULL at this point
  if (red != NULL) {
    delete red;
  }
}


Reduction *AttrContext::grabReduction()
{
  Reduction *ret = red;
  red = NULL;
  return ret;
}


// -------------------- XAmbiguity -------------------
STATICDEF string XAmbiguity::makeWhy(NonterminalNode const *n)
{
  stringBuilder sb;
  sb << "Ambiguity at " << n->locString() 
     << " between productions:";

  FOREACH_OBJLIST(Reduction, n->reductions, iter) {
    sb << " (" << iter.data()->production->toString() << ")";
  }

  return sb;
}


XAmbiguity::XAmbiguity(NonterminalNode const *n)
  : xBase(makeWhy(n)),
    node(n)
{}

XAmbiguity::XAmbiguity(XAmbiguity const &obj)
  : xBase(obj),
    DMEMB(node)
{}

XAmbiguity::~XAmbiguity()
{}

