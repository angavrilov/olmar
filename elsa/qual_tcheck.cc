// qual_tcheck.cc
// walk over the AST, computing Type_Qs for everything; this
// runs after cc_tcheck has computed Types, and relies on the
// results computed previously

#include "cc.ast.gen.h"      // C++ AST, with cc_tcheck.ast extension module
#include "qual_var.h"        // Variable_Q
#include "qual_type.h"       // Type_Q


// visitor for the qualTcheck
class QualTcheck : public ASTVisitor {
public:
  // since type computation is inherently bottom-up, all the
  // visitor methods are invoked in post-order
  void postvisitTypeSpecifier(TypeSpecifier *ts);
  void postvisitDeclaration(Declaration *decl);
  void postvisitExpression(Expression *expr);
};


void TranslationUnit::qualTcheck()
{
  // everything is done from within a visitor
  QualTcheck qt;
  traverse(qt);
}


void QualTcheck::postvisitTypeSpecifier(TypeSpecifier *ts)
{
  ASTSWITCH(TypeSpecifier, ts) {
    ASTCASE(TS_name, n) {
      ts->qtype = n->var->q->qtype;
    }
    ASTNEXT(TS_simple, s) {
      ts->qtype = getSimpleType_Q(s->id);
    }
    ASTNEXT(TS_elaborated, e) {
      ts->qtype = makeType_Q(e->atype);
    }
    ASTNEXT(TS_classSpec, cs) {
      ts->qtype = makeType_Q(cs->ctype);
    }
    ASTNEXT(TS_enumSpec, es) {
      ts->qtype = makeType_Q(es->etype);
    }
    ASTDEFAULT {
      xfailure("bad type-spec code");
    }
    ASTENDCASE
  }
}


Type_Q *ASTTypeId::getType_Q() 
{ 
  return decl->qtype; 
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

void Declarator::qualTcheck(Type_Q *typeSoFar)
{
  decl->qualTcheck(this, typeSoFar);
}

void D_name::qualTcheck(Declarator *dtor, Type_Q *typeSoFar)
{
  // this is the binding introduction of a variable, and at this
  // time we know its Type_Q, so this is the place we make Variable_Qs
  dtor->var->q = new Variable_Q(dtor->var, typeSoFar);
  dtor->qtype = typeSoFar;
}

void D_pointer::qualTcheck(Declarator *dtor, Type_Q *typeSoFar)
{
  typeSoFar = new PointerType_Q(&(type->asPointerTypeC()), typeSoFar);
  typeSoFar->q = deepClone(q);
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

  base->qualTcheck(dtor, ft);
}

void D_array::qualTcheck(Declarator *dtor, Type_Q *typeSoFar)
{
  base->qualTcheck(dtor, new ArrayType_Q(&(type->asArrayTypeC()), typeSoFar));
}

void D_bitfield::qualTcheck(Declarator *dtor, Type_Q *typeSoFar)
{
  // same as D_name
  dtor->var->q = new Variable_Q(dtor->var, typeSoFar);
  dtor->qtype = typeSoFar;
}

void D_grouping::qualTcheck(Declarator *dtor, Type_Q *typeSoFar)
{
  base->qualTcheck(dtor, typeSoFar);
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
      // also not correct
      expr->qtype = bi->e1->qtype;
    }
    ASTNEXT(E_addrOf, ad) {
      expr->qtype = new PointerType_Q(&(ad->type->asPointerTypeC()), ad->qtype);
    }
    ASTNEXT(E_deref, de) {
      expr->qtype = de->qtype->asPointerType_Q()->atType;
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
