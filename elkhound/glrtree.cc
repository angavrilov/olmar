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


// ------------------- TerminalNode -------------------------
TerminalNode::TerminalNode(Terminal const *t)
  : TreeNode(TERMINAL),
    terminal(t)
{}


TerminalNode::~TerminalNode()
{}


Symbol const *TerminalNode::getSymbolC() const
{
  return terminal;
}


void TerminalNode::printParseTree(ostream &os, int indent) const
{
  // I am a leaf
  IND << terminal->name << endl;
}


// ------------------ NonterminalNode -----------------------
NonterminalNode::NonterminalNode(Reduction *red)
  : TreeNode(NONTERMINAL)
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


Symbol const *NonterminalNode::getSymbolC() const
{
  return getLHS();
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


// ---------------------- Reduction -------------------------
Reduction::Reduction(Production const *prod)
  : production(prod)
{}


Reduction::~Reduction()
{}


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
