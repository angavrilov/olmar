// glrtree.cc
// code for glrtree.h

#include "glrtree.h"     // this module
#include "lexer2.h"      // Lexer2Token


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
TerminalNode::TerminalNode(Lexer2Token const *tk, Terminal const *tc)
  : TreeNode(TERMINAL),
    token(tk),
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


string TerminalNode::unparseString() const
{
  return token->unparseString();
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


string NonterminalNode::unparseString() const
{
  // all reductions will yield same string (at least I think so!)
  return reductions.firstC()->unparseString();
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


void Reduction::ambiguityReport(ostream &os) const
{
  SFOREACH_OBJLIST(TreeNode, children, child) {
    child.data()->ambiguityReport(os);
  }
}


string Reduction::unparseString() const
{
  stringBuilder sb;

  int ct=0;
  SFOREACH_OBJLIST(TreeNode, children, child) {
    string childString = child.data()->unparseString();
    if (childString.length() == 0) {
      // it was a nonterminal that derived empty
      // (I'm being anal about extra spaces here and there... :)  )
      continue;
    }

    if (ct++ > 0) {
      sb << " ";
    }
    sb << childString;
  }

  return sb;
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
