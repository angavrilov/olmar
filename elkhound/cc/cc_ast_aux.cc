// cc_ast_aux.cc            see license.txt for copyright and terms of use
// auxilliary code (debug printing, etc.) for cc.ast

#include "cc.ast.gen.h"     // C++ AST


// TranslationUnit
// TopForm
// Function
// MemberInit
// Declaration
// ASTTypeId

// ------------------------ PQName ------------------------
void PQName::printExtras(ostream &os, int indent) const
{
  ind(os, indent) << "qualifiers: " << qualifierString() << "\n";
}


string PQName::qualifierString() const
{
  stringBuilder sb;
  SFOREACH_OBJLIST(char const, qualifiers, iter) {
    char const *q = iter.data();
    if (q) {
      sb << q;
    }
    else {
      // for a NULL qualifier, don't print anything; it means
      // there was a leading "::" with no explicit qualifier,
      // and I'll use similar syntax on output
    }
    sb << "::";
  }
  return sb;
}

stringBuilder& operator<< (stringBuilder &sb, PQName const &obj)
{
  return sb << obj.qualifierString() << obj.name;
}


//  ------------------- TypeSpecifier ---------------------
void TypeSpecifier::printExtras(ostream &os, int indent) const
{
  PRINT_GENERIC(cv);
}


// BaseClass
// MemberList
// Member
// Enumerator

// ---------------------- Declarator ---------------------------
void Declarator::printExtras(ostream &os, int indent) const
{                    
  if (var) {
    ind(os, indent) << "var: " 
      << toString(var->flags) << (var->flags? " " : "")
      << var->type->toCString(var->name) << "\n";
  }
}


// --------------------- IDeclarator ---------------------------
void IDeclarator::printExtras(ostream &os, int indent) const
{
  ind(os, indent) << "stars:";
  FAKELIST_FOREACH(PtrOperator, stars, iter) {
    os << " " << iter->toString();
  }
  os << "\n";
}


// -------------------- PtrOperator --------------------
string PtrOperator::toString() const
{
  stringBuilder ret;
  ret << (isPtr? "*" : "&");

  if (cv) {
    ret << " " << ::toString(cv);
  }

  return ret;
}


// ExceptionSpec
// OperatorDeclarator

// ---------------------- Statement --------------------
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

void Statement::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "Statement", os, indent);
}


void Statement::addAmbiguity(Statement *alt)
{
  // prepend 'alt' to my list
  const_cast<Statement*&>(alt->ambiguity) = ambiguity;
  const_cast<Statement*&>(ambiguity) = alt;
}


// Condition
// Handler

// --------------------- Expression ---------------------
void Expression::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "Expression", os, indent);

  // make sure that all the 'next' fields end up the same
  for (Expression *e = ambiguity; e != NULL; e = e->ambiguity) {
    xassert(this->next == e->next);
  }
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


void Expression::printExtras(ostream &os, int indent) const
{         
  if (type) {
    ind(os, indent) << "type: " << type->toString() << "\n";
  }
}


// ExpressionListOpt
// Initializer
// InitLabel
