// cc_ast_aux.cc            see license.txt for copyright and terms of use
// auxilliary code (debug printing, etc.) for cc.ast

#include "strutil.h"        // plural
#include "generic_aux.h"    // C++ AST, and genericPrintAmbiguities, etc.
#include "cc_ast_aux.h"     // class ASTTemplVisitor


// ---------------------- ASTTemplVisitor ----------------------
void ASTTemplVisitor::visitTemplateDeclaration_oneTempl(Variable *var0) {
  xassert(var0);
  xassert(var0->templateInfo());

  // run a sub-traversal of the AST instantiation
  if (var0->funcDefn) {
    var0->funcDefn->traverse(*this);
  }
  if (var0->type->isCompoundType() && var0->type->asCompoundType()->syntax) {
    var0->type->asCompoundType()->syntax->traverse(*this);
  }

  // this block can be removed if it ever really causes problems
  if (var0->type->isFunctionType()) {
    if (var0->funcDefn) {
      // if a function template was declared before being defined,
      // unify the types of the two resulting variables so that
      // external calls get matched with internal references to
      // parameters
      //
      // UPDATE: I now make sure that the definition re-uses the
      // variable of the declaration so this is not necessary
      xassert(var0 == var0->funcDefn->nameAndParams->var);
    } else {
      // FIX: make this a user warning?
//        USER_WARNING
//          (var0->loc, " Declaration of a template [specialization?] without a definition");
    }
  } else {
    xassert(!var0->funcDefn);
  }
}

// sm: added this layer to handle new design with specializations
// as a middle layer between primaries and instantiations
void ASTTemplVisitor::visitTemplateDeclaration_oneContainer
  (Variable *container)
{
  TemplateInfo *ti = container->templateInfo();
  xassert(ti);

  if (ti->isCompleteSpec()) {
    // visit as template and bail
    visitTemplateDeclaration_oneTempl(container);
    return;
  }

  // visit the container itself, if desired
  if (primariesAndPartials) {
    visitTemplateDeclaration_oneTempl(container);
  }

  // visit each instantiation
  SFOREACH_OBJLIST_NC(Variable, ti->instantiations, iter) {
    visitTemplateDeclaration_oneTempl(iter.data());
  }
}

bool ASTTemplVisitor::visitTemplateDeclaration(TemplateDeclaration *obj)
{
  // visit all the template instantiations
  Variable *tinfoVar = NULL;
  // FIX: should I do anything here for TD_tmember?
  if (obj->isTD_func()) {
    Declarator *decltor = obj->asTD_func()->f->nameAndParams;
    if (hasBeenTchecked) xassert(decltor->var);
    tinfoVar = decltor->var;
    // this fails for function members of template classes
    //   xassert(tinfoVar);
  } else if (obj->isTD_proto()) {
    FakeList<Declarator> *decllist = obj->asTD_proto()->d->decllist;
    xassert(decllist->count() == 1);
    Declarator *decltor = decllist->first();
    if (hasBeenTchecked) xassert(decltor->var);
    tinfoVar = decltor->var;
    // this fails for var members of template classes
    //      xassert(tinfoVar);
  } else if (obj->isTD_class() && obj->asTD_class()->spec->isTS_classSpec()) {
    TS_classSpec *ts = obj->asTD_class()->spec->asTS_classSpec();
    // ts->ctype can be NULL if there was an error: in/0027.cc:ERROR(1)
    if (ts->ctype) {
      tinfoVar = ts->ctype->asCompoundType()->getTypedefVar();
      // I think this will fail for class members of template
      // classes, but I'll leave it until it does.
      xassert(tinfoVar);
    }
  }

  if (tinfoVar) {
    TemplateInfo *tinfo = tinfoVar->templateInfo();
    // unless we are at the primary, do nothing
    if (!tinfo || !tinfo->isPrimary()) return false;

    // don't visit the instantiations of the same primary twice
    if (primaryTemplateInfos.contains(tinfo)) {
      return false;             // prune; note: all returns from this method return false
    }
    primaryTemplateInfos.add(tinfo);

    // look in the primary (as a container)
    visitTemplateDeclaration_oneContainer(tinfoVar);
    
    // look in the specializations (as containers)
    SFOREACH_OBJLIST_NC(Variable, tinfo->specializations, iter) {
      visitTemplateDeclaration_oneContainer(iter.data());
    }
  }

  return false;                 // prune the walk here; don't decend into template definitions
}


// ---------------------- refersTo ----------------------
// Nominally "refers to <loc>", but with additional information
// about "using declaration" aliasing.  This is an example of how
// the aliasing complicates what used to be a simple process,
// in this case getting a unique name for an entity.  We'll see
// how much more of this I can take before I implement some global
// de-aliasing measure.
//
// Now that I'm using Env::storeVar, the AST shouldn't have any
// alias pointers in it.  But this method remains so I can do things
// like grepping through printTypedAST output for stray aliases.
//
// 1/15/04: Modified to tolerate NULL 'v' values, and to print types,
// since Daniel and I wanted to see addtional information while
// debugging a tricky cc_qual issue.  The result is more verbose
// but the extra information is probably worth it.
string refersTo(Variable *v)
{
  if (!v) {
    return "NULL";
  }

  stringBuilder sb;
  sb << v->toString();
  sb << ", at " << toString(v->loc);
  if (v->usingAlias) {
    sb << ", alias of " << toString(v->skipAlias()->loc);
  }
  sb << stringf(" (0x%08X)", (long)v);
  return sb;
}


// TranslationUnit

// ---------------------- TopForm --------------------
void TopForm::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "TopForm", os, indent);
}


void TopForm::addAmbiguity(TopForm *alt)
{
  // this does not call 'genericAddAmbiguity' because TopForms do not
  // have 'next' fields

  // prepend 'alt' to my list
  xassert(alt->ambiguity == NULL);
  alt->ambiguity = ambiguity;
  ambiguity = alt;
}


bool TopForm::hasImplicitInt(TS_simple *&implIntSpec, Declarator *&declarator)
{
  if (isTF_decl()) {
    return asTF_decl()->decl->hasImplicitInt(implIntSpec, declarator);
  }
  return false;
}


// ---------------------- Function --------------------
void Function::printExtras(ostream &os, int indent) const
{
  if (funcType) {
    ind(os, indent) << "funcType: " << funcType->toString() << "\n";
  }
  ind(os, indent) << "receiver: " << refersTo(receiver) << "\n";
}


SourceLoc Function::getLoc() const
{
  return nameAndParams->getLoc();
}


// ---------------------- MemberInit ----------------------
void MemberInit::printExtras(ostream &os, int indent) const
{
  if (member) {
    ind(os, indent) << "member: " << refersTo(member) << "\n";
  }

  if (base) {
    ind(os, indent) << "base: " << base->toString() << "\n";
  }

  if (ctorVar) {
    ind(os, indent) << "ctorVar: " << refersTo(ctorVar) << "\n";
  }
}


// ----------------------- Declaration ----------------------
bool Declaration::hasImplicitInt(TS_simple *&implIntSpec, Declarator *&declarator)
{
  if (spec->isTS_simple() && spec->asTS_simple()->id == ST_IMPLINT) {
    implIntSpec = spec->asTS_simple();
    declarator = decllist->first();
    xassert(declarator);
    return true;
  }
  return false;
}


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


bool ASTTypeId::hasImplicitInt(TS_simple *&implIntSpec, Declarator *&declarator)
{
  if (spec->isTS_simple() && spec->asTS_simple()->id == ST_IMPLINT) {
    implIntSpec = spec->asTS_simple();
    declarator = decl;
    xassert(declarator);
    return true;
  }
  return false;
}


// ------------------------ PQName ------------------------
string targsToString(ASTList<TemplateArgument> const &list)
{
  stringBuilder sb;
  sb << "<";
  int ct=0;
  FOREACH_ASTLIST(TemplateArgument, list, iter) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << iter.data()->argString();
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

      if (q->targs.isNotEmpty()) {
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
void TypeSpecifier::printExtras(ostream &os, int indent) const
{
  PRINT_GENERIC(cv);
}


// disambiguation for cppstd 14.1 para 3
bool TypeSpecifier::canBeTypeParam() const
{
  if (isTS_elaborated() &&
      asTS_elaboratedC()->keyword == TI_CLASS) {
    return true;
  }

  if (isTS_name() &&
      asTS_nameC()->typenameUsed) {
    return true;
  }

  return false;
}


// ------------------- BaseClassSpec ---------------------
void BaseClassSpec::printExtras(ostream &os, int indent) const
{
  if (type) {
    ind(os, indent) << "type: " << type->toString() << "\n";
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
      << var->toString() << "\n";
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


PQName const *Declarator::getDeclaratorIdC() const
{
  return decl->getDeclaratorIdC();
}


SourceLoc Declarator::getLoc() const
{
  return decl->loc;
}


void Declarator::printExtras(ostream &os, int indent) const
{
  if (var) {
    ind(os, indent) << "var: "
      << toString(var->flags) << (var->flags? " " : "")
      << var->toString();

    if (var->overload) {
      int n = var->overload->count();
      os << " (" << n << " " << plural(n, "overloading") << ")";
    }

    os << "\n";
  }
  
  ind(os, indent) << "context = " << toString(context) << "\n";
}


// --------------------- IDeclarator ---------------------------
PQName const *D_name::getDeclaratorIdC() const
{
  return name;
}

PQName const *D_pointer::getDeclaratorIdC() const
{
  return base->getDeclaratorIdC();
}

PQName const *D_func::getDeclaratorIdC() const
{
  return base->getDeclaratorIdC();
}

PQName const *D_array::getDeclaratorIdC() const
{
  return base->getDeclaratorIdC();
}

PQName const *D_bitfield::getDeclaratorIdC() const
{
  // the ability to simply return 'name' here is why bitfields contain
  // a PQName instead of just a StringRef
  return name;
}

PQName const *D_ptrToMember::getDeclaratorIdC() const
{
  return base->getDeclaratorIdC();
}

PQName const *D_grouping::getDeclaratorIdC() const
{
  return base->getDeclaratorIdC();
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
  xassert(alt->ambiguity == NULL);
  alt->ambiguity = ambiguity;
  ambiguity = alt;
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


bool Statement::hasImplicitInt(TS_simple *&implIntSpec, Declarator *&declarator)
{
  if (isS_decl()) {
    return asS_decl()->decl->hasImplicitInt(implIntSpec, declarator);
  }
  return false;
}


// ----------------------- Condition ----------------------
void Condition::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "Condition", os, indent);
}


void Condition::addAmbiguity(Condition *alt)
{
  // this does not call 'genericAddAmbiguity' because Conditions
  // do not have 'next' fields

  // prepend 'alt' to my list
  xassert(alt->ambiguity == NULL);
  alt->ambiguity = ambiguity;
  ambiguity = alt;
}


// ----------------------- Handler ----------------------
bool Handler::isEllipsis() const
{
  return typeId->spec->isTS_simple() &&
         typeId->spec->asTS_simple()->id == ST_ELLIPSIS;
}


// --------------------- Expression ---------------------
void Expression::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "Expression", os, indent);
    
  // old
  //genericCheckNexts(this);
}


void Expression::addAmbiguity(Expression *alt)
{
  // it turns out the RHS could have been yielded if the
  // reduction action is the identity function.. so instead
  // find the last node in the 'alt' list and we'll splice
  // that entire list into 'main's ambiguity list
  Expression *altLast = alt;
  while (altLast->ambiguity) {
    altLast = altLast->ambiguity;
  }

  // finally, prepend 'alt's ambiguity list to 'this's ambiguity list
  altLast->ambiguity = this->ambiguity;
  this->ambiguity = alt;

  #if 0     // old; from when I had lists of Expressions
  genericAddAmbiguity(this, alt);
  #endif // 0
}

#if 0     // old; from when I had lists of Expressions
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
#endif // 0


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
      ind(os, indent) << "var: " << refersTo(v->var) << "\n";

    ASTNEXTC(E_constructor, c)
      ind(os, indent) << "ctorVar: " << refersTo(c->ctorVar) << "\n";

    ASTNEXTC(E_new, n)
      PRINT_SUBTREE(n->arraySize);

    ASTNEXTC(E_fieldAcc, f)
      ind(os, indent) << "field: " << refersTo(f->field) << "\n";

    ASTDEFAULTC
      /* do nothing */

    ASTENDCASEC
  }
}


// remove layers of parens: keep going down until the expression is
// not an E_grouping and return that
Expression *Expression::skipGroups()
{
  Expression *ret = this;
  while (ret->isE_grouping()) {
    ret = ret->asE_grouping()->expr;
  }
  return ret;
}


// FullExpression

// ------------------- ArgExpression -------------------------
void ArgExpression::setNext(ArgExpression *newNext)
{
  xassert(next == NULL);
  next = newNext;
}
        

void ArgExpression::addAmbiguity(ArgExpression *alt)
{
  // find the end of alt's ambiguity list
  ArgExpression *altLast = alt;
  while (altLast->ambiguity) {
    altLast = altLast->ambiguity;
  }

  // finally, prepend 'alt's ambiguity list to 'this's ambiguity list
  altLast->ambiguity = this->ambiguity;
  this->ambiguity = alt;
}


void ArgExpression::printAmbiguities(ostream &os, int indent) const
{
  genericPrintAmbiguities(this, "ArgExpression", os, indent);
}


// ExpressionListOpt
// Initializer
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
}


void TemplateArgument::addAmbiguity(TemplateArgument *alt)
{
  xassert(alt->ambiguity == NULL);
  alt->ambiguity = this->ambiguity;
  this->ambiguity = alt;
}


string TA_type::argString() const
{
  return type->getType()->toString();
}

string TA_nontype::argString() const
{
  if (sarg.kind != STemplateArgument::STA_NONE) {
    return sarg.toString();       // hope to get concrete value like "3"
  }
  return expr->exprToString();
}   


void TemplateArgument::printExtras(ostream &os, int indent) const
{
  if (sarg.hasValue()) {
    ind(os, indent) << "sarg: " << sarg.toString() << "\n";
  }
}
