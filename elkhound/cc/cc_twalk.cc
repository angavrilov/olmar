// cc_twalk.cc            see license.txt for copyright and terms of use

// Adapted from cc_tcheck.cc by Daniel Wilkerson dsw@cs.berkeley.edu

// This is a tree walk that prints out a functionally equivalent C++
// program to the original.

#include "cc.ast.gen.h"         // C++ AST
#include "cc_env.h"             // Env
#include "trace.h"              // trace
#include "strutil.h"            // string utilities
#include <stdlib.h>

class code_output_stream {
  std::ostream &out;

  public:
  code_output_stream(std::ostream &out) : out(out) {}
  void flush() {out.flush();}

  code_output_stream & operator << (const char *message) {
    out << message;
    out.flush();
    return *this;
  }
  code_output_stream & operator << (bool message) {
    out << message;
    out.flush();
    return *this;
  }
  code_output_stream & operator << (int message) {
    out << message;
    out.flush();
    return *this;
  }
  code_output_stream & operator << (long message) {
    out << message;
    out.flush();
    return *this;
  }
  code_output_stream & operator << (double message) {
    out << message;
    out.flush();
    return *this;
  }
  code_output_stream & operator << (ostream& (*manipfunc)(ostream& outs)) {
    out << manipfunc;
    out.flush();
    return *this;
  }
};

// where code output goes
code_output_stream global_code_out(cout);

// allow a block to have the equivalent of a finally block; the
// "close" argument to the constructor is printed in the destructor.
class codeout {
  char *close;
  public:
  codeout(const char *message, char *open = "", char *close = "")
    : close(close)
  {
    global_code_out << const_cast<char *>(message);
    global_code_out << " ";
    global_code_out << open;
  }
  ~codeout() {
    global_code_out << close;
  }
};

class twalk_output_stream {
  std::ostream &out;
//    FILE *out
  bool on;
  int depth;

  private:
  void indent() {
    out << endl;
//      fprintf(out, "\n");
    out.flush();
//      fflush(out);
    for(int i=0; i<depth; ++i) out << " ";
//      for(int i=0; i<depth; ++i) fprintf(out, " ");
    out.flush();
//      fflush(out);
    out << ":::::";
//      fprintf(out, ":::::");
    out.flush();
//      fflush(out);
  }

  public:
  twalk_output_stream(std::ostream &out, bool on = true)
//    twalk_output_stream(FILE *out, bool on = true)
    : out(out), on(on), depth(0) {}

  void flush() {out.flush();}
//    void flush() {fflush(out);}
  twalk_output_stream & operator << (char *message) {
    if (on) {
      indent();
      out << message;
//        fprintf(out, message);
      out.flush();
//        fflush(out);
    }
    return *this;
  }
  twalk_output_stream & operator << (ostream& (*manipfunc)(ostream& outs)) {
    if (on) out << manipfunc;
    return *this;
  }
  void down() {++depth;}
  void up() {--depth;}
};

// set this environment variable to see the twalk_layer debugging
// output
twalk_output_stream twalk_layer_out(cout, getenv("TWALK_VERBOSE"));
//  twalk_output_stream twalk_layer_out(cout, true);

class olayer {
  twalk_output_stream &out;
  public:
  olayer(char *message, twalk_output_stream &out = twalk_layer_out)
    : out(out) {
    out << message << endl;
    out.flush();
    out.down();
  }
  ~olayer() {out.up();}
};

SourceLocation current_loc;

// print with only flags that were in the source
//  string var_toString(Variable *var, string qualifierName) {
string var_toString(Variable *var, PQName const * /*nullable*/ pqname)
{
  stringBuilder s;

  if (pqname && 0==strcmp(pqname->getName(), "conversion-operator")) {
    // special syntax for conversion operators; first the keyword
    s << "operator ";

    // then the return type and the function designator
    s << var->type->asFunctionTypeC().retType->toString() << " ()";
  }

  else if (var && var->name && 0==strcmp(var->name, "constructor-special")) {
    s << var->type->toCString(var->scope->curCompound->name);
  }

  else {
    s << toString( (DeclFlags) (var->flags & DF_SOURCEFLAGS) );
    s << " ";
    //s << var->type->toCString(qualifierName);
    s << var->toString();
  }

  return s;
}

// ------------------- TranslationUnit --------------------
void TranslationUnit::twalk(Env &env)
{
  olayer ol("TranslationUnit");
  FOREACH_ASTLIST_NC(TopForm, topForms, iter) {
    iter.data()->twalk(env);
  }
}

// --------------------- TopForm ---------------------
void TF_decl::twalk(Env &env)
{
  olayer ol("TF_decl");
  current_loc = loc;
  decl->twalk(env);
}

void TF_func::twalk(Env &env)
{
  olayer ol("TF_func");
  global_code_out << endl;
  current_loc = loc;
  f->twalk(env);
}

void TF_template::twalk(Env &env)
{
  current_loc = loc;
  td->twalk(env);
}

void TF_linkage::twalk(Env &env)
{
  current_loc = loc;
  forms->twalk(env);
}

// --------------------- Function -----------------
void Function::twalk(Env &env)
{
  olayer ol("Function");
  //global_code_out << var_toString(nameAndParams->var, nameAndParams->decl->decl->getDeclaratorId());
  nameAndParams->twalk(env);
  if (inits) twalk_memberInits(env);
  body->twalk(env);
  if (handlers) twalk_handlers(env);
}

// this is a prototype for a function down near E_funCall::itwalk
void twalkFakeExprList(FakeList<Expression> *list, Env &env);

// This is only for constructor "functions"
void Function::twalk_memberInits(Env &env)
{
  olayer ol("Function::twalk_memberInits");
  global_code_out << ":";
  bool first_time = true;
  FAKELIST_FOREACH_NC(MemberInit, inits, iter) {
    if (first_time) first_time = false;
    else global_code_out << ",";
    // NOTE: eventually will be able to figure out if we are
    // initializing a base class or a member variable.  There will
    // be a field added to class MemberInit that will say.
    codeout co(iter->name->toString(), "(", ")");
    twalkFakeExprList(iter->args, env);
  }
}

// exception handlers for constructors
void Function::twalk_handlers(Env &env)
{
  olayer ol("Function::twalk_handlers");
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->twalk(env);
  }
}

// MemberInit

// -------------------- Declaration -------------------
void Declaration::twalk(Env &env)
{
  olayer ol("Declaration");
  if(spec->isTS_classSpec()) {
    spec->asTS_classSpec()->twalk(env);
  }
  FAKELIST_FOREACH_NC(Declarator, decllist, iter) {
    // if there are decl flags that didn't get put into the
    // Variable (e.g. DF_EXTERN which gets turned off as soon
    // as a definition is seen), print them first
    DeclFlags extras = (DeclFlags)(dflags & ~(iter->var->flags));
    if (extras) {
      global_code_out << toString(extras) << " ";
    }

    iter->twalk(env);
    global_code_out << ";" << endl;
  }
}

//  -------------------- ASTTypeId -------------------
void ASTTypeId::twalk(Env &env)
{
  olayer ol("ASTTypeId");
  decl->twalk(env);
}

// ---------------------- PQName -------------------
void PQ_qualifier::twalk(Env &env)
{
  FAKELIST_FOREACH_NC(TemplateArgument, targs, iter) {
    iter->twalk(env);
  }
  rest->twalk(env);
}

void PQ_name::twalk(Env &env)
{}

void PQ_operator::twalk(Env &env)
{}

void PQ_template::twalk(Env &env)
{
  FAKELIST_FOREACH_NC(TemplateArgument, args, iter) {
    iter->twalk(env);
  }
}

// --------------------- TypeSpecifier --------------
void TS_name::twalk(Env &env)
{
  name->twalk(env);
}

void TS_simple::twalk(Env &env)
{
}

void TS_elaborated::twalk(Env &env)
{
  current_loc = loc;
  name->twalk(env);
}

void TS_classSpec::twalk(Env &env)
{
  olayer ol("TS_classSpec");
  global_code_out << toString(cv);
  global_code_out << toString(keyword) << " ";
  if (name) global_code_out << toString(name);
  bool first_time = true;
  FAKELIST_FOREACH_NC(BaseClassSpec, bases, iter) {
    if (first_time) {
      global_code_out << ":";
      first_time = false;
    }
    else global_code_out << ",";
    iter->twalk(env);
  }
  codeout co("", "{\n", "};\n");
  FOREACH_ASTLIST_NC(Member, members->list, iter2) {
    iter2.data()->twalk(env);
  }
}

void TS_enumSpec::twalk(Env &env)
{
  current_loc = loc;
  FAKELIST_FOREACH_NC(Enumerator, elts, iter) {
    iter->twalk(env);
  }
}

// BaseClass
void BaseClassSpec::twalk(Env &) {
  if (isVirtual) global_code_out << "virtual ";
  if (access!=AK_UNSPECIFIED) global_code_out << toString(access) << " ";
  global_code_out << name->toString();
}

// MemberList

// ---------------------- Member ----------------------
void MR_decl::twalk(Env &env)
{                   
  olayer ol("MR_decl");
  d->twalk(env);
}

void MR_func::twalk(Env &env)
{
  olayer ol("MR_func");
  f->twalk(env);
}

void MR_access::twalk(Env &env)
{
  olayer ol("MR_access");
  global_code_out << toString(k) << ":\n";
}

void MR_publish::twalk(Env &env)
{
  // dsw: Not sure why this has to be here.
}

// -------------------- Enumerator --------------------
void Enumerator::twalk(Env &env)
{
  if (expr) {
    expr->twalk(env);
  }
}

// -------------------- Declarator --------------------
void Declarator::twalk(Env &env)
{
  olayer ol("Declarator");
  global_code_out << var_toString(var, decl->getDeclaratorId());
  D_bitfield *b = dynamic_cast<D_bitfield*>(decl);
  if (b) {
    global_code_out << ":";
    b->bits->twalk(env);
  }
//    var_toString(var, decl->getDeclaratorId()->toString());
  if (init) {
    global_code_out << "=";
    init->twalk(env);
  }
}

// ------------------- ExceptionSpec --------------------
void ExceptionSpec::twalk(Env &env)
{
  olayer ol("ExceptionSpec");
  global_code_out << "throw"; // Scott says this is right.
  FAKELIST_FOREACH_NC(ASTTypeId, types, iter) {
    iter->twalk(env);
  }
}

// ---------------------- Statement ---------------------
void Statement::twalk(Env &env)
{
  olayer ol("Statement");
  current_loc = loc;
  itwalk(env);
  //    global_code_out << ";\n";
}

// no-op
void S_skip::itwalk(Env &env)
{
  olayer ol("S_skip::itwalk");
  global_code_out << ";\n";
}

void S_label::itwalk(Env &env)
{
  olayer ol("S_label::itwalk");
  global_code_out << name << ":";
  s->twalk(env);
}

void S_case::itwalk(Env &env)
{                    
  olayer ol("S_case::itwalk");
  global_code_out << "case";
  expr->twalk(env);
  global_code_out << ":";
  s->twalk(env);
}

void S_default::itwalk(Env &env)
{
  olayer ol("S_default::itwalk");
  global_code_out << "default:";
  s->twalk(env);
}

void S_expr::itwalk(Env &env)
{
  olayer ol("S_expr::itwalk");
  expr->twalk(env);
  global_code_out << ";\n";
}

void S_compound::itwalk(Env &env)
{ 
  olayer ol("S_compound::itwalk");
  codeout co("", "{\n", "}\n");
  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    iter.data()->twalk(env);
  }
}

void S_if::itwalk(Env &env)
{
  olayer ol("S_if::itwalk");
  {
    codeout co("if", "(", ")");
    cond->twalk(env);
  }
  thenBranch->twalk(env);
  global_code_out << "else ";
  elseBranch->twalk(env);
}

void S_switch::itwalk(Env &env)
{
  olayer ol("S_switch::itwalk");
  {
    codeout co("switch", "(", ")");
    cond->twalk(env);
  }
  branches->twalk(env);
}

void S_while::itwalk(Env &env)
{
  olayer ol("S_while::itwalk");
  {
    codeout co("while", "(", ")");
    cond->twalk(env);
  }
  body->twalk(env);
}

void S_doWhile::itwalk(Env &env)
{
  olayer ol("S_doWhile::itwalk");
  {
    codeout co("do");
    body->twalk(env);
  }
  {
    codeout co("while", "(", ")");
    expr->twalk(env);
  }
  global_code_out << ";\n";
}

void S_for::itwalk(Env &env)
{
  olayer ol("S_for::itwalk");
  {
    codeout co("for", "(", ")");
    init->twalk(env);
    // this one not needed as the declaration provides one
    //          global_code_out << ";";
    cond->twalk(env);
    global_code_out << ";";
    after->twalk(env);
  }
  body->twalk(env);
}

void S_break::itwalk(Env &env)
{
  olayer ol("S_break::itwalk");
  global_code_out << "break";
  global_code_out << ";\n";
}

void S_continue::itwalk(Env &env)
{
  olayer ol("S_continue::itwalk");
  global_code_out << "continue";
  global_code_out << ";\n";
}

void S_return::itwalk(Env &env)
{
  olayer ol("S_return::itwalk");
  global_code_out << "return";
  if (expr) expr->twalk(env);
  global_code_out << ";\n";
}

void S_goto::itwalk(Env &env)
{
  // dsw: When doing a control-flow pass, keep a current function so
  // we know where to look for the label.
  olayer ol("S_goto::itwalk");
  global_code_out << "goto ";
  global_code_out << target;
  global_code_out << ";\n";
}

void S_decl::itwalk(Env &env)
{
  olayer ol("S_decl::itwalk");
  decl->twalk(env);
  //      global_code_out << ";\n";
}

void S_try::itwalk(Env &env)
{
  olayer ol("S_try::itwalk");
  global_code_out << "try";
  body->twalk(env);
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->twalk(env);
  }
}

// ------------------- Condition --------------------
// CN = ConditoN

// this situation: if (gronk()) {...
void CN_expr::twalk(Env &env)
{
  olayer ol("CN_expr");
  expr->twalk(env);
}

// this situation: if (bool b=gronk()) {...
void CN_decl::twalk(Env &env)
{
  olayer ol("CN_decl");
  typeId->twalk(env);
}

// ------------------- Handler ----------------------
// catch clause
void HR_type::twalk(Env &env)
{           
  olayer ol("HR_type");
  {
    codeout co("catch", "(", ")");
    typeId->twalk(env);
  }
  body->twalk(env);
}

void HR_default::twalk(Env &env)
{
  olayer ol("HR_default");
  global_code_out << "catch (...)";
  body->twalk(env);
}

// ------------------- Expression twalk -----------------------
void Expression::twalk(Env &env)
{
  olayer ol("Expression");
  codeout co("", "(", ")");   // this will put parens around every expression
  itwalk(env);
}

void E_boolLit::itwalk(Env &env)
{
  olayer ol("E_boolLit::itwalk");
  global_code_out << b;
}

void E_intLit::itwalk(Env &env)
{
  olayer ol("E_intLit::itwalk");
  global_code_out << i;
}

void E_floatLit::itwalk(Env &env)
{                                
  olayer ol("E_floatLit::itwalk");
  global_code_out << f;
}

void E_stringLit::itwalk(Env &env)
{                                                                     
  olayer ol("E_stringLit::itwalk");
  global_code_out << "\"" << encodeWithEscapes(s) << "\"";
}

void E_charLit::itwalk(Env &env)
{                               
  olayer ol("E_charLit::itwalk");
  global_code_out << "'" << encodeWithEscapes(&c, 1) << "'";
}

void E_variable::itwalk(Env &env)
{
  olayer ol("E_variable::itwalk");
  global_code_out << var->name;
}

void twalkFakeExprList(FakeList<Expression> *list, Env &env)
{
  olayer ol("twalkFakeExprList");
  bool first_time = true;
  FAKELIST_FOREACH_NC(Expression, list, iter) {
    if (first_time) first_time = false;
    else global_code_out << ",";
    iter->twalk(env);
  }
}

void E_funCall::itwalk(Env &env)
{
  olayer ol("E_funCall::itwalk");
  func->twalk(env);
  codeout co("", "(", ")");
  twalkFakeExprList(args, env);
}

void E_constructor::itwalk(Env &env)
{
  olayer ol("E_constructor::itwalk");
  global_code_out << type->toString();
  codeout co("", "(", ")");
  twalkFakeExprList(args, env);
}

void E_fieldAcc::itwalk(Env &env)
{
  olayer ol("E_fieldAcc::itwalk");
  obj->twalk(env);
  global_code_out << ".";
  global_code_out << field->name;
}

void E_sizeof::itwalk(Env &env)
{
  olayer ol("E_sizeof::itwalk");
  // NOTE parens are no necessary because its an expression, not a
  // type.
  global_code_out << "sizeof";
  expr->twalk(env);           // putting parens in here so we are safe wrt precedence
}

// dsw: unary expression?
void E_unary::itwalk(Env &env)
{
  olayer ol("E_unary::itwalk");
  global_code_out << toString(op);
  expr->twalk(env);
}

void E_effect::itwalk(Env &env)
{
  olayer ol("E_effect::itwalk");
  if (!isPostfix(op)) global_code_out << toString(op);
  expr->twalk(env);
  if (isPostfix(op)) global_code_out << toString(op);
}

// dsw: binary operator.
void E_binary::itwalk(Env &env)
{
  olayer ol("E_binary::itwalk");
  e1->twalk(env);
  global_code_out << toString(op);
  e2->twalk(env);
}

void E_addrOf::itwalk(Env &env)
{
  olayer ol("E_addrOf::itwalk");
  global_code_out << "&";
  expr->twalk(env);
}

void E_deref::itwalk(Env &env)
{
  olayer ol("E_deref::itwalk");
  global_code_out << "*";
  ptr->twalk(env);
}

// C-style cast
void E_cast::itwalk(Env &env)
{
  olayer ol("E_cast::itwalk");
  {
    codeout co("", "(", ")");
    ctype->twalk(env);
  }
  expr->twalk(env);
}

// ? : syntax
void E_cond::itwalk(Env &env)
{
  olayer ol("E_cond::itwalk");
  cond->twalk(env);
  global_code_out << "?";
  th->twalk(env);
  global_code_out << ":";
  el->twalk(env);
}

void E_comma::itwalk(Env &env)
{
  olayer ol("E_comma::itwalk");
  e1->twalk(env);
  global_code_out << ",";
  e2->twalk(env);
}

void E_sizeofType::itwalk(Env &env)
{
  olayer ol("E_sizeofType::itwalk");
  codeout co("sizeof", "(", ")"); // NOTE yes, you do want the parens because argument is a type.
  atype->twalk(env);
}

void E_assign::itwalk(Env &env)
{
  olayer ol("E_assign::itwalk");
  target->twalk(env);
  if (op!=BIN_ASSIGN) global_code_out << toString(op);
  global_code_out << "=";
  src->twalk(env);
}

void E_new::itwalk(Env &env)
{
  olayer ol("E_new::itwalk");
  if (colonColon) global_code_out << "::";
  global_code_out << "new";
  if (placementArgs) {
    codeout co("", "(", ")");
    twalkFakeExprList(placementArgs, env);
  }

  if (!arraySize) {
    // no array size, normal type-id printing is fine
    atype->twalk(env);
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
    //   arraySize->twalk()                is "n"
    //   "array of 5 ints"->rightString()  is "[5]"
    Type const *t = atype->decl->var->type;   // type-id in question
    global_code_out << " " << t->leftString() << " [";
    arraySize->twalk(env);
    global_code_out << "]" << t->rightString();
  }

  if (ctorArgs) {
    codeout co("", "(", ")");
    twalkFakeExprList(ctorArgs->list, env);
  }
}

void E_delete::itwalk(Env &env)
{
  olayer ol("E_delete::itwalk");
  if (colonColon) global_code_out << "::";
  global_code_out << "delete";
  if (array) global_code_out << "[]";
  expr->twalk(env);
}

void E_throw::itwalk(Env &env)
{
  olayer ol("E_throw::itwalk");
  global_code_out << "throw";
  if (expr) expr->twalk(env);
}

// C++-style cast
void E_keywordCast::itwalk(Env &env)
{
  olayer ol("E_keywordCast::itwalk");
  global_code_out << toString(key);
  {
    codeout co("", "<", ">");
    type->twalk(env);
  }
  codeout co("", "(", ")");
  expr->twalk(env);
}

// RTTI: typeid(expression)
void E_typeidExpr::itwalk(Env &env)
{
  olayer ol("E_typeidExpr::itwalk");
  codeout co("typeid", "(", ")");
  expr->twalk(env);
}

// RTTI: typeid(type)
void E_typeidType::itwalk(Env &env)
{
  olayer ol("E_typeidType::itwalk");
  codeout co("typeid", "(", ")");
  type->twalk(env);
}

// ----------------------- Initializer --------------------

// this is under a declaration
// int x = 3;
//         ^ only
void IN_expr::twalk(Env &env)
{
  olayer ol("IN_expr");
  e->twalk(env);
}

// int x[] = {1, 2, 3};
//           ^^^^^^^^^ only
void IN_compound::twalk(Env &env)
{
  olayer ol("IN_compound");
  codeout co("", "{", "}");
  bool first_time = true;
  FOREACH_ASTLIST_NC(Initializer, inits, iter) {
    if (first_time) first_time = false;
    else global_code_out << ",";
    iter.data()->twalk(env);
  }
}

void IN_ctor::twalk(Env &env)
{
  twalkFakeExprList(args, env);
}


// InitLabel

// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::twalk(Env &env)
{
  FAKELIST_FOREACH_NC(TemplateParameter, params, iter) {
    iter->twalk(env);
  }
  itwalk(env);
}

void TD_func::itwalk(Env &env)
{
  f->twalk(env);
}

void TD_class::itwalk(Env &env)
{ 
  type->twalk(env);
}

// ------------------- TemplateParameter ------------------
void TP_type::twalk(Env &env)
{
  if (defaultType) {
    defaultType->twalk(env);
  }
}


// -------------------- TemplateArgument ------------------
void TA_type::twalk(Env &env)
{
  type->twalk(env);
}
