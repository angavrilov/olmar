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

// This is a dummy global so that this file will compile both in
// default mode and in qualifiers mode.
class dummy_type;               // This does nothing.
dummy_type *ql;
string toString(class dummy_type*) {return "";}
  
// sm: folded this into the PrintEnv
//SourceLocation current_loc;

string make_indentation(int n) {
  stringBuilder s;
  for (int i=0; i<n; ++i) s << "  ";
  return s;
}

string indent_message(int n, rostring message) {
  stringBuilder s;
  char const *m = message.c_str();
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
string declaration_toString (
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
    s << type->asFunctionTypeC()->retType->toString() << " ()";
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
      if (type->isFunctionType() &&
          var->templateInfo() &&
          var->templateInfo()->isCompleteSpecOrInstantiation()) {
        // print the spec/inst args after the function name
        sb << sargsToString(var->templateInfo()->arguments);
      }
      sb << var->namePrintSuffix();    // hook for verifier
      s << type->toCString(sb);
//        if (type->isTemplateClass()) {
//          cout << "TEMPLATE CLASS" << endl;
//        } else if (type->isTemplateFunction()) {
//          cout << "TEMPLATE FUNCTION" << endl;
//        }
    } else {
      s << type->toCString(finalName);
    }
  }

  return s;
}


// more specialized version of the previous function
string var_toString(Variable *var, PQName const * /*nullable*/ pqname)
{
  olayer ol("var_toString");
  return declaration_toString(var->flags, var->type, pqname, var);
}


// this is a prototype for a function down near E_funCall::iprint
void printArgExprList(FakeList<ArgExpression> *list, PrintEnv &env);


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
  olayer ol("TF_template");
  env.current_loc = loc;
  td->print(env);
}

void TF_explicitInst::print(PrintEnv &env)
{
  olayer ol("TF_explicitInst");
  env.current_loc = loc;
  env << "template ";
  d->print(env);
}

void TF_linkage::print(PrintEnv &env)
{         
  olayer ol("TF_linkage");
  env.current_loc = loc;
  env << "extern " << linkageType;
  codeout co(env, "", " {\n", "}\n");
  forms->print(env);
}

void TF_one_linkage::print(PrintEnv &env)
{
  olayer ol("TF_one_linkage");
  env.current_loc = loc;
  env << "extern " << linkageType << " ";
  form->print(env);
}

void TF_asm::print(PrintEnv &env)
{    
  olayer ol("TF_asm");
  env.current_loc = loc;
  env << "asm(" << text << ");\n";
}

void TF_namespaceDefn::print(PrintEnv &env)
{
  olayer ol("TF_namespaceDefn");
  env.current_loc = loc;
  env << "namespace " << (name? name : "/*anon*/") << " {\n";
  FOREACH_ASTLIST_NC(TopForm, forms, iter) {
    iter.data()->print(env);
  }
  env << "} /""* namespace " << (name? name : "(anon)") << " */\n";
}

void TF_namespaceDecl::print(PrintEnv &env)
{
  olayer ol("TF_namespaceDecl");
  env.current_loc = loc;
  decl->print(env);
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

  if (instButNotTchecked()) {
    // this is an unchecked instantiation
    env << "; // not instantiated\n";
    return;
  }

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
      printArgExprList(iter->args, env);
    }
  }

  if (handlers) env << "\ntry";

  if (body->stmts.isEmpty()) {
    // more concise
    env << " {}\n";
  }
  else {
    body->print(env);
  }

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
    env << ";\n";
  }
  else if(spec->isTS_enumSpec()) {
    spec->asTS_enumSpec()->print(env);
    env << ";\n";
  }
  
  // TODO: this does not print "friend class Foo;" declarations
  // because the type specifier is TS_elaborated and there are no
  // declarators

  FAKELIST_FOREACH_NC(Declarator, decllist, iter) {
    // if there are decl flags that didn't get put into the
    // Variable (e.g. DF_EXTERN which gets turned off as soon
    // as a definition is seen), print them first
    DeclFlags extras = (DeclFlags)(dflags & ~(iter->var->flags));
    if (extras) {
      env << toString(extras) << " ";
    }

    // TODO: this will not work if there is more than one declarator ...

    iter->print(env);
    env << ";" << endl;
  }
}

//  -------------------- ASTTypeId -------------------
void ASTTypeId::print(PrintEnv &env)
{
  olayer ol("ASTTypeId");
  env << getType()->toString();
  if (decl->getDeclaratorId()) {
    env << " ";
    decl->getDeclaratorId()->print(env);
  }
  
  if (decl->init) {
    env << " = ";
    decl->init->print(env);
  }
}

// ---------------------- PQName -------------------
void printTemplateArgumentList(PrintEnv &env, /*fakelist*/TemplateArgument *args)
{
  int ct=0;
  while (args) {
    if (!args->isTA_templateUsed()) {
      if (ct++ > 0) {
        env << ", ";
      }
      args->print(env);
    }

    args = args->next;
  }
}

void PQ_qualifier::print(PrintEnv &env)
{
  if (templateUsed()) {
    env << "template ";
  }

  env << qualifier;
  if (templArgs/*isNotEmpty*/) {
    env << "<";
    printTemplateArgumentList(env, templArgs);
    env << ">";
  }
  env << "::";
  rest->print(env);
}

void PQ_name::print(PrintEnv &env)
{
  env << name;
}

void PQ_operator::print(PrintEnv &env)
{
  env << fakeName;
}

void PQ_template::print(PrintEnv &env)
{
  if (templateUsed()) {
    env << "template ";
  }

  env << name << "<";
  printTemplateArgumentList(env, templArgs);
  env << ">";
}


// --------------------- TypeSpecifier --------------
void TS_name::print(PrintEnv &env)
{
  xassert(0);                   // I'll bet this is never called.
//    olayer ol("TS_name");
//    env << toString(ql);          // see string toString(class dummy_type*) above
//    name->print(env);
}

void TS_simple::print(PrintEnv &env)
{
  xassert(0);                   // I'll bet this is never called.
//    olayer ol("TS_simple");
//    env << toString(ql);          // see string toString(class dummy_type*) above
}

void TS_elaborated::print(PrintEnv &env)
{
  olayer ol("TS_elaborated");
  env.current_loc = loc;
  env << toString(ql);          // see string toString(class dummy_type*) above
  env << toString(keyword) << " ";
  name->print(env);
}

void TS_classSpec::print(PrintEnv &env)
{
  olayer ol("TS_classSpec");
  env << toString(ql);          // see string toString(class dummy_type*) above
  env << toString(cv);
  env << toString(keyword) << " ";
  if (name) env << name->toString();
  bool first_time = true;
  FAKELIST_FOREACH_NC(BaseClassSpec, bases, iter) {
    if (first_time) {
      env << " : ";
      first_time = false;
    }
    else env << ", ";
    iter->print(env);
  }
  codeout co(env, "", "{\n", "}");
  FOREACH_ASTLIST_NC(Member, members->list, iter2) {
    iter2.data()->print(env);
  }
}

void TS_enumSpec::print(PrintEnv &env)
{
  olayer ol("TS_classSpec");
  env << toString(ql);          // see string toString(class dummy_type*) above
  env << toString(cv);
  env << "enum ";
  if (name) env << name;
  codeout co(env, "", "{\n", "}");
  FAKELIST_FOREACH_NC(Enumerator, elts, iter) {
    iter->print(env);
    env << "\n";
  }
}

// BaseClass
void BaseClassSpec::print(PrintEnv &env) {
  olayer ol("BaseClassSpec");
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
  olayer ol("MR_publish");
  env << name->toString() << ";\n";
}

void MR_usingDecl::print(PrintEnv &env)
{
  olayer ol("MR_usingDecl");
  decl->print(env);
}

void MR_template::print(PrintEnv &env)
{
  olayer ol("MR_template");
  d->print(env);
}


// -------------------- Enumerator --------------------
void Enumerator::print(PrintEnv &env)
{
  olayer ol("Enumerator");
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
//    if (var->type->isTemplateClass()) {
//      env << "/*TEMPLATE CLASS*/" << endl;
//    } else if (var->type->isTemplateFunction()) {
//      env << "/*TEMPLATE FUNCTION*/" << endl;
//    }
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
      // sm: don't print "()" as an IN_ctor initializer (cppstd 8.5 para 8)
      if (ctor->args->isEmpty()) {
        env << " /*default-ctor-init*/";
      }
      else {
        // dsw:Constructor arguments.
        codeout co(env, "", "(", ")");
        ctor->print(env);       // NOTE: You can NOT factor this line out of the if!
      }
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
  olayer ol("S_skip::iprint");
  env << ";\n";
}

void S_label::iprint(PrintEnv &env)
{
  olayer ol("S_label::iprint");
  env << name << ":";
  s->print(env);
}

void S_case::iprint(PrintEnv &env)
{                    
  olayer ol("S_case::iprint");
  env << "case";
  expr->print(env);
  env << ":";
  s->print(env);
}

void S_default::iprint(PrintEnv &env)
{
  olayer ol("S_default::iprint");
  env << "default:";
  s->print(env);
}

void S_expr::iprint(PrintEnv &env)
{
  olayer ol("S_expr::iprint");
  expr->print(env);
  env << ";\n";
}

void S_compound::iprint(PrintEnv &env)
{ 
  olayer ol("S_compound::iprint");
  codeout co(env, "", "{\n", "}\n");
  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    iter.data()->print(env);
  }
}

void S_if::iprint(PrintEnv &env)
{
  olayer ol("S_if::iprint");
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
  olayer ol("S_switch::iprint");
  {
    codeout co(env, "switch", "(", ")");
    cond->print(env);
  }
  branches->print(env);
}

void S_while::iprint(PrintEnv &env)
{
  olayer ol("S_while::iprint");
  {
    codeout co(env, "while", "(", ")");
    cond->print(env);
  }
  body->print(env);
}

void S_doWhile::iprint(PrintEnv &env)
{
  olayer ol("S_doWhile::iprint");
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
  olayer ol("S_for::iprint");
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
  olayer ol("S_break::iprint");
  env << "break";
  env << ";\n";
}

void S_continue::iprint(PrintEnv &env)
{
  olayer ol("S_continue::iprint");
  env << "continue";
  env << ";\n";
}

void S_return::iprint(PrintEnv &env)
{
  olayer ol("S_return::iprint");
  env << "return";
  if (expr) expr->print(env);
  env << ";\n";
}

void S_goto::iprint(PrintEnv &env)
{
  // dsw: When doing a control-flow pass, keep a current function so
  // we know where to look for the label.
  olayer ol("S_goto::iprint");
  env << "goto ";
  env << target;
  env << ";\n";
}

void S_decl::iprint(PrintEnv &env)
{
  olayer ol("S_decl::iprint");
  decl->print(env);
  //      env << ";\n";
}

void S_try::iprint(PrintEnv &env)
{
  olayer ol("S_try::iprint");
  env << "try";
  body->print(env);
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->print(env);
  }
}

void S_asm::iprint(PrintEnv &env)
{
  olayer ol("S_asm::iprint");
  env << "asm(" << text << ");\n";
}

void S_namespaceDecl::iprint(PrintEnv &env)
{
  olayer ol("S_namespaceDecl::iprint");
  decl->print(env);
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
void Handler::print(PrintEnv &env)
{
  olayer ol("Handler");
  {
    codeout co(env, "catch", "(", ")");
    if (isEllipsis()) {
      env << "...";
    }
    else {
      typeId->print(env);
    }
  }
  body->print(env);
}


// ------------------- Full Expression print -----------------------
void FullExpression::print(PrintEnv &env)
{
  olayer ol("FullExpression");
  // FIX: for now I omit printing the declarations of the temporaries
  // since we really don't have a syntax for it.  We would have to
  // print some curlies somewhere to make it legal to parse it back in
  // again, and we aren't using E_statement, so it would not reflect
  // the actual ast.
  expr->print(env);
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
  olayer ol("Expression::exprToString");
  stringBuilder sb;
  PrintEnv env(sb);
  
  // sm: I think all the 'print' methods should be 'const', but
  // I'll leave such a change up to this module's author (dsw)
  const_cast<Expression*>(this)->print(env);

  return sb;
}

string renderExpressionAsString(char const *prefix, Expression const *e)
{
  return stringc << prefix << e->exprToString();
}

char *expr_toString(Expression const *e)
{               
  // this function is defined in smbase/strutil.cc
  return copyToStaticBuffer(e->exprToString().c_str());
}

int expr_debugPrint(Expression const *e)
{
  e->debugPrint(cout, 0);
  return 0;    // make gdb happy?
}


void E_boolLit::iprint(PrintEnv &env)
{
  olayer ol("E_boolLit::iprint");
  env << b;
}

void E_intLit::iprint(PrintEnv &env)
{
  olayer ol("E_intLit::iprint");
  // FIX: do this correctly from the internal representation
  // fails to print the trailing U for an unsigned int.
//    env << i;
  env << text;
}

void E_floatLit::iprint(PrintEnv &env)
{                                
  olayer ol("E_floatLit::iprint");
  // FIX: do this correctly from the internal representation
  // this fails to print ".0" for a float/double that happens to lie
  // on an integer boundary
//    env << d;
  // doing it this way also preserves the trailing "f" for float
  // literals
  env << text;
}

void E_stringLit::iprint(PrintEnv &env)
{                                                                     
  olayer ol("E_stringLit::iprint");
  
  E_stringLit *p = this;
  while (p) {
    env << p->text;
    p = p->continuation;
    if (p) {
      env << " ";
    }
  }
}

void E_charLit::iprint(PrintEnv &env)
{
  olayer ol("E_charLit::iprint");
  env << text;
}

void E_this::iprint(PrintEnv &env)
{
  olayer ol("E_this::iprint");
  env << "this";
}

// print template args, if any
void printTemplateArgs(PrintEnv &env, Variable *var)
{
  if (!( var && var->templateInfo() )) {
    return;
  }

  TemplateInfo *tinfo = var->templateInfo();
  int totArgs = tinfo->arguments.count();
  if (totArgs == 0) {
    return;
  }

  // use only arguments that apply to non-inherited parameters
  int args = totArgs;
  if (tinfo->isInstantiation()) {
    args = tinfo->instantiationOf->templateInfo()->params.count();
    if (args == 0) {
      return;
    }
  }

  // print final 'args' arguments
  ObjListIter<STemplateArgument> iter(var->templateInfo()->arguments);
  for (int i=0; i < (totArgs-args); i++) {
    iter.adv();
  }
  env << "<";
  int ct=0;
  for (; !iter.isDone(); iter.adv()) {
    if (ct++ > 0) {
      env << ", ";
    }
    env << iter.data()->toString();
  }
  env << ">";
}

void E_variable::iprint(PrintEnv &env)
{
  olayer ol("E_variable::iprint");
  if (var && var->isBoundTemplateParam()) {
    // this is a bound template variable, so print its value instead
    // of printing its name
    xassert(var->value);
    var->value->print(env);
  }
  else {
    env << name->qualifierString() << name->getName();
    printTemplateArgs(env, var);
  }
}

void printArgExprList(FakeList<ArgExpression> *list, PrintEnv &env)
{
  olayer ol("printArgExprList");
  bool first_time = true;
  FAKELIST_FOREACH_NC(ArgExpression, list, iter) {
    if (first_time) first_time = false;
    else env << ", ";
    iter->expr->print(env);
  }
}

void E_funCall::iprint(PrintEnv &env)
{
  olayer ol("E_funCall::iprint");
  func->print(env);
  codeout co(env, "", "(", ")");
  printArgExprList(args, env);
}

void E_constructor::iprint(PrintEnv &env)
{
  olayer ol("E_constructor::iprint");
  env << type->toString();
  codeout co(env, "", "(", ")");
  printArgExprList(args, env);
}

void E_fieldAcc::iprint(PrintEnv &env)
{
  olayer ol("E_fieldAcc::iprint");
  obj->print(env);
  env << ".";
  if (field &&
      !field->type->isDependent()) {
    env << field->name;
    printTemplateArgs(env, field);
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

void E_arrow::iprint(PrintEnv &env)
{
  olayer ol("E_arrow::iprint");
  
  // E_arrow shouldn't normally be present in code that is to be
  // prettyprinted, so it doesn't much matter what this does.
  obj->print(env);
  env << "->";
  fieldName->print(env);
}

void E_sizeof::iprint(PrintEnv &env)
{
  olayer ol("E_sizeof::iprint");
  // NOTE parens are not necessary because it's an expression, not a
  // type.
  env << "sizeof";
  expr->print(env);           // putting parens in here so we are safe wrt precedence
}

// dsw: unary expression?
void E_unary::iprint(PrintEnv &env)
{
  olayer ol("E_unary::iprint");
  env << toString(op);
  expr->print(env);
}

void E_effect::iprint(PrintEnv &env)
{
  olayer ol("E_effect::iprint");
  if (!isPostfix(op)) env << toString(op);
  expr->print(env);
  if (isPostfix(op)) env << toString(op);
}

// dsw: binary operator.
void E_binary::iprint(PrintEnv &env)
{
  olayer ol("E_binary::iprint");
  e1->print(env);
  if (op != BIN_BRACKETS) {
    env << toString(op);
    e2->print(env);
  }
  else {
    env << "[";
    e2->print(env);
    env << "]";
  }
}

void E_addrOf::iprint(PrintEnv &env)
{
  olayer ol("E_addrOf::iprint");
  env << "&";
  if (expr->isE_variable()) {
    // could be forming ptr-to-member, do not parenthesize
    expr->iprint(env);
  }
  else {
    expr->print(env);
  }
}

void E_deref::iprint(PrintEnv &env)
{
  olayer ol("E_deref::iprint");
  env << "*";
  ptr->print(env);
}

// C-style cast
void E_cast::iprint(PrintEnv &env)
{
  olayer ol("E_cast::iprint");
  {
    codeout co(env, "", "(", ")");
    ctype->print(env);
  }
  expr->print(env);
}

// ? : syntax
void E_cond::iprint(PrintEnv &env)
{
  olayer ol("E_cond::iprint");
  cond->print(env);
  env << "?";
  // In gcc it is legal to omit the 'then' part;
  // http://gcc.gnu.org/onlinedocs/gcc-3.4.1/gcc/Conditionals.html#Conditionals
  if (th) {
    th->print(env);
  }
  env << ":";
  el->print(env);
}

void E_sizeofType::iprint(PrintEnv &env)
{
  olayer ol("E_sizeofType::iprint");
  codeout co(env, "sizeof", "(", ")"); // NOTE yes, you do want the parens because argument is a type.
  atype->print(env);
}

void E_assign::iprint(PrintEnv &env)
{
  olayer ol("E_assign::iprint");
  target->print(env);
  if (op!=BIN_ASSIGN) env << toString(op);
  env << "=";
  src->print(env);
}

void E_new::iprint(PrintEnv &env)
{
  olayer ol("E_new::iprint");
  if (colonColon) env << "::";
  env << "new ";
  if (placementArgs) {
    codeout co(env, "", "(", ")");
    printArgExprList(placementArgs, env);
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
    printArgExprList(ctorArgs->list, env);
  }
}

void E_delete::iprint(PrintEnv &env)
{
  olayer ol("E_delete::iprint");
  if (colonColon) env << "::";
  env << "delete";
  if (array) env << "[]";
  // dsw: this can be null because elaboration can remove syntax when
  // it is replaced with other syntax
  if (expr) {
    expr->print(env);
  }
}

void E_throw::iprint(PrintEnv &env)
{
  olayer ol("E_throw::iprint");
  env << "throw";
  if (expr) expr->print(env);
}

// C++-style cast
void E_keywordCast::iprint(PrintEnv &env)
{
  olayer ol("E_keywordCast::iprint");
  env << toString(key);
  {
    codeout co(env, "", "<", ">");
    ctype->print(env);
  }
  codeout co(env, "", "(", ")");
  expr->print(env);
}

// RTTI: typeid(expression)
void E_typeidExpr::iprint(PrintEnv &env)
{
  olayer ol("E_typeidExpr::iprint");
  codeout co(env, "typeid", "(", ")");
  expr->print(env);
}

// RTTI: typeid(type)
void E_typeidType::iprint(PrintEnv &env)
{
  olayer ol("E_typeidType::iprint");
  codeout co(env, "typeid", "(", ")");
  ttype->print(env);
}

void E_grouping::iprint(PrintEnv &env)
{
  olayer ol("E_grouping::iprint");
  
  // sm: given that E_grouping is now in the tree, and prints its
  // parentheses, perhaps we could eliminate some of the
  // paren-printing above?
  //codeout co(env, "", "(", ")");
  //
  // update:  Actually, it's a problem for E_grouping to print parens
  // because it messes up idempotency.  And, if we restored idempotency
  // by turning off paren-printing elsewhere, then we'd have a subtle
  // long-term problem that AST transformations would be required to
  // insert E_grouping when composing new expression trees, and that
  // would suck.  So I'll let E_grouping be a no-op, and continue to
  // idly plan some sort of precedence-aware paren-inserter mechanism.

  expr->iprint(env);    // iprint means Expression won't put parens either
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
  codeout co(env, "", "{\n", "\n}");
  bool first_time = true;
  FOREACH_ASTLIST_NC(Initializer, inits, iter) {
    if (first_time) first_time = false;
    else env << ",\n";
    iter.data()->print(env);
  }
}

void IN_ctor::print(PrintEnv &env)
{
  olayer ol("IN_ctor");
  printArgExprList(args, env);
}

// InitLabel

// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::print(PrintEnv &env)
{ 
  olayer ol("TemplateDeclaration");

  env << "template <";
  int ct=0;
  FAKELIST_FOREACH_NC(TemplateParameter, params, iter) {
    if (ct++ > 0) {
      env << ", ";
    }
    iter->print(env);
  }
  env << ">\n";

  iprint(env);
}

void printFuncInstantiations(PrintEnv &env, Variable const *var)
{
  TemplateInfo *ti = var->templateInfo();
  SFOREACH_OBJLIST(Variable, ti->instantiations, iter) {
    Variable const *inst = iter.data();
    if (inst->funcDefn) {
      inst->funcDefn->print(env);
    }
    else {
      env << inst->toQualifiedString() << ";    // decl but not defn\n";
    }
  }
}

void TD_func::iprint(PrintEnv &env)
{
  olayer ol("TD_func");
  f->print(env);

  // print instantiations
  Variable *var = f->nameAndParams->var;
  if (var->isTemplate() &&      // for complete specializations, don't print
      !var->templateInfo()->isPartialInstantiation()) {     // nor partial inst
    env << "#if 0    // instantiations of " << var->toString() << "\n";
    printFuncInstantiations(env, var);

    TemplateInfo *varTI = var->templateInfo();
    if (!varTI->definitionTemplateInfo) {
      // little bit of a hack: if this does not have a
      // 'definitionTemplateInfo', then it was defined inline, and
      // the partial instantiations will be printed when the class
      // instantiation is
    }
    else {
      // also look in partial instantiations
      SFOREACH_OBJLIST(Variable, varTI->partialInstantiations, iter) {
        printFuncInstantiations(env, iter.data());
      }
    }

    env << "#endif   // instantiations of " << var->name << "\n\n";
  }
}

void TD_decl::iprint(PrintEnv &env)
{
  d->print(env);

  // print instantiations
  if (d->spec->isTS_classSpec()) {
    CompoundType *ct = d->spec->asTS_classSpec()->ctype;
    TemplateInfo *ti = ct->typedefVar->templateInfo();
    if (!ti->isCompleteSpec()) {
      env << "#if 0    // instantiations of " << ct->name << "\n";

      SFOREACH_OBJLIST(Variable, ti->instantiations, iter) {
        Variable const *instV = iter.data();

        env << "// " << instV->type->toString();
        CompoundType *instCT = instV->type->asCompoundType();
        if (instCT->syntax) {
          env << "\n";
          instCT->syntax->print(env);
          env << ";\n";
        }
        else {
          env << ";     // body not instantiated\n";
        }
      }
      env << "#endif   // instantiations of " << ct->name << "\n\n";
    }
  }
  else {
    // it could be a forward declaration of a template class;
    // do nothing more
  }
}

void TD_tmember::iprint(PrintEnv &env)
{
  d->print(env);
}


// ------------------- TemplateParameter ------------------
// sm: this isn't used..
void TP_type::print(PrintEnv &env)
{
  olayer ol("TP_type");
  env << "class " << name;
                          
  if (defaultType) {
    env << " = ";
    defaultType->print(env);
  }
}

void TP_nontype::print(PrintEnv &env)
{
  olayer ol("TP_nontype");
  param->print(env);
}


// -------------------- TemplateArgument ------------------
void TA_type::print(PrintEnv &env)
{
  // dig down to prevent printing "/*anon*/" since template
  // type arguments are always anonymous so it's just clutter
  env << type->decl->var->type->toCString();
}

void TA_nontype::print(PrintEnv &env)
{
  expr->print(env);
}

void TA_templateUsed::print(PrintEnv &env)
{
  // the caller should have recognized the presence of TA_templateUsed,
  // adjusted its printing accordingly, and then skipped this element
  xfailure("do not print TA_templateUsed");
}


// -------------------- NamespaceDecl ---------------------
void ND_alias::print(PrintEnv &env)
{
  env << "namespace " << alias << " = ";
  original->print(env);
  env << ";\n";
}

void ND_usingDecl::print(PrintEnv &env)
{
  env << "using ";
  name->print(env);
  env << ";\n";
}

void ND_usingDir::print(PrintEnv &env)
{
  env << "using namespace ";
  name->print(env);
  env << ";\n";
}


// EOF
