// tcheck.cc
// implementation of typechecker over C ast
                      
#include "c.ast.gen.h"      // C ast

void TranslationUnit::tcheck(Env &env)
{
  FOREACH_ASTLIST_NC(TopForm, topForms, iter) {
    iter.data()->tcheck(env);
  }
}


void TF_decl::tcheck(Env &env)
{
  decl->tcheck(env);
}

void TF_func::tcheck(Env &env)
{
  nameParams->tcheck(env);
  body->tcheck(env);
}


void Declaration::tcheck(Env &env)
{
  FOREACH_ASTLIST_NC(Declarator, decllist, iter) {
    iter.data()->tcheck(env);
  }
}


void D_name::tcheck(Env &env)
{
  cout << "tcheck: found declarator name: " << (name? name : "(null)") << endl;
}

void D_func::tcheck(Env &env)
{
  base->tcheck(env);
  FOREACH_ASTLIST_NC(ASTTypeId, params, iter) {
    iter.data()->decl->tcheck(env);
  }
}

void D_array::tcheck(Env &env)
{
  base->tcheck(env);
}

void D_bitfield::tcheck(Env &env)
{
  cout << "tcheck: found bitfield declarator name: " 
       << (name? name : "(null)") << endl;
}


void S_skip::tcheck(Env &env)
{}

void S_label::tcheck(Env &env)
{}

void S_case::tcheck(Env &env)
{}

void S_caseRange::tcheck(Env &env)
{}

void S_default::tcheck(Env &env)
{}

void S_expr::tcheck(Env &env)
{}

void S_compound::tcheck(Env &env)
{
  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    iter.data()->tcheck(env);
  }
}

void S_if::tcheck(Env &env)
{}

void S_switch::tcheck(Env &env)
{}

void S_while::tcheck(Env &env)
{}

void S_doWhile::tcheck(Env &env)
{}

void S_for::tcheck(Env &env)
{}

void S_break::tcheck(Env &env)
{}

void S_continue::tcheck(Env &env)
{}

void S_return::tcheck(Env &env)
{}

void S_goto::tcheck(Env &env)
{}

void S_decl::tcheck(Env &env)
{
  decl->tcheck(env);
}
