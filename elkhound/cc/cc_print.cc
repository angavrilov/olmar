// cc_print.cc            see license.txt for copyright and terms of use
// code for cc_print.h

// Adapted from cc_tcheck.cc by Daniel Wilkerson dsw@cs.berkeley.edu

// This is a tree walk that prints out a functionally equivalent C++
// program to the original.

#include "cc_print.h"           // this module
#include "trace.h"              // trace
#include "strutil.h"            // string utilities

#include <stdlib.h>             // getenv

// where code output goes
// sm: I've replaced uses of this with 'env' instead
//code_output_stream global_code_out(cout);

// set this environment variable to see the twalk_layer debugging
// output
twalk_output_stream twalk_layer_out(cout, getenv("TWALK_VERBOSE"));
//  twalk_output_stream twalk_layer_out(cout, true);
  
// sm: folded this into the PrintEnv
//SourceLocation current_loc;

string make_indentation(int n) {
  stringBuilder s;
  for (int i=0; i<n; ++i) s << "  ";
  return s;
}

string indent_message(int n, string message) {
  stringBuilder s;
  char *m = message.pchar();
  int len = strlen(m);
  for(int i=0; i<len; ++i) {
    s << m[i];
    if (m[i] == '\n') s << make_indentation(n);
  }
  return s;
}

// function for printing declarations (without the final semicolon);
// handles a variety of declarations such as:
//   int x
//   int x()
//   C()                    // ctor inside class C
//   operator delete[]()
//   char operator()        // conversion operator to 'char'
string declaration_toString(
  // declflags present in source; not same as 'var->flags' because
  // the latter is a mixture of flags present in different
  // declarations
  DeclFlags dflags,

  // type of the variable; not same as 'var->type' because the latter
  // can come from a prototype and hence have different parameter
  // names
  Type const *type,

  // original name in the source; for now this is redundant with
  // 'var->name', but we plan to print the qualifiers by looking
  // at 'pqname'
  PQName const * /*nullable*/ pqname,

  // associated variable; in the final design, this will only be
  // used to look up the variable's scope
  Variable *var)
{
  olayer ol("declaration_toString");
  stringBuilder s;

  // mask off flags used for internal purposes, so all that's
  // left is the flags that were present in the source
  dflags = (DeclFlags)(dflags & DF_SOURCEFLAGS);
  if (dflags) {
    s << toString(dflags) << " ";
  }

  // the string name after all of the qualifiers; if this is
  // a special function, we're getting the encoded version
  StringRef finalName = pqname? pqname->getName() : NULL;

  if (finalName && 0==strcmp(finalName, "conversion-operator")) {
    // special syntax for conversion operators; first the keyword
    s << "operator ";

    // then the return type and the function designator
    s << type->asFunctionTypeC().retType->toString() << " ()";
  }

  else if (finalName && 0==strcmp(finalName, "constructor-special")) {
    // extract the class name, which can be found by looking up
    // the name of the scope which contains the associated variable
    s << type->toCString(var->scope->curCompound->name);
  }

  else {
    //s << var->type->toCString(qualifierName);
    //s << var->toString();
//      s << type->toCString(finalName);
    if (finalName) {
      string scope_thing = string("");
      if (pqname) {
        scope_thing = pqname->qualifierString();
      }
      stringBuilder sb;
      sb << scope_thing;
      sb << finalName;
      s << type->toCString(sb);
    } else {
      s << type->toCString(finalName);
    }
  }

  return s;
}


// more specialized version of the previous function
string var_toString(Variable *var, PQName const * /*nullable*/ pqname)
{
  return declaration_toString(var->flags, var->type, pqname, var);
}


// this is a prototype for a function down near E_funCall::iprint
void printFakeExprList(FakeList<Expression> *list, PrintEnv &env);


// ------------------- TranslationUnit --------------------
void TranslationUnit::print(PrintEnv &env)
{
  olayer ol("TranslationUnit");
  FOREACH_ASTLIST_NC(TopForm, topForms, iter) {
    iter.data()->print(env);
  }
}

// --------------------- TopForm ---------------------
void TF_decl::print(PrintEnv &env)
{
  olayer ol("TF_decl");
  env.current_loc = loc;
  decl->print(env);
}

void TF_func::print(PrintEnv &env)
{
  olayer ol("TF_func");
  env << endl;
  env.current_loc = loc;
  f->print(env);
}

void TF_template::print(PrintEnv &env)
{
  env.current_loc = loc;
  td->print(env);
}

void TF_linkage::print(PrintEnv &env)
{
  env.current_loc = loc;
  forms->print(env);
}

// --------------------- Function -----------------
void Function::print(PrintEnv &env)
{
  olayer ol("Function");
  //env << var_toString(nameAndParams->var, nameAndParams->decl->decl->getDeclaratorId());

  // instead of walking 'nameAndParams', use 'funcType' to
  // get accurate parameter names
  //nameAndParams->print(env);

  env <<
    declaration_toString(dflags, funcType,
                         nameAndParams->getDeclaratorId(),
                         nameAndParams->var);

  if (inits) {
    env << ":";
    bool first_time = true;
    FAKELIST_FOREACH_NC(MemberInit, inits, iter) {
      if (first_time) first_time = false;
      else env << ",";
      // NOTE: eventually will be able to figure out if we are
      // initializing a base class or a member variable.  There will
      // be a field added to class MemberInit that will say.
      codeout co(env, iter->name->toString(), "(", ")");
      printFakeExprList(iter->args, env);
    }
  }

  if (handlers) env << "try";

  body->print(env);

  if (handlers) {
    FAKELIST_FOREACH_NC(Handler, handlers, iter) {
      iter->print(env);
    }
  }
}


// MemberInit

// -------------------- Declaration -------------------
void Declaration::print(PrintEnv &env)
{
  olayer ol("Declaration");
  if(spec->isTS_classSpec()) {
    spec->asTS_classSpec()->print(env);
  }
  else if(spec->isTS_enumSpec()) {
    spec->asTS_enumSpec()->print(env);
  }
  FAKELIST_FOREACH_NC(Declarator, decllist, iter) {
    // if there are decl flags that didn't get put into the
    // Variable (e.g. DF_EXTERN which gets turned off as soon
    // as a definition is seen), print them first
    DeclFlags extras = (DeclFlags)(dflags & ~(iter->var->flags));
    if (extras) {
      env << toString(extras) << " ";
    }

    iter->print(env);
    env << ";" << endl;
  }
}

//  -------------------- ASTTypeId -------------------
void ASTTypeId::print(PrintEnv &env)
{
  olayer ol("ASTTypeId");
  decl->print(env);
}

// ---------------------- PQName -------------------
void PQ_qualifier::print(PrintEnv &env)
{
  FAKELIST_FOREACH_NC(TemplateArgument, targs, iter) {
    iter->print(env);
  }
  rest->print(env);
}

void PQ_name::print(PrintEnv &env)
{}

void PQ_operator::print(PrintEnv &env)
{}

void PQ_template::print(PrintEnv &env)
{
  FAKELIST_FOREACH_NC(TemplateArgument, args, iter) {
    iter->print(env);
  }
}

// --------------------- TypeSpecifier --------------
void TS_name::print(PrintEnv &env)
{
  name->print(env);
}

void TS_simple::print(PrintEnv &env)
{
}

void TS_elaborated::print(PrintEnv &env)
{
  env.current_loc = loc;
  name->print(env);
}

void TS_classSpec::print(PrintEnv &env)
{
  olayer ol("TS_classSpec");
  env << toString(cv);
  env << toString(keyword) << " ";
  if (name) env << toString(name);
  bool first_time = true;
  FAKELIST_FOREACH_NC(BaseClassSpec, bases, iter) {
    if (first_time) {
      env << ":";
      first_time = false;
    }
    else env << ",";
    iter->print(env);
  }
  codeout co(env, "", "{\n", "};\n");
  FOREACH_ASTLIST_NC(Member, members->list, iter2) {
    iter2.data()->print(env);
  }
}

void TS_enumSpec::print(PrintEnv &env)
{
  olayer ol("TS_classSpec");
  env << toString(cv);
  env << "enum ";
  if (name) env << toString(name);
  codeout co(env, "", "{\n", "};\n");
  FAKELIST_FOREACH_NC(Enumerator, elts, iter) {
    iter->print(env);
    env << "\n";
  }
}

// BaseClass
void BaseClassSpec::print(PrintEnv &env) {
  if (isVirtual) env << "virtual ";
  if (access!=AK_UNSPECIFIED) env << toString(access) << " ";
  env << name->toString();
}

// MemberList

// ---------------------- Member ----------------------
void MR_decl::print(PrintEnv &env)
{                   
  olayer ol("MR_decl");
  d->print(env);
}

void MR_func::print(PrintEnv &env)
{
  olayer ol("MR_func");
  f->print(env);
}

void MR_access::print(PrintEnv &env)
{
  olayer ol("MR_access");
  env << toString(k) << ":\n";
}

void MR_publish::print(PrintEnv &env)
{
  env << name->toString() << ";\n";
}

// -------------------- Enumerator --------------------
void Enumerator::print(PrintEnv &env)
{
  env << name;
  if (expr) {
    env << "=";
    expr->print(env);
  }
  env << ", ";
}

// -------------------- Declarator --------------------
void Declarator::print(PrintEnv &env)
{
  olayer ol("Declarator");
  env << var_toString(var, decl->getDeclaratorId());
  D_bitfield *b = dynamic_cast<D_bitfield*>(decl);
  if (b) {
    env << ":";
    b->bits->print(env);
  }
//    var_toString(var, decl->getDeclaratorId()->toString());
  if (init) {
    IN_ctor *ctor = dynamic_cast<IN_ctor*>(init);
    if (ctor) {
      // dsw:Constructor arguments.
      codeout co(env, "", "(", ")");
      ctor->print(env);         // NOTE: You can NOT factor this line out of the if!
    } else {
      env << "=";
      init->print(env);         // Don't pull this out!
    }
  }
}

// ------------------- ExceptionSpec --------------------
void ExceptionSpec::print(PrintEnv &env)
{
  olayer ol("ExceptionSpec");
  env << "throw"; // Scott says this is right.
  FAKELIST_FOREACH_NC(ASTTypeId, types, iter) {
    iter->print(env);
  }
}

// ---------------------- Statement ---------------------
void Statement::print(PrintEnv &env)
{
  olayer ol("Statement");
  env.current_loc = loc;
  iprint(env);
  //    env << ";\n";
}

// no-op
void S_skip::iprint(PrintEnv &env)
{
  olayer ol("S_skip::itprint");
  env << ";\n";
}

void S_label::iprint(PrintEnv &env)
{
  olayer ol("S_label::itprint");
  env << name << ":";
  s->print(env);
}

void S_case::iprint(PrintEnv &env)
{                    
  olayer ol("S_case::itprint");
  env << "case";
  expr->print(env);
  env << ":";
  s->print(env);
}

void S_default::iprint(PrintEnv &env)
{
  olayer ol("S_default::itprint");
  env << "default:";
  s->print(env);
}

void S_expr::iprint(PrintEnv &env)
{
  olayer ol("S_expr::itprint");
  expr->print(env);
  env << ";\n";
}

void S_compound::iprint(PrintEnv &env)
{ 
  olayer ol("S_compound::itprint");
  codeout co(env, "", "{\n", "}\n");
  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    iter.data()->print(env);
  }
}

void S_if::iprint(PrintEnv &env)
{
  olayer ol("S_if::itprint");
  {
    codeout co(env, "if", "(", ")");
    cond->print(env);
  }
  thenBranch->print(env);
  env << "else ";
  elseBranch->print(env);
}

void S_switch::iprint(PrintEnv &env)
{
  olayer ol("S_switch::itprint");
  {
    codeout co(env, "switch", "(", ")");
    cond->print(env);
  }
  branches->print(env);
}

void S_while::iprint(PrintEnv &env)
{
  olayer ol("S_while::itprint");
  {
    codeout co(env, "while", "(", ")");
    cond->print(env);
  }
  body->print(env);
}

void S_doWhile::iprint(PrintEnv &env)
{
  olayer ol("S_doWhile::itprint");
  {
    codeout co(env, "do");
    body->print(env);
  }
  {
    codeout co(env, "while", "(", ")");
    expr->print(env);
  }
  env << ";\n";
}

void S_for::iprint(PrintEnv &env)
{
  olayer ol("S_for::itprint");
  {
    codeout co(env, "for", "(", ")");
    init->print(env);
    // this one not needed as the declaration provides one
    //          env << ";";
    cond->print(env);
    env << ";";
    after->print(env);
  }
  body->print(env);
}

void S_break::iprint(PrintEnv &env)
{
  olayer ol("S_break::itprint");
  env << "break";
  env << ";\n";
}

void S_continue::iprint(PrintEnv &env)
{
  olayer ol("S_continue::itprint");
  env << "continue";
  env << ";\n";
}

void S_return::iprint(PrintEnv &env)
{
  olayer ol("S_return::itprint");
  env << "return";
  if (expr) expr->print(env);
  env << ";\n";
}

void S_goto::iprint(PrintEnv &env)
{
  // dsw: When doing a control-flow pass, keep a current function so
  // we know where to look for the label.
  olayer ol("S_goto::itprint");
  env << "goto ";
  env << target;
  env << ";\n";
}

void S_decl::iprint(PrintEnv &env)
{
  olayer ol("S_decl::itprint");
  decl->print(env);
  //      env << ";\n";
}

void S_try::iprint(PrintEnv &env)
{
  olayer ol("S_try::itprint");
  env << "try";
  body->print(env);
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->print(env);
  }
}

// ------------------- Condition --------------------
// CN = ConditioN

// this situation: if (gronk()) {...
void CN_expr::print(PrintEnv &env)
{
  olayer ol("CN_expr");
  expr->print(env);
}

// this situation: if (bool b=gronk()) {...
void CN_decl::print(PrintEnv &env)
{
  olayer ol("CN_decl");
  typeId->print(env);
}

// ------------------- Handler ----------------------
// catch clause
void HR_type::print(PrintEnv &env)
{           
  olayer ol("HR_type");
  {
    codeout co(env, "catch", "(", ")");
    typeId->print(env);
  }
  body->print(env);
}

void HR_default::print(PrintEnv &env)
{
  olayer ol("HR_default");
  env << "catch (...)";
  body->print(env);
}

// ------------------- Expression print -----------------------
void Expression::print(PrintEnv &env)
{
  olayer ol("Expression");
  codeout co(env, "", "(", ")");   // this will put parens around every expression
  iprint(env);
}

string Expression::exprToString() const
{              
  stringBuilder sb;
  PrintEnv env(sb);
  
  // sm: I think all the 'print' methods should be 'const', but
  // I'll leave such a change up to this module's author (dsw)
  const_cast<Expression*>(this)->print(env);

  return sb;
}

// todo: move this someplace better
static char *staticBuffer(char const *s)
{
  static char buf[200];

  int len = strlen(s);
  if (len > 79) len=79;
  memcpy(buf, s, len);
  buf[len] = 0;

  return buf;
}

char *expr_toString(Expression *e)
{
  return staticBuffer(e->exprToString());
}


void E_boolLit::iprint(PrintEnv &env)
{
  olayer ol("E_boolLit::itprint");
  env << b;
}

void E_intLit::iprint(PrintEnv &env)
{
  olayer ol("E_intLit::itprint");
  env << i;
}

void E_floatLit::iprint(PrintEnv &env)
{                                
  olayer ol("E_floatLit::itprint");
  env << f;
}

void E_stringLit::iprint(PrintEnv &env)
{                                                                     
  olayer ol("E_stringLit::itprint");
  env << "\"" << encodeWithEscapes(s) << "\"";
}

void E_charLit::iprint(PrintEnv &env)
{                               
  olayer ol("E_charLit::itprint");
  env << "'" << encodeWithEscapes(&c, 1) << "'";
}

void E_variable::iprint(PrintEnv &env)
{
  olayer ol("E_variable::itprint");
  env << name->qualifierString();
  env << var->name;
}

void printFakeExprList(FakeList<Expression> *list, PrintEnv &env)
{
  olayer ol("printFakeExprList");
  bool first_time = true;
  FAKELIST_FOREACH_NC(Expression, list, iter) {
    if (first_time) first_time = false;
    else env << ", ";
    iter->print(env);
  }
}

void E_funCall::iprint(PrintEnv &env)
{
  olayer ol("E_funCall::itprint");
  func->print(env);
  codeout co(env, "", "(", ")");
  printFakeExprList(args, env);
}

void E_constructor::iprint(PrintEnv &env)
{
  olayer ol("E_constructor::itprint");
  env << type->toString();
  codeout co(env, "", "(", ")");
  printFakeExprList(args, env);
}

void E_fieldAcc::iprint(PrintEnv &env)
{
  olayer ol("E_fieldAcc::itprint");
  obj->print(env);
  env << ".";
  if (field) {
    env << field->name;
  }
  else {
    // the 'field' remains NULL if we're in a template
    // function and the 'obj' is dependent on the template
    // arguments.. there are probably a few other places
    // lurking that will need similar treatment, because
    // typechecking of templates is very incomplete and in
    // any event when checking the template *itself* (as
    // opposed to an instantiation) we never have enough
    // information to fill in all the variable references..
    env << fieldName->toString();
  }
}

void E_sizeof::iprint(PrintEnv &env)
{
  olayer ol("E_sizeof::itprint");
  // NOTE parens are no necessary because its an expression, not a
  // type.
  env << "sizeof";
  expr->print(env);           // putting parens in here so we are safe wrt precedence
}

// dsw: unary expression?
void E_unary::iprint(PrintEnv &env)
{
  olayer ol("E_unary::itprint");
  env << toString(op);
  expr->print(env);
}

void E_effect::iprint(PrintEnv &env)
{
  olayer ol("E_effect::itprint");
  if (!isPostfix(op)) env << toString(op);
  expr->print(env);
  if (isPostfix(op)) env << toString(op);
}

// dsw: binary operator.
void E_binary::iprint(PrintEnv &env)
{
  olayer ol("E_binary::itprint");
  e1->print(env);
  env << toString(op);
  e2->print(env);
}

void E_addrOf::iprint(PrintEnv &env)
{
  olayer ol("E_addrOf::itprint");
  env << "&";
  expr->print(env);
}

void E_deref::iprint(PrintEnv &env)
{
  olayer ol("E_deref::itprint");
  env << "*";
  ptr->print(env);
}

// C-style cast
void E_cast::iprint(PrintEnv &env)
{
  olayer ol("E_cast::itprint");
  {
    codeout co(env, "", "(", ")");
    ctype->print(env);
  }
  expr->print(env);
}

// ? : syntax
void E_cond::iprint(PrintEnv &env)
{
  olayer ol("E_cond::itprint");
  cond->print(env);
  env << "?";
  th->print(env);
  env << ":";
  el->print(env);
}

void E_comma::iprint(PrintEnv &env)
{
  olayer ol("E_comma::itprint");
  e1->print(env);
  env << ",";
  e2->print(env);
}

void E_sizeofType::iprint(PrintEnv &env)
{
  olayer ol("E_sizeofType::itprint");
  codeout co(env, "sizeof", "(", ")"); // NOTE yes, you do want the parens because argument is a type.
  atype->print(env);
}

void E_assign::iprint(PrintEnv &env)
{
  olayer ol("E_assign::itprint");
  target->print(env);
  if (op!=BIN_ASSIGN) env << toString(op);
  env << "=";
  src->print(env);
}

void E_new::iprint(PrintEnv &env)
{
  olayer ol("E_new::itprint");
  if (colonColon) env << "::";
  env << "new ";
  if (placementArgs) {
    codeout co(env, "", "(", ")");
    printFakeExprList(placementArgs, env);
  }

  if (!arraySize) {
    // no array size, normal type-id printing is fine
    atype->print(env);
  }
  else {
    // sm: to correctly print new-declarators with array sizes, we
    // need to dig down a bit, because the arraySize is printed right
    // where the variable name would normally go in an ordinary
    // declarator
    //
    // for example, suppose the original syntax was
    //   new int [n][5];
    // the type-id of the object being allocated is read as
    // "array of 5 ints" and 'n' of them are created; so:
    //   "array of 5 ints"->leftString()   is "int"
    //   arraySize->print()                is "n"
    //   "array of 5 ints"->rightString()  is "[5]"
    Type const *t = atype->decl->var->type;   // type-id in question
    env << t->leftString() << " [";
    arraySize->print(env);
    env << "]" << t->rightString();
  }

  if (ctorArgs) {
    codeout co(env, "", "(", ")");
    printFakeExprList(ctorArgs->list, env);
  }
}

void E_delete::iprint(PrintEnv &env)
{
  olayer ol("E_delete::itprint");
  if (colonColon) env << "::";
  env << "delete";
  if (array) env << "[]";
  expr->print(env);
}

void E_throw::iprint(PrintEnv &env)
{
  olayer ol("E_throw::itprint");
  env << "throw";
  if (expr) expr->print(env);
}

// C++-style cast
void E_keywordCast::iprint(PrintEnv &env)
{
  olayer ol("E_keywordCast::itprint");
  env << toString(key);
  {
    codeout co(env, "", "<", ">");
    type->print(env);
  }
  codeout co(env, "", "(", ")");
  expr->print(env);
}

// RTTI: typeid(expression)
void E_typeidExpr::iprint(PrintEnv &env)
{
  olayer ol("E_typeidExpr::itprint");
  codeout co(env, "typeid", "(", ")");
  expr->print(env);
}

// RTTI: typeid(type)
void E_typeidType::iprint(PrintEnv &env)
{
  olayer ol("E_typeidType::itprint");
  codeout co(env, "typeid", "(", ")");
  type->print(env);
}

// ----------------------- Initializer --------------------

// this is under a declaration
// int x = 3;
//         ^ only
void IN_expr::print(PrintEnv &env)
{
  olayer ol("IN_expr");
  e->print(env);
}

// int x[] = {1, 2, 3};
//           ^^^^^^^^^ only
void IN_compound::print(PrintEnv &env)
{
  olayer ol("IN_compound");
  codeout co(env, "", "{", "}");
  bool first_time = true;
  FOREACH_ASTLIST_NC(Initializer, inits, iter) {
    if (first_time) first_time = false;
    else env << ",";
    iter.data()->print(env);
  }
}

void IN_ctor::print(PrintEnv &env)
{
  printFakeExprList(args, env);
}


// InitLabel

// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::print(PrintEnv &env)
{ 
  // sm: the declared variable knows it is a template, and
  // knows what its parameters are, so it will print that
  // stuff (e.g. "template <...>")
  //FAKELIST_FOREACH_NC(TemplateParameter, params, iter) {
  //  iter->print(env);
  //}

  iprint(env);
}

void TD_func::iprint(PrintEnv &env)
{
  f->print(env);
}

void TD_class::iprint(PrintEnv &env)
{
  type->print(env);
}

// ------------------- TemplateParameter ------------------
// sm: this isn't used..
void TP_type::print(PrintEnv &env)
{
  env << "class " << name;
                          
  if (defaultType) {
    env << " = ";
    defaultType->print(env);
  }
}


// -------------------- TemplateArgument ------------------
void TA_type::print(PrintEnv &env)
{
  type->print(env);
}
