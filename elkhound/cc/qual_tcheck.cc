// qual_tcheck.cc
// walk over the AST, computing Type_Qs for everything; this
// runs after cc_tcheck has computed Types, and relies on the
// results computed previously

#include "cc.ast.gen.h"      // C++ AST, with cc_tcheck.ast extension module
#include "qual_var.h"        // Variable_Q
#include "qual_type.h"       // Type_Q

#include <iostream.h>        // cout


// ------------------ utilities ----------------------
// given a Variable for which the 'qtype' might be its binding
// definition, make a Variable_Q if in fact it is the binding
// definition (we'll know it is if the 'q' field of 'var' is still
// NULL); this is used for making the Variable_Q corresponding
// to implicit typedefs
void makeQVarIfNeeded(Variable *var, Type_Q *qtype)
{
  if (!var->q) {
    var->q = new Variable_Q(var, qtype);
  }
}


// given a Type known to be an Atomic type without interesting
// qualifiers (see below), make a CVAtomicType_Q
CVAtomicType_Q *makeAtomic(Type const *t)
{
  return new CVAtomicType_Q(&( t->asCVAtomicTypeC() ));
}

// given a Type that is known to be a pointer (or reference) to an
// atomic type, construct a corresponding PointerType_Q by
// deconstructing the Type and rebuilding a Type_Q; this assumes
// neither the atomic nor the pointer have any interesting qualifiers,
// because if either does they'll be lost by this process
PointerType_Q *makePointerToAtomic(Type const *t)
{
  PointerType const *pt = &( t->asPointerTypeC() );
  PointerType_Q *ptq = new PointerType_Q(pt, makeAtomic(pt->atType));
  return ptq;
}


// AST contains something we can't supported yet in C++Qual world
void notSupported(char const *what)
{
  XNotSupported x(what);
  THROW(x);
}


// ---------------------- visitor ---------------------------
// visitor for the qualTcheck
class QualTcheck : public ASTVisitor {
public:
  // when 0, we're not in a class; when 1, we're in a toplevel
  // class; etc.
  int classNestDepth;

  // select which of the class body passes we're doing,
  // either 1 or 2
  int classPassNumber;

public:
  QualTcheck() : classNestDepth(0), classPassNumber(2) {}
  virtual ~QualTcheck();

  // since type computation is inherently bottom-up, all the
  // visitor methods are invoked in post-order
  void postvisitDeclaration(Declaration *decl);
  void postvisitASTTypeId(ASTTypeId *tid);
  void postvisitTypeSpecifier(TypeSpecifier *ts);
  void postvisitEnumerator(Enumerator *e);
  void postvisitExpression(Expression *expr);
  void postvisitTemplateParameter(TemplateParameter *param);

  // actually, a few special cases require intervening at pre-order time
  bool visitFunction(Function *f);
  bool visitTypeSpecifier(TypeSpecifier *ts);
  bool visitMember(Member *m);
  bool visitTemplateDeclaration(TemplateDeclaration *td);
  bool visitOperatorName(OperatorName *on);
};

QualTcheck::~QualTcheck()
{
  if (!( classNestDepth==0 && classPassNumber==2 )) {
    // don't want to throw an exception in a dtor
    cout << "warning: context state failed to reset (" << __FILE__ << ")\n";
  }
}


void TranslationUnit::qualTcheck(SObjList<Variable> &madeUpVariables)
{
  // for each of the variables that cc_type made up, we assume it's
  // safe to regard them as qualifier-less so we'll build parallel
  // Variable_Q objects with no qualifiers
  SFOREACH_OBJLIST_NC(Variable, madeUpVariables, iter) {
    Variable *v = iter.data();
    if (!v->q) {
      v->q = new Variable_Q(v, buildQualifierFreeType_Q(v->type));
    }
  }

  // everything else is done from within a visitor
  QualTcheck qt;
  traverse(qt);
}


bool QualTcheck::visitTypeSpecifier(TypeSpecifier *ts)
{
  if (ts->isTS_classSpec()) {
    // due to scoping rules for classes, we need to do a pre-pass
    // over all the declarations where we ignore function bodies
    TS_classSpec *cs = ts->asTS_classSpec();

    classNestDepth++;

    // I have to hook up the variable before looking at the class
    // body in case the body refers to its own name (as is routine
    // in C++ classes)
    ts->qtype = makeType_Q(cs->ctype);
    makeQVarIfNeeded(cs->ctype->typedefVar, ts->qtype);

    if (classNestDepth > 1) {
      // we're already inside a class, so let the outer class
      // control the passes--just do one pass here
      return true;
    }

    // the first pass gets all the member decls, ignoring all function
    // bodies, then the next pass gets all bodies
    classPassNumber = 1;

    FOREACH_ASTLIST_NC(Member, cs->members->list, iter) {
      iter.data()->traverse(*this);
    }

    // second pass, and this is the nominal value of 'classPassNumber'
    classPassNumber = 2;
  }

  // now do 2nd pass
  return true;
}


bool QualTcheck::visitFunction(Function *f)
{
  // functions have a TypeSpecifier/Declarator pair which
  // needs to be analyzed first to make a Variable_Q, and
  // the body is then analyzed afterwards

  if (classNestDepth==0 || classPassNumber==1) {
    // pass 1: analyze the declarations only

    // traverse non-body elements
    f->retspec->traverse(*this);
    f->nameAndParams->traverse(*this);

    // this is like a Declaration, so we push the type specifier
    // into the declarator (see below)
    f->nameAndParams->qualTcheck(f->retspec->qtype);
    
    if (f->thisVar) {
      // The C++ type of 'this' should always be 'C * const' or 'C
      // const * const' for a method of class 'C'.  Leverage this
      // expectation to make the C++Qual type.
      PointerType const *pt = &( f->thisVar->type->asPointerTypeC() );
      CVAtomicType const *at = &( pt->atType->asCVAtomicTypeC() );
      
      // the thing to which the 'this' pointer points inherits
      // the qualifiers attached to the function type
      CVAtomicType_Q *atq = new CVAtomicType_Q(at);  
      atq->q = deepClone(f->nameAndParams->var->q->qtype->asFunctionType_Q()->q);

      // the 'this' pointer itself never has user qualifiers (there's
      // syntactically no place to put them)
      PointerType_Q *ptq = new PointerType_Q(pt, atq);
      
      // finally, make a Variable_Q to record this type
      xassert(!f->thisVar->q);
      f->thisVar->q = new Variable_Q(f->thisVar, ptq);
    }
  }

  if (classNestDepth==0 || classPassNumber==2) {
    // pass 2: look at the function body
    FAKELIST_FOREACH_NC(MemberInit, f->inits, iter1) {
      iter1->traverse(*this);
    }
    f->body->traverse(*this);
    FAKELIST_FOREACH_NC(Handler, f->handlers, iter2) {
      iter2->traverse(*this);
    }
  }

  // no automatic traversal
  return false;
}


bool QualTcheck::visitMember(Member *m)
{
  if (m->isMR_func()) {
    // 'visitFunction' knows how to organize itself
    // with respect to the classPasses
    return true;
  }

  else {
    if (classPassNumber==1) {
      // pass 1: do the normal thing
      return true;
    }
    else {
      // pass 2: skip, we're already done
      xassert(classPassNumber==2);
      return false;
    }
  }
}


void QualTcheck::postvisitEnumerator(Enumerator *e)
{
  xassert(!e->var->q);
  e->var->q = new Variable_Q(e->var, makeAtomic(e->var->type));
}


void QualTcheck::postvisitTypeSpecifier(TypeSpecifier *ts)
{
  ASTSWITCH(TypeSpecifier, ts) {
    ASTCASE(TS_name, n) {
      ts->qtype = n->var->q->qtype;
    }
    ASTNEXT(TS_simple, s) {
      ts->qtype = getSimpleType_Q(s->id, s->cv);
      return;    // don't do the applyCV at the end
    }
    ASTNEXT(TS_elaborated, e) {
      ts->qtype = makeType_Q(e->atype);
      makeQVarIfNeeded(e->atype->typedefVar, ts->qtype);
    }
    ASTNEXT(TS_classSpec, cs) {
      PRETEND_USED(cs);
      // this has been moved into the pre-order 'visit' method
      //ts->qtype = makeType_Q(cs->ctype);
      //makeQVarIfNeeded(cs->ctype->typedefVar, ts->qtype);

      // leaving this class
      classNestDepth--;
    }
    ASTNEXT(TS_enumSpec, es) {
      ts->qtype = makeType_Q(es->etype);
      if (es->etype->typedefVar) {
        makeQVarIfNeeded(es->etype->typedefVar, ts->qtype);
      }
      else {
        // happens for anonymous enums, where cc_tcheck doesn't
        // make a Variable object at all
      }
    }
    ASTDEFAULT {
      xfailure("bad type-spec code");
    }
    ASTENDCASE
  }

  ts->qtype = ts->qtype->applyCV(ts->cv);
}


void QualTcheck::postvisitDeclaration(Declaration *decl)
{
  FAKELIST_FOREACH_NC(Declarator, decl->decllist, iter) {
    Declarator *dtor = iter;

    // sm: the code I'm working from re-tchecked the
    // specifier each time to come up with distinct
    // Type objects; here cloning should be equivalent
    dtor->qualTcheck(decl->spec->qtype->deepClone());
  }
}


Type_Q *ASTTypeId::getType_Q() 
{ 
  return decl->qtype; 
}

void QualTcheck::postvisitASTTypeId(ASTTypeId *tid)
{
  // this needs to push the type of 'spec' down into 'decl' the same
  // way Declaration does for all of its declarators; but I don't
  // think cloning the type should be necessary since there is only
  // one declarator to use it (and in fact the old code in cc_tcheck
  // didn't clone it)
  tid->decl->qualTcheck(tid->spec->qtype);
}


void Declarator::qualTcheck(Type_Q *typeSoFar)
{
  decl->qualTcheck(this, typeSoFar);
}

void D_name::qualTcheck(Declarator *dtor, Type_Q *typeSoFar)
{
  if (!dtor->var->q) {
    // this is the binding introduction of a variable, and at this time
    // we know its Type_Q, so this is the place we make Variable_Qs for
    // ordinary variables and for explicit typedefs
    dtor->var->q = new Variable_Q(dtor->var, typeSoFar);
  }
  else {
    // this happens for duplicate declarations
  }

  // either way, record the type constructed from *this* syntax
  // (as opposed to any duplicates)
  xassert(!dtor->qtype);       // shouldn't have already set it
  dtor->qtype = typeSoFar;
  xassert(dtor->qtype->type()->equals(type));
}

void D_pointer::qualTcheck(Declarator *dtor, Type_Q *typeSoFar)
{
  typeSoFar = new PointerType_Q(&(type->asPointerTypeC()), typeSoFar);
  typeSoFar->q = deepClone(q);
  xassert(typeSoFar->type()->equals(type));
  base->qualTcheck(dtor, typeSoFar);
}

void D_func::qualTcheck(Declarator *dtor, Type_Q *typeSoFar)
{
  // wrap up 'typeSoFar' as the return type of a function type
  FunctionType_Q *ft = new FunctionType_Q(&(type->asFunctionTypeC()), typeSoFar);
  ft->q = deepClone(q);

  // iterate over parameters, adding them to 'ft' (in reverse)
  FAKELIST_FOREACH_NC(ASTTypeId, params, iter) {
    ft->params.prepend(iter->decl->var->q);
  }
  ft->params.reverse();

  xassert(ft->type()->equals(type));
  base->qualTcheck(dtor, ft);
}

void D_array::qualTcheck(Declarator *dtor, Type_Q *typeSoFar)
{
  if (!isNewSize) {
    typeSoFar = new ArrayType_Q(&(type->asArrayTypeC()), typeSoFar);
  }
  else {
    // this happens when the D_array is in a 'new' expression and is
    // specifying the # of elements to allocate, *not* acting as a type
    // constructor; we just pass on our type unchanged
  }

  xassert(typeSoFar->type()->equals(type));
  base->qualTcheck(dtor, typeSoFar);
}

void D_bitfield::qualTcheck(Declarator *dtor, Type_Q *typeSoFar)
{
  // same as D_name
  dtor->var->q = new Variable_Q(dtor->var, typeSoFar);
  dtor->qtype = typeSoFar;
  xassert(typeSoFar->type()->equals(type));
}

void D_grouping::qualTcheck(Declarator *dtor, Type_Q *typeSoFar)
{
  base->qualTcheck(dtor, typeSoFar);
  xassert(typeSoFar->type()->equals(type));
}


void QualTcheck::postvisitExpression(Expression *expr)
{
  ASTSWITCH(Expression, expr) {
    ASTCASE(E_boolLit, bo) {
      expr->qtype = makeAtomic(bo->type);
    }
    ASTNEXT(E_intLit, in) {
      expr->qtype = makeAtomic(in->type);
    }
    ASTNEXT(E_floatLit, in) {
      expr->qtype = makeAtomic(in->type);
    }
    ASTNEXT(E_stringLit, st) {
      // cc_tcheck should have computed char* or similar
      expr->qtype = makePointerToAtomic(st->type);
    }
    ASTNEXT(E_charLit, ch) {
      expr->qtype = makeAtomic(ch->type);
    }
    ASTNEXT(E_variable, va) {
      expr->qtype = va->var->q->qtype;
    }
    ASTNEXT(E_funCall, fc) {
      if (fc->func->type->asRval()->ifCompoundType()) {
        notSupported("operator overloading");
      }
      FunctionType_Q *ft = fc->func->qtype->asFunctionType_Q();
      expr->qtype = ft->retType;
    }
    ASTNEXT(E_constructor, co) {
      expr->qtype = co->spec->qtype;
    }
    ASTNEXT(E_fieldAcc, fa) {
      expr->qtype = fa->field->q->qtype;
    }
    ASTNEXT(E_sizeof, si) {
      expr->qtype = makeAtomic(si->type);
    }
    ASTNEXT(E_unary, un) {
      // for now; cc_tcheck is incomplete
      expr->qtype = makeAtomic(un->type);
    }
    ASTNEXT(E_effect, ef) {        
      // not always correct, but agrees with cc_tcheck
      expr->qtype = ef->expr->qtype;
    }
    ASTNEXT(E_binary, bi) {       
      if (bi->e1->qtype->isArrayType_Q()) {
        // coerce the lhs array to a pointer
        expr->qtype =
          new PointerType_Q(&(bi->type->asPointerTypeC()), 
                            bi->e1->qtype->asArrayType_Q()->eltType);
      }
      else {
        // not correct but close enough for now: use LHS type
        expr->qtype = bi->e1->qtype;
      }
    }
    ASTNEXT(E_addrOf, ad) {
      expr->qtype = new PointerType_Q(&(ad->type->asPointerTypeC()), ad->expr->qtype);
    }
    ASTNEXT(E_deref, de) {
      if (!de->ptr->type->asRval()->isPointerType()) {
        // could be operator overloading:  Foo f; f[4];
        // or ordinary overloading:
        //   void f();
        //   int *f(int);
        //   ... *(f(4)) ..     // picks wrong one
        notSupported("overloading");
      }
      expr->qtype = de->ptr->qtype->asPointerType_Q()->atType;
    }
    ASTNEXT(E_cast, ca) {
      expr->qtype = ca->ctype->decl->qtype;
    }
    ASTNEXT(E_cond, co) {
      expr->qtype = co->th->qtype;
    }
    ASTNEXT(E_comma, co) {
      expr->qtype = co->e2->qtype;
    }
    ASTNEXT(E_sizeofType, si) {
      expr->qtype = makeAtomic(si->type);
    }
    ASTNEXT(E_assign, as) {
      expr->qtype = as->target->qtype;
    }
    ASTNEXT(E_new, ne) {
      expr->qtype = new PointerType_Q(&(ne->type->asPointerTypeC()),
                                      ne->atype->decl->qtype);
    }
    ASTNEXT(E_delete, de) {
      expr->qtype = makeAtomic(de->type);
    }
    ASTNEXT(E_throw, th) {
      expr->qtype = makeAtomic(th->type);
    }
    ASTNEXT(E_keywordCast, ke) {
      expr->qtype = ke->ctype->getType_Q();
    }
    ASTNEXT(E_typeidExpr, ty) {
      // should be 'type_info const &'
      expr->qtype = makePointerToAtomic(ty->type);
    }
    ASTNEXT(E_typeidType, ty) {
      // should be 'type_info const &'
      expr->qtype = makePointerToAtomic(ty->type);
    }
    ASTDEFAULT {               
      // if this happens, either a new E_* node has been added
      // since I wrote this, or something that was expected to
      // be an atomic type wasn't
      xfailure("fell through");
    }
    ASTENDCASE
  }
}


void QualTcheck::postvisitTemplateParameter(TemplateParameter *param)
{
  Variable *v = param->type->atomic->asTypeVariableC().typedefVar;
  xassert(!v->q);
  v->q = new Variable_Q(v, new CVAtomicType_Q(param->type));
}


bool QualTcheck::visitTemplateDeclaration(TemplateDeclaration *)
{
  // sm: I've decided that continuing to massage the checker to
  // "work" in the face of templates is pointless.  The C++Qual
  // analysis hasn't been extended to deal with template
  // concepts directly, and we don't have a template expander
  // either.  Therefore, if I see any templates at all I'm
  // going to immediately bail.
  notSupported("templates");
  return false;   // silence warning
}

bool QualTcheck::visitOperatorName(OperatorName *on)
{
  if (on->isON_conversion()) {
    // In cc.in/cc.in20, I'm getting something weird where an
    // assertion fails with a type mismatch, one of which is
    // "/*cdtor*/ ()()" and the other is "char ()()" and I don't
    // know why.  For now I'll call it unsupported
    notSupported("conversion operators");
  }
  return true;
}

