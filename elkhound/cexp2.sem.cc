// cexp2.sem.cc
// semantics for cexp2.gr

#include "glrtree.h"     // TreeNode
#include "glr.h"         // GLR
#include "lexer2.h"      // Lexer2Token


// -------------------- tree access ---------------
TreeNode const *nodeChild(TreeNode const *node, int which)
{
  return node->asNontermC().only()->children.nthC(which);
}

TreeNode const *hop(TreeNode const *node)
{
  return nodeChild(node, 0);
}

int numChildren(TreeNode const *node)
{
  return node->asNontermC().only()->children.count();
}

bool nodeTypeIs(TreeNode const *node, char const *name)
{
  return node->getSymbolC()->name.equals(name);
}

string nodeString(TreeNode const *node)
{
  Lexer2Token const *t = node->asTermC().token;
  xassert(t->type == L2_NAME || t->type == L2_STRING_LITERAL);
  return t->strValue;
}

int nodeInt(TreeNode const *node)
{
  Lexer2Token const *t = node->asTermC().token;
  xassert(t->type == L2_INT_LITERAL);
  return t->intValue;
}

Lexer2TokenType nodeL2Type(TreeNode const *node)
{
  Lexer2Token const *t = node->asTermC().token;
  return t->type;
}


// ----------------- bindings list ----------------
class Binding {
public:
  string name;
  int value;
  
public:
  Binding(char const *n, int v) : name(n), value(v) {}
  ~Binding();
};

Binding::~Binding()
{}


int lookup(ObjList<Binding> const &bindings, char const *name)
{
  FOREACH_OBJLIST(Binding, bindings, iter) {
    if (iter.data()->name.equals(name)) {
      return iter.data()->value;
    }
  }
  
  xfailure(stringc << "unbound: " << name);
  return 0;  // silence warning
}


// ------------------- tree parsing --------------------
int evalCexp2(TreeNode const *node);
void gatherBindings(ObjList<Binding> &bindings, TreeNode const *node);
int evalExp(ObjList<Binding> const &bindings, TreeNode const *node);


int evalCexp2(TreeNode const *node)
{
  xassert(node->isNonterm() &&
          nodeTypeIs(node, "Input"));

  // testing node access interface
  cout << "top symbol has "
       << node->asNontermC().only()->children.count()
       << " children\n";

  // process bindings
  ObjList<Binding> bindings;
  gatherBindings(bindings, nodeChild(node, 0));

  // evaluate expression
  return evalExp(bindings, nodeChild(node, 2));
}


void gatherBindings(ObjList<Binding> &bindings, TreeNode const *node)
{
  if (nodeTypeIs(node, "VarDefList")) {
    // process the binding here
    gatherBindings(bindings, nodeChild(node, 0));

    if (numChildren(node) == 3) {
      // recursively process rest of list
      gatherBindings(bindings, nodeChild(node, 2));
    }
  }

  else { xassert(nodeTypeIs(node, "VarDef"));
    // this node has a binding
    string name = nodeString(nodeChild(node, 0));
    
    ObjList<Binding> empty;
    int val = evalExp(empty, nodeChild(node, 2));

    bindings.append(new Binding(name, val));
  }
}


int evalExp(ObjList<Binding> const &bindings, TreeNode const *node)
{
  if (numChildren(node) == 1) {
    // name or literal
    node = hop(node);

    if (nodeTypeIs(node, "L2_NAME")) {
      string name = nodeString(node);
      return lookup(bindings, name);
    }

    else { xassert(nodeTypeIs(node, "L2_INT_LITERAL"));
      return nodeInt(node);
    }
  }

  else { xassert(numChildren(node) == 3);
    // operator expression
    int left = evalExp(bindings, nodeChild(node, 0));
    Lexer2TokenType op = nodeL2Type(hop(nodeChild(node, 1)));
    int right = evalExp(bindings, nodeChild(node, 2));

    switch (op) {
      case L2_STAR:    return left * right;
      case L2_SLASH:   return left / right;
      case L2_PERCENT: return left % right;
      case L2_PLUS:    return left + right;
      case L2_MINUS:   return left - right;

      // etc..
      default:         return left + right;
    }
  }
}


#ifdef TEST_CEXP2_SEM

int main(int argc, char *argv[])
{
  TRACE_ARGS();
  
  if (argc < 2) {
    cout << "usage: " << argv[0] << " [-tr flags] input-file\n";
    return 0;
  }

  GLR glr;
  Lexer2 lexer2;
  glr.glrParseFrontEnd(lexer2, "cexp2.gr", argv[1]);

  int v = evalCexp2(glr.getParseTree());
  cout << "evaluation result: " << v << endl;

  return 0;
}

#endif // TEST_CEXP2_SEM
