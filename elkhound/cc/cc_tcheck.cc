// cc_tcheck.cc
// C++ typechecker, implemented as methods declared in cc.ast

#include "cc.ast.gen.h"     // C++ AST


template <class NODE>
void genericPrintAmbiguities(NODE const *ths, char const *typeName,
                             ostream &os, int indent)
{
  // count the number of alternatives
  int numAlts = 0;
  {
    for (NODE const *p = ths; p != NULL; p = p->ambiguity) {
      numAlts++;
    }
  }

  // print a visually obvious header
  ind(os, indent) << "--------- ambiguous " << typeName << ": "
                  << numAlts << " alternatives ---------\n";

  // walk down the list of ambiguities, printing each one by
  // temporarily nullifying the 'ambiguity' pointer; cast away
  // the constness so I can do the temporary modification
  for (NODE *e = const_cast<NODE*>(ths);
       e != NULL;
       e = e->ambiguity) {
    NODE *tempAmbig = e->ambiguity;
    e->ambiguity = NULL;
    e->debugPrint(os, indent+2);
    e->ambiguity = tempAmbig;
  }

  ind(os, indent) << "--------- end of ambiguous " << typeName
                  << " ---------\n";
}


void Expression::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "Expression", os, indent);
}

void Statement::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "Statement", os, indent);
}


void IDeclarator::printExtras(ostream &os, int indent) const
{
  ind(os, indent) << "stars:";
  FOREACH_ASTLIST(PtrOperator, stars, iter) {
    os << " " << iter.data()->toString();
  }
  os << "\n";
}


string PtrOperator::toString() const
{
  stringBuilder ret;
  ret << (isPtr? "*" : "&");

  if (cv) {
    ret << " " << ::toString(cv);
  }

  return ret;
}
