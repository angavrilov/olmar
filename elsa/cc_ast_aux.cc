// cc_ast_aux.cc            see license.txt for copyright and terms of use
// auxilliary code (debug printing, etc.) for cc.ast

#include "cc.ast.gen.h"     // C++ AST
#include "strutil.h"        // plural


// ------------ generic ambiguity handling ----------------
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


// make sure that all the 'next' fields end up the same
template <class NODE>
void genericCheckNexts(NODE const *main)
{
  for (NODE const *a = main->ambiguity; a != NULL; a = a->ambiguity) {
    xassert(main->next == a->next);
  }
}

// add another ambiguous alternative to 'main', with care given
// to the fact that NODE has 'next' links
template <class NODE>
void genericAddAmbiguity(NODE *main, NODE *alt)
{
  // 'alt' had better not already be on a list (shouldn't be,
  // because it's the RHS argument to merge, which means it's
  // never been yielded to anything)
  xassert(alt->next == NULL);

  // same reasoning for 'ambiguity'
  //xassert(alt->ambiguity == NULL);
  // no, it turns out the RHS could have been yielded if the
  // reduction action is the identity function.. so instead
  // find the last node in the 'alt' list and we'll splice
  // that entire list into 'main's ambiguity list
  NODE *altLast = alt;
  while (altLast->ambiguity) {
    altLast = altLast->ambiguity;

    // the assignment below will only get the first node, so
    // this takes care of the other ones in 'alt'
    altLast->next = main->next;
  }

  if (main->next) {
    // I don't expect 'main' to already be on a list, so I'll
    // make some noise; but I think it will work anyway
    cout << "note: ambiguous " << main->kindName()
         << "node leader is already on a list..\n";
  }

  // if 'main' has been added to a list, add 'alt' also
  alt->next = main->next;

  // finally, prepend 'alt's ambiguity list to 'main's ambiguity list
  altLast->ambiguity = main->ambiguity;
  main->ambiguity = alt;
}


// set the 'next' field of 'main' to 'newNext', with care given
// to possible ambiguities
template <class NODE>
void genericSetNext(NODE *main, NODE *newNext)
{
  // 'main's 'next' should be NULL; if it's not, then it's already
  // on a list, and setting 'next' now will lose information
  xassert(main->next == NULL);
  main->next = newNext;

  // if 'main' has any ambiguous alternatives, set all their 'next'
  // pointers too
  if (main->ambiguity) {
    genericSetNext(main->ambiguity, newNext);     // recursively set them all
  }
}


// TranslationUnit
// TopForm

// ---------------------- Function --------------------
void Function::printExtras(ostream &os, int indent) const
{
  if (funcType) {
    ind(os, indent) << "funcType: " << funcType->toString() << "\n";
  }
}


// ---------------------- MemberInit ----------------------
void MemberInit::printExtras(ostream &os, int indent) const
{
  if (member) {
    ind(os, indent) << "member: refers to " << toString(member->loc) << "\n";
  }       

  if (base) {
    ind(os, indent) << "base: " << base->toCString() << "\n";
  }
}


// Declaration

// ---------------------- ASTTypeId -----------------------
void ASTTypeId::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "ASTTypeId", os, indent);
  
  genericCheckNexts(this);
}

void ASTTypeId::addAmbiguity(ASTTypeId *alt)
{
  genericAddAmbiguity(this, alt);
}

void ASTTypeId::setNext(ASTTypeId *newNext)
{
  genericSetNext(this, newNext);
}


// ------------------------ PQName ------------------------
string targsToString(FakeList<TemplateArgument> const *list)
{
  stringBuilder sb;
  sb << "<";
  int ct=0;
  FAKELIST_FOREACH(TemplateArgument, list, iter) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << iter->argString();
  }
  sb << ">";       
  return sb;
}


string PQName::qualifierString() const
{
  stringBuilder sb;

  PQName const *p = this;
  while (p->isPQ_qualifier()) {
    PQ_qualifier const *q = p->asPQ_qualifierC();
    if (q->qualifier) {
      sb << q->qualifier;

      if (q->targs) {
        sb << targsToString(q->targs);
      }
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
  // leading qualifiers, with template arguments as necessary
  sb << obj.qualifierString();

  // final simple name
  PQName const *final = obj.getUnqualifiedNameC();
  sb << final->getName();
                      
  // template arguments applied to final name
  if (final->isPQ_template()) {
    sb << targsToString(final->asPQ_templateC()->args);
  }

  return sb;
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


PQName const *PQName::getUnqualifiedNameC() const
{                   
  PQName const *p = this;
  while (p->isPQ_qualifier()) {
    p = p->asPQ_qualifierC()->rest;
  }
  return p;
}


//  ------------------- TypeSpecifier ---------------------
void TypeSpecifier::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "TypeSpecifier", os, indent);

  genericCheckNexts(this);
}


void TypeSpecifier::addAmbiguity(TypeSpecifier *alt)
{
  genericAddAmbiguity(this, alt);
}

void TypeSpecifier::printExtras(ostream &os, int indent) const
{
  PRINT_GENERIC(cv);
}


// ------------------- BaseClassSpec ---------------------
void BaseClassSpec::printExtras(ostream &os, int indent) const
{
  if (type) {
    ind(os, indent) << "type: " << type->toCString() << "\n";
  }
}


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
  genericAddAmbiguity(this, alt);
}

void Declarator::setNext(Declarator *newNext)
{
  genericSetNext(this, newNext);
}


PQName const *Declarator::getDeclaratorId() const
{
  return decl->getDeclaratorId();
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

PQName const *D_ptrToMember::getDeclaratorId() const
{
  return base->getDeclaratorId();
}

PQName const *D_grouping::getDeclaratorId() const
{
  return base->getDeclaratorId();
}


IDeclarator *IDeclarator::skipGroups()
{
  if (isD_grouping()) {
    return asD_grouping()->base->skipGroups();
  }
  else {
    return this;
  }
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
  // this does not call 'genericAddAmbiguity' because Statements
  // do not have 'next' fields

  // prepend 'alt' to my list
  const_cast<Statement*&>(alt->ambiguity) = ambiguity;
  const_cast<Statement*&>(ambiguity) = alt;
}


string Statement::lineColString() const
{
  char const *fname;
  int line, col;
  sourceLocManager->decodeLineCol(loc, fname, line, col);

  return stringc << line << ":" << col;
}

string Statement::kindLocString() const
{
  return stringc << kindName() << "@" << lineColString();
}


// Condition
// Handler

// --------------------- Expression ---------------------
void Expression::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "Expression", os, indent);

  genericCheckNexts(this);
}


void Expression::addAmbiguity(Expression *alt)
{
  genericAddAmbiguity(this, alt);
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

  genericSetNext(this, newNext);
}


void Expression::printExtras(ostream &os, int indent) const
{         
  if (type) {
    ind(os, indent) << "type: " << type->toString() << "\n";
  }

  // print type-specific extras
  ASTSWITCHC(Expression, this) {
    ASTCASEC(E_intLit, i) {
      ind(os, indent) << "i: " << i->i << "\n";
    }

    ASTNEXTC(E_floatLit, f) {
      ind(os, indent) << "f: " << f->d << "\n";
    }

    ASTNEXTC(E_stringLit, s) {
      // nothing extra to print since there's no interpretation yet
      PRETEND_USED(s);
    }

    ASTNEXTC(E_charLit, c) {
      ind(os, indent) << "c: " << c->c << "\n";    // prints as an integer
    }

    ASTNEXTC(E_variable, v)
      if (v->var) {
        ind(os, indent) << "var: refers to " << toString(v->var->loc) << "\n";
      }

    ASTNEXTC(E_new, n)
      PRINT_SUBTREE(n->arraySize);

    ASTNEXTC(E_fieldAcc, f)
      if (f->field) {
        ind(os, indent) << "field: refers to " << toString(f->field->loc) << "\n";
      }

    ASTDEFAULTC
      /* do nothing */

    ASTENDCASEC
  }
}


// ExpressionListOpt
// Initializer

// --------------------- Designator ---------------------
void Designator::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "Designator", os, indent);
  
  genericCheckNexts(this);
}

void Designator::addAmbiguity(Designator *alt)
{
  genericAddAmbiguity(this, alt);
}

void Designator::setNext(Designator *newNext)
{
  genericSetNext(this, newNext);
}

// InitLabel

// ------------------- TemplateDeclaration ------------------
void TD_class::printExtras(ostream &os, int indent) const
{
  if (type) {
    ind(os, indent) << "type: " << type->toString() << "\n";
  }
}


// TemplateParameter

// -------------------- TemplateArgument ---------------------
void TemplateArgument::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "TemplateArgument", os, indent);

  genericCheckNexts(this);
}


void TemplateArgument::addAmbiguity(TemplateArgument *alt)
{
  genericAddAmbiguity(this, alt);
}

void TemplateArgument::setNext(TemplateArgument *newNext)
{
  if (next == newNext) {
    return;    // bail if it's already what we want..
  }

  genericSetNext(this, newNext);
}


string TA_type::argString() const
{
  return type->getType()->toString();
}

string TA_nontype::argString() const
{
  return expr->exprToString();
}   


void TemplateArgument::printExtras(ostream &os, int indent) const
{
  if (sarg.hasValue()) {
    ind(os, indent) << "sarg: " << sarg.toString() << "\n";
  }
}
