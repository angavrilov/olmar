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
    const_cast<NODE*&>(e->ambiguity) = NULL;
    e->debugPrint(os, indent+2);
    const_cast<NODE*&>(e->ambiguity) = tempAmbig;
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


void Expression::addAmbiguity(Expression *alt)
{   
  // 'alt' had better not already be on a list (shouldn't be,
  // because it's the RHS argument to merge, which means it's
  // never been yielded to anything)
  xassert(alt->next == NULL);

  if (next) {
    // I don't expect to already be on a list myself, so I'll
    // make some noise; but I think it will work anyway
    cout << "note: ambiguous expression leader is already on a list..\n";
  }

  // if I have been added to a a list, add 'alt' also
  const_cast<Expression*&>(alt->next) = next;
  
  // finally, prepend 'alt' to my ambiguity list
  const_cast<Expression*&>(alt->ambiguity) = ambiguity;
  const_cast<Expression*&>(ambiguity) = alt;
}

void Expression::setNext(Expression *newNext)
{
  // my 'next' should be NULL; if it's not, then I'm already
  // on a list, and setting 'next' now will lose information
  xassert(next == NULL);
  const_cast<Expression*&>(next) = newNext;

  // if I've got any ambiguous alternatives, set all their 'next'
  // pointers too
  if (ambiguity) {
    ambiguity->setNext(newNext);    // recursively set them all
  }
}


void Statement::addAmbiguity(Statement *alt)
{
  // prepend 'alt' to my list
  const_cast<Statement*&>(alt->ambiguity) = ambiguity;
  const_cast<Statement*&>(ambiguity) = alt;
}


void IDeclarator::printExtras(ostream &os, int indent) const
{
  ind(os, indent) << "stars:";
  FAKELIST_FOREACH(PtrOperator, stars, iter) {
    os << " " << iter->toString();
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
