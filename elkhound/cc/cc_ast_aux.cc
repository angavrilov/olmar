// cc_ast_aux.cc            see license.txt for copyright and terms of use
// auxilliary code (debug printing, etc.) for cc.ast

#include "cc.ast.gen.h"     // C++ AST
#include "strutil.h"        // plural


// ------------ generic ambiguity printing ----------------
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
  int ct=0;
  for (NODE *e = const_cast<NODE*>(ths);
       e != NULL;
       e = e->ambiguity) {
    if (ct++ > 0) {
      ind(os, indent) << "---- or ----\n";
    }

    NODE *tempAmbig = e->ambiguity;
    const_cast<NODE*&>(e->ambiguity) = NULL;
    e->debugPrint(os, indent+2);
    const_cast<NODE*&>(e->ambiguity) = tempAmbig;
  }

  ind(os, indent) << "--------- end of ambiguous " << typeName
                  << " ---------\n";
}


// TranslationUnit
// TopForm
// Function
// MemberInit
// Declaration
// ASTTypeId

// ------------------------ PQName ------------------------
string PQName::qualifierString() const
{
  stringBuilder sb;
  
  PQName const *p = this;
  while (p->isPQ_qualifier()) {
    PQ_qualifier const *q = p->asPQ_qualifierC();
    if (q->qualifier) {
      sb << q->qualifier;
    }         
    else {
      // for a NULL qualifier, don't print anything; it means
      // there was a leading "::" with no explicit qualifier,
      // and I'll use similar syntax on output
    }
    sb << "::";
    
    p = q->rest;
  }
  return sb;
}

stringBuilder& operator<< (stringBuilder &sb, PQName const &obj)
{
  return sb << obj.qualifierString() << obj.getName();
}

string PQName::toString() const
{
  stringBuilder sb;
  sb << *this;
  return sb;
}


StringRef PQ_qualifier::getName() const
{
  return rest->getName();
}

StringRef PQ_name::getName() const
{
  return name;
}

StringRef PQ_operator::getName() const
{
  return fakeName;
}

StringRef PQ_template::getName() const
{
  return name;
}


PQName const *PQName::getUnqualifiedName() const
{                   
  PQName const *p = this;
  while (p->isPQ_qualifier()) {
    p = p->asPQ_qualifierC()->rest;
  }
  return p;
}


//  ------------------- TypeSpecifier ---------------------
void TypeSpecifier::printExtras(ostream &os, int indent) const
{
  PRINT_GENERIC(cv);
}


// BaseClass
// MemberList
// Member

// ---------------------- Enumerator ------------------
void Enumerator::printExtras(ostream &os, int indent) const
{
  if (var) {
    ind(os, indent) << "var: " 
      << toString(var->flags) << (var->flags? " " : "")
      << var->type->toCString(var->name) << "\n";
    PRINT_GENERIC(enumValue);
  }
}


// ---------------------- Declarator ---------------------------
void Declarator::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "Declarator", os, indent);
  
  // check 'next' fields
  for (Declarator *d = ambiguity; d != NULL; d = d->ambiguity) {
    xassert(this->next == d->next);
  }
}


void Declarator::addAmbiguity(Declarator *alt)
{ 
  // see Expression::addAmbiguity for comments; this code is
  // mostly copied from there

  xassert(alt->next == NULL);
  
  if (next) {
    cout << "note: ambiguous declarator leader is already on a list...\n";
  }
  
  alt->next = next;
  
  alt->ambiguity = ambiguity;
  ambiguity = alt;
}


void Declarator::setNext(Declarator *newNext)
{
  // see Expression::setNext

  xassert(next == NULL);
  next = newNext;

  if (ambiguity) {
    ambiguity->setNext(newNext);
  }
}


void Declarator::printExtras(ostream &os, int indent) const
{
  if (var) {
    ind(os, indent) << "var: "
      << toString(var->flags) << (var->flags? " " : "")
      << var->type->toCString(var->name);

    if (var->overload) {
      int n = var->overload->count();
      os << " (" << n << " " << plural(n, "overloading") << ")";
    }

    os << "\n";
  }
}


// --------------------- IDeclarator ---------------------------
PQName const *D_name::getDeclaratorId() const
{
  return name;
}

PQName const *D_pointer::getDeclaratorId() const
{
  return base->getDeclaratorId();
}

PQName const *D_func::getDeclaratorId() const
{
  return base->getDeclaratorId();
}

PQName const *D_array::getDeclaratorId() const
{
  return base->getDeclaratorId();
}

PQName const *D_bitfield::getDeclaratorId() const
{
  // the ability to simply return 'name' here is why bitfields contain
  // a PQName instead of just a StringRef
  return name;
}


// ExceptionSpec
// OperatorDeclarator

// ---------------------- Statement --------------------
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
  // relaxation: The syntax
  //   tok = strtok(((void *)0) , delim);
  // provokes a double-add, where 'next' is the same both
  // times.  I think this is because we merge a little
  // later than usual due to unexpected state splitting.
  // I might try to investigate this more carefully at a
  // later time, but for now..
  if (next == newNext) {
    return;    // bail if it's already what we want..
  }

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

  // print type-specific extras
  ASTSWITCHC(Expression, this) {
    ASTCASEC(E_variable, v)
      if (v->var) {
        ind(os, indent) << "var: refers to " << v->var->loc.toString() << "\n";
      }

    ASTNEXTC(E_new, n)
      PRINT_SUBTREE(n->arraySize);

    ASTDEFAULTC
      /* do nothing */

    ASTENDCASEC
  }
}


// ExpressionListOpt
// Initializer
// InitLabel
// TemplateDeclaration
// TemplateParameter

// -------------------- TemplateArgument ---------------------
string TA_type::argString() const
{
  return type->getType()->toString();
}
