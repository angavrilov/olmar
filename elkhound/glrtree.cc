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
  : TreeNode(NONTERMINAL),
    nonterminal(red->production->left)
{
  // add the first reduction
  addReduction(red);
}


NonterminalNode::~NonterminalNode()
{}


void NonterminalNode::addReduction(Reduction *red)
{
  // verify this one is consistent with others
  xassert(red->production->left == nonterminal);

  reductions.append(red);
}


Symbol const *NonterminalNode::getSymbolC() const
{
  return nonterminal;
}


void NonterminalNode::printParseTree(ostream &os, int indent) const
{
  if (reductions.count() == 1) {
    // I am unambiguous
    reductions.nthC(0)->printParseTree(os, indent);
  }

  else {
    // I am ambiguous
    IND << "ALTERNATIVE PARSES for nonterminal " << nonterminal->name << ":\n";
    indent += 2;

    FOREACH_OBJLIST(Reduction, reductions, red) {
      red.data()->printParseTree(os, indent);
    }
  }
}


// ---------------------- Reduction -------------------------
Reduction::Reduction(Production const *prod)
  : production(prod)
{}


Reduction::~Reduction()
{}


void Reduction::printParseTree(ostream &os, int indent) const
{
  // print the production that was used to reduce
  IND << *(production) << endl;

  // print children
  indent += 2;
  SFOREACH_OBJLIST(TreeNode, children, child) {
    child.data()->printParseTree(os, indent);
  }
}
