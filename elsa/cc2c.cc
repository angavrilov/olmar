// cc2c.cc
// code for cc2c.h

#include "cc2c.h"            // this module

// Elsa
#include "ia64mangle.h"      // ia64MangleType

// smbase
#include "exc.h"             // xunimp
#include "macros.h"          // Restorer


// for now
SourceLoc const SL_GENERATED = SL_UNKNOWN;


// set/restore the current compound statement
#define SET_CUR_COMPOUND_STMT(newValue) \
  Restorer<S_compound*> restore(env.curCompoundStmt, (newValue)) /* user ; */


// ------------- CC2CEnv::TypedefEntry --------------
STATICDEF CType const *CC2CEnv::TypedefEntry::getType(TypedefEntry *entry)
{
  return entry->type;
}


// -------------------- CC2CEnv ---------------------
CC2CEnv::CC2CEnv(StringTable &s)
  : str(s),
    typedefs(&TypedefEntry::getType,
             &CType::hashType,
             &CType::equalTypes),
    compoundTypes(),
    curCompoundStmt(NULL),
    dest(new TranslationUnit(NULL /*topForms*/))
{}


CC2CEnv::~CC2CEnv()
{
  if (dest) {
    delete dest;
  }
}


TranslationUnit *CC2CEnv::takeDest()
{
  TranslationUnit *ret = dest;
  dest = NULL;
  return ret;
}


void CC2CEnv::addTopForm(TopForm *tf)
{
  dest->topForms.append(tf);
}


void CC2CEnv::addStatement(Statement *s)
{
  xassert(curCompoundStmt);
  curCompoundStmt->stmts.append(s);
}


StringRef CC2CEnv::getTypeName(CType *t)
{
  TypedefEntry *entry = typedefs.get(t);
  if (entry) {
    return entry->name;
  }

  // make a new entry; the name is chosen to correspond with the
  // mangled name of the type for readability, but any unique name
  // would suffice since types are not linker-visible
  entry = new TypedefEntry;
  entry->type = t;
  entry->name = str(stringb("Type__" << ia64MangleType(t)));
  typedefs.add(t, entry);

  // Also add a declaration to the output.  This requires actually
  // describing the type using TypeSpecifiers and Declarators.  Use
  // just one nontrivial declarator at a time, recursively creating
  // typedefs to describe the inner structure.
  //
  // Each of the cases below sets 'typeSpec' and 'decl'.
  TypeSpecifier *typeSpec = NULL;
  IDeclarator *decl = NULL;

  // 'decl' will always be built on top of this.
  D_name *baseName = new D_name(SL_GENERATED, makePQ_name(entry->name));

  switch (t->getTag()) {
    case CType::T_ATOMIC: {
      typeSpec = makeTypeSpecifier(t);
      decl = baseName;
      break;
    }

    case CType::T_POINTER:
    case CType::T_REFERENCE:
      // reference and pointer both become pointer
      typeSpec = makeTypeSpecifier(t->getAtType());
      decl = new D_pointer(SL_GENERATED, t->getCVFlags(), baseName);
      break;

    case CType::T_FUNCTION: {
      FunctionType *ft = t->asFunctionType();
      typeSpec = makeTypeSpecifier(ft->retType);
      xunimp("rest of FunctionType");
      break;
    }

    case CType::T_ARRAY:
      xunimp("array");
      break;

    case CType::T_POINTERTOMEMBER:
      xunimp("ptm");
      break;

    case CType::T_DEPENDENTSIZEDARRAY:
    case CType::T_LAST_TYPE_TAG:
      // don't set 'typeSpec'; error detected below
      break;

    // there is no 'default' so that I will get a compiler warning if
    // a new tag is added
  }

  if (!typeSpec) {
    xfailure("getTypeName: bad/unhandled type tag");
  }
  xassert(decl);

  addTopForm(new TF_decl(
    SL_GENERATED,
    new Declaration(
      DF_TYPEDEF,
      typeSpec,
      FakeList<Declarator>::makeList(
        new Declarator(decl, NULL /*init*/)
      )
    )
  ));

  return entry->name;
}


StringRef CC2CEnv::getCompoundTypeName(CompoundType *ct)
{
  StringRef ret = compoundTypes.get(ct);
  if (ret) {
    return ret;
  }

  if (!ct->parentScope->isGlobalScope()) {
    xunimp("CC2CEnv::getCompoundTypeName: non-global scope");
  }

  if (ct->templateInfo()) {
    xunimp("CC2CEnv::getCompoundTypeName: non-NULL templateInfo");
  }

  if (!ct->name) {
    xunimp("CC2CEnv::getCompoundTypeName: anonymous");
  }

  xunimp("CC2CEnv::getCompoundTypeName: add a declaration");
  // also need to add to 'compoundTypes'

  return str(ct->name);
}


TypeSpecifier *CC2CEnv::makeTypeSpecifier(CType *t)
{
  if (t->isCVAtomicType()) {
    // no need for a declarator
    CVAtomicType *at = t->asCVAtomicType();
    TypeSpecifier *ret = makeAtomicTypeSpecifier(at->atomic);
    ret->cv = at->cv;
    return ret;
  }

  else {
    // need a declarator; circumvent by using a typedef
    return new TS_name(
      SL_GENERATED, 
      makePQ_name(getTypeName(t)),
      false /*typenameUsed*/
    );
  }
}


TypeSpecifier *CC2CEnv::makeAtomicTypeSpecifier(AtomicType *at)
{
  switch (at->getTag()) {
    case AtomicType::T_SIMPLE: {
      SimpleType *st = at->asSimpleType();
      
      // I assume that the target language supports all of the
      // built-in types that the source language does except for bool,
      // which is translated into 'char'.
      SimpleTypeId id = st->type;
      if (id == ST_BOOL) {
        id = ST_CHAR;
      }

      return new TS_simple(SL_GENERATED, id);
    }

    case AtomicType::T_COMPOUND: {
      CompoundType *ct = at->asCompoundType();
      return new TS_elaborated(
        SL_GENERATED,
        ct->keyword == CompoundType::K_UNION? TI_UNION : TI_STRUCT,
        makePQ_name(getCompoundTypeName(ct))
      );
    }

    case AtomicType::T_ENUM:
      xunimp("makeAtomicTypeSpecifier for enum");
      return NULL;   // placeholder
      
    case AtomicType::T_TYPEVAR:
    case AtomicType::T_PSEUDOINSTANTIATION:
    case AtomicType::T_DEPENDENTQTYPE:
    case AtomicType::T_TEMPLATETYPEVAR:
      xfailure("makeAtomicTypeSpecifier: template-related type");
      break;

    case AtomicType::NUM_TAGS:
      break;
  }

  xfailure("makeAtomicTypeSpecifier: bad/unhandled type tag");
  return NULL;   // silence warning
}


StringRef CC2CEnv::getVariableName(Variable const *v)
{
  // for now
  return str(v->name);
}


PQ_name *CC2CEnv::makeName(Variable const *v)
{
  return makePQ_name(getVariableName(v));
}


PQ_name *CC2CEnv::makePQ_name(StringRef name)
{
  return new PQ_name(SL_GENERATED, name);
}


FakeList<ASTTypeId> *CC2CEnv::makeParameterTypes(FunctionType *ft)
{
  // will build in reverse order, then fix at the end
  FakeList<ASTTypeId> *dest = FakeList<ASTTypeId>::emptyList();

  if (ft->acceptsVarargs()) {
    dest = dest->prepend(
      new ASTTypeId(
        new TS_simple(SL_GENERATED, ST_ELLIPSIS),
        new Declarator(
          new D_name(SL_GENERATED, NULL /*name*/),
          NULL /*init*/
        )
      )
    );
  }

  SFOREACH_OBJLIST(Variable, ft->params, iter) {
    Variable const *param = iter.data();
    
    dest = dest->prepend(
      new ASTTypeId(
        makeTypeSpecifier(param->type),
        new Declarator(
          new D_name(SL_GENERATED, makeName(param)),
          NULL /*init*/
        )
      )
    );
  }

  return dest->reverse();
}


// -------------------- TopForm --------------------
void TopForm::cc2c(CC2CEnv &env) const
{
  ASTSWITCHC(TopForm, this) {
    ASTCASEC1(TF_decl) {
      xunimp("cc2cTopForm: TF_decl");
    }

    ASTNEXTC(TF_func, f) {
      env.addTopForm(new TF_func(SL_GENERATED, f->f->cc2c(env)));
    }

    ASTDEFAULTC {
      xunimp("cc2cTopForm");
    }
    
    ASTENDCASE
  }
}


// -------------------- Function -------------------
Function *Function::cc2c(CC2CEnv &env) const
{
  if (inits->isNotEmpty()) {
    xunimp("member initializers");
  }

  if (handlers->isNotEmpty()) {
    xunimp("exception handlers");
  }

  Function *ret = new Function(
    dflags & (DF_STATIC | DF_EXTERN | DF_INLINE),
    env.makeTypeSpecifier(funcType->retType),
    new Declarator(
      new D_func(
        SL_GENERATED,
        new D_name(SL_GENERATED, env.makeName(nameAndParams->var)),
        env.makeParameterTypes(funcType),
        CV_NONE,
        NULL /*exnSpec*/
      ),
      NULL /*init*/
    ),
    NULL /*inits*/,
    body->cc2c(env)->asS_compound(),
    NULL /*handlers*/
  );

  return ret;
}


// -------------------- Statement --------------------
Statement *S_skip::cc2c(CC2CEnv &env) const 
{
  return new S_skip(SL_GENERATED);
}


Statement *S_label::cc2c(CC2CEnv &env) const
{
  return new S_label(SL_GENERATED, env.str(name), s->cc2c(env));
}


Statement *S_case::cc2c(CC2CEnv &env) const
{
  return new S_case(SL_GENERATED, expr->cc2cNoSideEffects(env), s->cc2c(env));
}


Statement *S_default::cc2c(CC2CEnv &env) const
{
  return new S_default(SL_GENERATED, s->cc2c(env));
}


Statement *S_expr::cc2c(CC2CEnv &env) const
{
  xunimp("S_expr::cc2c");
  return NULL;
}


Statement *S_compound::cc2c(CC2CEnv &env) const
{
  S_compound *ret = new S_compound(SL_GENERATED, NULL /*stmts*/);

  SET_CUR_COMPOUND_STMT(ret);

  FOREACH_ASTLIST(Statement, stmts, iter) {
    ret->stmts.append(iter.data()->cc2c(env));
  }

  return ret;
}


Statement *S_if::cc2c(CC2CEnv &env) const { xunimp("if"); return NULL; }
Statement *S_switch::cc2c(CC2CEnv &env) const { xunimp("switch"); return NULL; }
Statement *S_while::cc2c(CC2CEnv &env) const { xunimp("while"); return NULL; }
Statement *S_doWhile::cc2c(CC2CEnv &env) const { xunimp("doWhile"); return NULL; }
Statement *S_for::cc2c(CC2CEnv &env) const { xunimp("for"); return NULL; }


Statement *S_break::cc2c(CC2CEnv &env) const
{
  return new S_break(SL_GENERATED);
}


Statement *S_continue::cc2c(CC2CEnv &env) const
{
  return new S_continue(SL_GENERATED);
}


Statement *S_return::cc2c(CC2CEnv &env) const
{
  if (expr) {
    return new S_return(SL_GENERATED, expr->cc2c(env));
  }
  else {
    return new S_return(SL_GENERATED, NULL);
  }
}


Statement *S_goto::cc2c(CC2CEnv &env) const
{
  return new S_goto(SL_GENERATED, env.str(target));
}


Statement *S_decl::cc2c(CC2CEnv &env) const { xunimp("decl"); return NULL; }
Statement *S_try::cc2c(CC2CEnv &env) const { xunimp("try"); return NULL; }
Statement *S_asm::cc2c(CC2CEnv &env) const { xunimp("asm"); return NULL; }


Statement *S_namespaceDecl::cc2c(CC2CEnv &env) const 
{
  // should be able to just drop these
  return new S_skip(SL_GENERATED);
}


Statement *S_computedGoto::cc2c(CC2CEnv &env) const { xunimp("computed goto"); return NULL; }
Statement *S_rangeCase::cc2c(CC2CEnv &env) const { xunimp("range case"); return NULL; }
Statement *S_function::cc2c(CC2CEnv &env) const { xunimp("S_function"); return NULL; }


// ------------------- Expression --------------------
Expression *Expression::cc2cNoSideEffects(CC2CEnv &env) const
{ 
  // provide a place to put generated statements
  S_compound temp(SL_GENERATED, NULL /*stmts*/);
  SET_CUR_COMPOUND_STMT(&temp);
  
  Expression *ret = cc2c(env);
  
  // confirm that nothing got put there
  xassert(temp.stmts.isEmpty());
  
  return ret;
}


Expression *E_boolLit::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }


Expression *E_intLit::cc2c(CC2CEnv &env) const
{
  return new E_intLit(text);
}


Expression *E_floatLit::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_stringLit::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_charLit::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_this::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_variable::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_funCall::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_constructor::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_fieldAcc::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_sizeof::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_unary::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_effect::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_binary::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_addrOf::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_deref::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_cast::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_cond::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_sizeofType::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_assign::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_new::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_delete::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_throw::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_keywordCast::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_typeidExpr::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_typeidType::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_grouping::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_arrow::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }

Expression *E_addrOfLabel::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_gnuCond::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_alignofExpr::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_alignofType::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E___builtin_va_arg::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E___builtin_constant_p::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_compoundLit::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }
Expression *E_statement::cc2c(CC2CEnv &env) const { xunimp(""); return NULL; }


// ------------------ FullExpression -----------------
FullExpression *FullExpression::cc2c(CC2CEnv &env) const
{
  return new FullExpression(expr->cc2c(env));
}


// ------------------- entry point -------------------
TranslationUnit *cc_to_c(StringTable &str, TranslationUnit const &input)
{
  CC2CEnv env(str);

  FOREACH_ASTLIST(TopForm, input.topForms, iter) {
    iter.data()->cc2c(env);
  }

  return env.takeDest();
}


// EOF
