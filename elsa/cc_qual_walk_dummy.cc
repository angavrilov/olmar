// cc_qual.cc            see license.txt for copyright and terms of use
// code for cc_qual.h

// This is a dummy tree walk that is a front end for a C++ version of
// cqual.  It provides dummy definitions for everything delcared in
// cc_qual_dummy.h and cc.ast.  Adapted from cc_tcheck.cc by Daniel
// Wilkerson dsw@cs.berkeley.edu

#include "cc.ast.gen.h"         // C++ AST; this module

void init_cc_qual(char *config_file) {
  printf("DUMMY INIT config_file: %s\n", config_file);
}

void finish_quals_CQUAL() {
  printf("DUMMY FINISH\n");
}

// ------------------- TranslationUnit --------------------
void TranslationUnit::qual(QualEnv &env)
{
  cout << "CC_QUAL_DUMMY" << endl;
}

// --------------------- TopForm ---------------------
void TF_decl::qual(QualEnv &env) {}

void TF_func::qual(QualEnv &env) {}

void TF_template::qual(QualEnv &env) {}

void TF_linkage::qual(QualEnv &env) {}

// --------------------- Function -----------------
void Function::qual(QualEnv &env) {}


// MemberInit

// -------------------- Declaration -------------------
void Declaration::qual(QualEnv &env) {}

//  -------------------- ASTTypeId -------------------
void ASTTypeId::qual(QualEnv &env) {}

// ---------------------- PQName -------------------
void qualTemplateArgumentFakeList(QualEnv &env, FakeList<TemplateArgument> *args) {}

void PQ_qualifier::qual(QualEnv &env) {}

void PQ_name::qual(QualEnv &env) {}

void PQ_operator::qual(QualEnv &env) {}

void PQ_template::qual(QualEnv &env) {}

// --------------------- TypeSpecifier --------------
void TS_name::qual(QualEnv &env) {}

void TS_simple::qual(QualEnv &env) {}

void TS_elaborated::qual(QualEnv &env) {}

void TS_classSpec::qual(QualEnv &env) {}

void TS_enumSpec::qual(QualEnv &env) {}

// BaseClass
void BaseClassSpec::qual(QualEnv &env) {}

// MemberList

// ---------------------- Member ----------------------
void MR_decl::qual(QualEnv &env) {}

void MR_func::qual(QualEnv &env) {}

void MR_access::qual(QualEnv &env) {}

void MR_publish::qual(QualEnv &env) {}

// -------------------- Enumerator --------------------
void Enumerator::qual(QualEnv &env) {}

// -------------------- Declarator --------------------
void Declarator::qual(QualEnv &env) {}

// ------------------- ExceptionSpec --------------------
void ExceptionSpec::qual(QualEnv &env) {}

// ---------------------- Statement ---------------------
void Statement::qual(QualEnv &env) {}

// no-op
void S_skip::iqual(QualEnv &env) {}

void S_label::iqual(QualEnv &env) {}

void S_case::iqual(QualEnv &env) {}

void S_default::iqual(QualEnv &env) {}

void S_expr::iqual(QualEnv &env) {}

void S_compound::iqual(QualEnv &env) {}

void S_if::iqual(QualEnv &env) {}

void S_switch::iqual(QualEnv &env) {}

void S_while::iqual(QualEnv &env) {}

void S_doWhile::iqual(QualEnv &env) {}

void S_for::iqual(QualEnv &env) {}

void S_break::iqual(QualEnv &env) {}

void S_continue::iqual(QualEnv &env) {}

void S_return::iqual(QualEnv &env) {}

void S_goto::iqual(QualEnv &env) {}

void S_decl::iqual(QualEnv &env) {}

void S_try::iqual(QualEnv &env) {}

// ------------------- Condition --------------------
// CN = ConditioN

// this situation: if (gronk()) {...
void CN_expr::qual(QualEnv &env) {}

// this situation: if (bool b=gronk()) {...
void CN_decl::qual(QualEnv &env) {}

// ------------------- Handler ----------------------
// catch clause
void HR_type::qual(QualEnv &env) {}

void HR_default::qual(QualEnv &env) {}

// ------------------- Expression qual -----------------------
void Expression::qual(QualEnv &env) {}

void E_boolLit::iqual(QualEnv &env) {}

void E_intLit::iqual(QualEnv &env) {}

void E_floatLit::iqual(QualEnv &env) {}

void E_stringLit::iqual(QualEnv &env) {}

void E_charLit::iqual(QualEnv &env) {}

void E_variable::iqual(QualEnv &env) {}

void qualFakeExprList(FakeList<Expression> *list, QualEnv &env) {}

void E_funCall::iqual(QualEnv &env) {}

void E_constructor::iqual(QualEnv &env) {}

void E_fieldAcc::iqual(QualEnv &env) {}

void E_sizeof::iqual(QualEnv &env) {}

// dsw: unary expression?
void E_unary::iqual(QualEnv &env) {}

void E_effect::iqual(QualEnv &env) {}

// dsw: binary operator.
void E_binary::iqual(QualEnv &env) {}

void E_addrOf::iqual(QualEnv &env) {}

void E_deref::iqual(QualEnv &env) {}

// C-style cast
void E_cast::iqual(QualEnv &env) {}

// ? : syntax
void E_cond::iqual(QualEnv &env) {}

void E_comma::iqual(QualEnv &env) {}

void E_sizeofType::iqual(QualEnv &env) {}

void E_assign::iqual(QualEnv &env) {}

void E_new::iqual(QualEnv &env) {}

void E_delete::iqual(QualEnv &env) {}

void E_throw::iqual(QualEnv &env) {}

// C++-style cast
void E_keywordCast::iqual(QualEnv &env) {}

// RTTI: typeid(expression)
void E_typeidExpr::iqual(QualEnv &env) {}

// RTTI: typeid(type)
void E_typeidType::iqual(QualEnv &env) {}

// ----------------------- Initializer --------------------

// this is under a declaration
// int x = 3;
//         ^ only
void IN_expr::qual(QualEnv &env) {}

// int x[] = {1, 2, 3};
//           ^^^^^^^^^ only
void IN_compound::qual(QualEnv &env) {}

void IN_ctor::qual(QualEnv &env) {}


// InitLabel

// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::qual(QualEnv &env) {}

void TD_func::iqual(QualEnv &env) {}

void TD_class::iqual(QualEnv &env) {}

// ------------------- TemplateParameter ------------------
// sm: this isn't used..
void TP_type::qual(QualEnv &env) {}


// -------------------- TemplateArgument ------------------
void TA_type::qual(QualEnv &env) {} 
