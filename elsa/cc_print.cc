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
//CodeOutputStream global_code_out(cout);

// set this environment variable to see the twalk_layer debugging
// output
TreeWalkOutputStream treeWalkOut(cout, getenv("TWALK_VERBOSE"));

// This is a dummy global so that this file will compile both in
// default mode and in qualifiers mode.
class dummy_type;               // This does nothing.
dummy_type *ql;
string toString(class dummy_type*) {return "";}

// **** class CodeOutputStream

CodeOutputStream::CodeOutputStream(ostream &out)
  : out(&out), sb(NULL), using_sb(false), depth(0), buffered_newlines(0)
{}

CodeOutputStream::CodeOutputStream(stringBuilder &sb)
  : out(NULL), sb(&sb), using_sb(true), depth(0), buffered_newlines(0)
{}

CodeOutputStream::~CodeOutputStream()
{
  if (buffered_newlines) {
    cout << "**************** ERROR.  "
         << "You called my destructor before making sure all the buffered newlines\n"
         << "were flushed (by, say, calling finish())\n";
  }
}

void CodeOutputStream::finish()
{
  // NOTE: it is probably an error if depth is ever > 0 at this point.
  //      printf("BUFFERED NEWLINES: %d\n", buffered_newlines);
  stringBuilder s;
  for(;buffered_newlines>1;buffered_newlines--) s << "\n";
  rawPrintAndIndent(indentMessage(depth,s));
  xassert(buffered_newlines == 1 || buffered_newlines == 0);
  if (buffered_newlines) {
    buffered_newlines--;
    rawPrintAndIndent(string("\n")); // don't indent after last one
  }
}

void CodeOutputStream::up()   {depth--;}
void CodeOutputStream::down() {depth++;}

void CodeOutputStream::flush()
{
  if (!using_sb) out->flush();
}

void CodeOutputStream::rawPrintAndIndent(string s)
{
  if (using_sb) *sb << s;
  else *out << s;
  flush();
}

CodeOutputStream & CodeOutputStream::operator << (char const *message)
{
  int len = strlen(message);
  if (len<1) return *this;
  string message1 = message;

  int pending_buffered_newlines = 0;
  if (message1[len-1] == '\n') {
    message1[len-1] = '\0';    // whack it
    pending_buffered_newlines++;
  }

  stringBuilder message2;
  if (buffered_newlines) {
    message2 << "\n";
    buffered_newlines--;
  }
  message2 << message1;
  buffered_newlines += pending_buffered_newlines;

  rawPrintAndIndent(indentMessage(depth, message2));
  return *this;
}

CodeOutputStream & CodeOutputStream::operator << (ostream& (*manipfunc)(ostream& outs))
{
  if (using_sb) {
    // sm: just assume it's "endl"; the only better thing I could
    // imagine doing is pointer comparisons with some other
    // well-known omanips, since we certainly can't execute it...
    if (buffered_newlines) {
      *sb << "\n";
      *sb << makeIndentation(depth);
    } else buffered_newlines++;
  }
  else {
    // dsw: just assume its endl
    //        *out << manipfunc;
    if (buffered_newlines) {
      *out << endl;
      *out << makeIndentation(depth);
    } else buffered_newlines++;
    out->flush();
  }
  return *this;
}

CodeOutputStream & CodeOutputStream::operator << (rostring message)
{
  return operator<< (message.c_str());
}

// provide access to the built string
stringBuilder const &CodeOutputStream::getString() const
{
  xassert(using_sb);
  return *sb;
}

string CodeOutputStream::makeIndentation(int n) {
  stringBuilder s;
  for (int i=0; i<n; ++i) s << "  ";
  return s;
}

string CodeOutputStream::indentMessage(int n, rostring message) {
  stringBuilder s;
  char const *m = message.c_str();
  int len = strlen(m);
  for(int i=0; i<len; ++i) {
    s << m[i];
    if (m[i] == '\n') s << makeIndentation(n);
  }
  return s;
}

// **** class PairDelim

PairDelim::PairDelim
  (CodeOutputStream &out, rostring message, rostring open, char const *close)
  : close(close), out(out)
{
  out << message;
  out << " ";
  out << open;
  if (strchr(toCStr(open), '{')) out.down();
}

PairDelim::PairDelim(CodeOutputStream &out, rostring message)
  : close(""), out(out)
{
  out << message;
  out << " ";
}

PairDelim::~PairDelim() {
  if (strchr(close, '}')) out.up();
  out << close;
}

// **** class TreeWalkOutputStream

TreeWalkOutputStream::TreeWalkOutputStream(ostream &out, bool on)
  : out(out), on(on), depth(0)
{}

void TreeWalkOutputStream::indent() {
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

void TreeWalkOutputStream::flush() {out.flush();}

TreeWalkOutputStream & TreeWalkOutputStream::operator << (char *message)
{
  if (on) {
    indent();
    out << message;
    //        fprintf(out, message);
    out.flush();
    //        fflush(out);
  }
  return *this;
}

TreeWalkOutputStream & TreeWalkOutputStream::operator << (ostream& (*manipfunc)(ostream& outs))
{
  if (on) out << manipfunc;
  return *this;
}

void TreeWalkOutputStream::down() {++depth;}
void TreeWalkOutputStream::up()   {--depth;}

// **** class TreeWalkDebug

TreeWalkDebug::TreeWalkDebug(char *message, TreeWalkOutputStream &out)
  : out(out)
{
  out << message << endl;
  out.flush();
  out.down();
}

TreeWalkDebug::~TreeWalkDebug()
{
  out.up();
}

// **** class TypePrinter

void TypePrinter::print(Type *type, stringBuilder &sb, char const *name)
{
  sb << type->toCString(name);
}

string TypePrinter::print(Type *type, char const *name)
{
  stringBuilder sb;
  print(type, sb, name);
  return sb;
}

// ****************

// function for printing declarations (without the final semicolon);
// handles a variety of declarations such as:
//   int x
//   int x()
//   C()                    // ctor inside class C
//   operator delete[]()
//   char operator()        // conversion operator to 'char'
string declaration_toString
  (PrintEnv &env,
                             
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
  TreeWalkDebug treeDebug("declaration_toString");
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
    env.typePrinter.print(type->asFunctionTypeC()->retType, s);
    s << " ()";
  }

  else if (finalName && 0==strcmp(finalName, "constructor-special")) {
    // extract the class name, which can be found by looking up
    // the name of the scope which contains the associated variable
    s << type->toCString(var->scope->curCompound->name);
  }

  else {
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
string var_toString(PrintEnv &env, Variable *var, PQName const * /*nullable*/ pqname)
{
  TreeWalkDebug treeDebug("var_toString");
  return declaration_toString(env, var->flags, var->type, pqname, var);
}


// this is a prototype for a function down near E_funCall::iprint
void printArgExprList(FakeList<ArgExpression> *list, PrintEnv &env);


// ------------------- TranslationUnit --------------------
void TranslationUnit::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TranslationUnit");
  FOREACH_ASTLIST_NC(TopForm, topForms, iter) {
    iter.data()->print(env);
  }
}

// --------------------- TopForm ---------------------
void TF_decl::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_decl");
  env.loc = loc;
  decl->print(env);
}

void TF_func::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_func");
  env.out << endl;
  env.loc = loc;
  f->print(env);
}

void TF_template::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_template");
  env.loc = loc;
  td->print(env);
}

void TF_explicitInst::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_explicitInst");
  env.loc = loc;
  env.out << "template ";
  d->print(env);
}

void TF_linkage::print(PrintEnv &env)
{         
  TreeWalkDebug treeDebug("TF_linkage");
  env.loc = loc;
  env.out << "extern " << linkageType;
  PairDelim pair(env.out, "", " {\n", "}\n");
  forms->print(env);
}

void TF_one_linkage::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_one_linkage");
  env.loc = loc;
  env.out << "extern " << linkageType << " ";
  form->print(env);
}

void TF_asm::print(PrintEnv &env)
{    
  TreeWalkDebug treeDebug("TF_asm");
  env.loc = loc;
  env.out << "asm(" << text << ");\n";
}

void TF_namespaceDefn::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_namespaceDefn");
  env.loc = loc;
  env.out << "namespace " << (name? name : "/*anon*/") << " {\n";
  FOREACH_ASTLIST_NC(TopForm, forms, iter) {
    iter.data()->print(env);
  }
  env.out << "} /""* namespace " << (name? name : "(anon)") << " */\n";
}

void TF_namespaceDecl::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TF_namespaceDecl");
  env.loc = loc;
  decl->print(env);
}


// --------------------- Function -----------------
void Function::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("Function");

  env.out <<
    declaration_toString(env, dflags, funcType,
                         nameAndParams->getDeclaratorId(),
                         nameAndParams->var);

  if (instButNotTchecked()) {
    // this is an unchecked instantiation
    env.out << "; // not instantiated\n";
    return;
  }

  if (inits) {
    env.out << ":";
    bool first_time = true;
    FAKELIST_FOREACH_NC(MemberInit, inits, iter) {
      if (first_time) first_time = false;
      else env.out << ",";
      // NOTE: eventually will be able to figure out if we are
      // initializing a base class or a member variable.  There will
      // be a field added to class MemberInit that will say.
      PairDelim pair(env.out, iter->name->toString(), "(", ")");
      printArgExprList(iter->args, env);
    }
  }

  if (handlers) env.out << "\ntry";

  if (body->stmts.isEmpty()) {
    // more concise
    env.out << " {}\n";
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
  TreeWalkDebug treeDebug("Declaration");
  if(spec->isTS_classSpec()) {
    spec->asTS_classSpec()->print(env);
    env.out << ";\n";
  }
  else if(spec->isTS_enumSpec()) {
    spec->asTS_enumSpec()->print(env);
    env.out << ";\n";
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
      env.out << toString(extras) << " ";
    }

    // TODO: this will not work if there is more than one declarator ...

    iter->print(env);
    env.out << ";" << endl;
  }
}

//  -------------------- ASTTypeId -------------------
void ASTTypeId::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("ASTTypeId");
  env.out << env.typePrinter.print(getType());
  if (decl->getDeclaratorId()) {
    env.out << " ";
    decl->getDeclaratorId()->print(env);
  }
  
  if (decl->init) {
    env.out << " = ";
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
        env.out << ", ";
      }
      args->print(env);
    }

    args = args->next;
  }
}

void PQ_qualifier::print(PrintEnv &env)
{
  if (templateUsed()) {
    env.out << "template ";
  }

  env.out << qualifier;
  if (templArgs/*isNotEmpty*/) {
    env.out << "<";
    printTemplateArgumentList(env, templArgs);
    env.out << ">";
  }
  env.out << "::";
  rest->print(env);
}

void PQ_name::print(PrintEnv &env)
{
  env.out << name;
}

void PQ_operator::print(PrintEnv &env)
{
  env.out << fakeName;
}

void PQ_template::print(PrintEnv &env)
{
  if (templateUsed()) {
    env.out << "template ";
  }

  env.out << name << "<";
  printTemplateArgumentList(env, templArgs);
  env.out << ">";
}


// --------------------- TypeSpecifier --------------
void TS_name::print(PrintEnv &env)
{
  xassert(0);                   // I'll bet this is never called.
//    TreeWalkDebug treeDebug("TS_name");
//    env.out << toString(ql);          // see string toString(class dummy_type*) above
//    name->print(env);
}

void TS_simple::print(PrintEnv &env)
{
  xassert(0);                   // I'll bet this is never called.
//    TreeWalkDebug treeDebug("TS_simple");
//    env.out << toString(ql);          // see string toString(class dummy_type*) above
}

void TS_elaborated::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TS_elaborated");
  env.loc = loc;
  env.out << toString(ql);          // see string toString(class dummy_type*) above
  env.out << toString(keyword) << " ";
  name->print(env);
}

void TS_classSpec::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TS_classSpec");
  env.out << toString(ql);          // see string toString(class dummy_type*) above
  env.out << toString(cv);
  env.out << toString(keyword) << " ";
  if (name) env.out << name->toString();
  bool first_time = true;
  FAKELIST_FOREACH_NC(BaseClassSpec, bases, iter) {
    if (first_time) {
      env.out << " : ";
      first_time = false;
    }
    else env.out << ", ";
    iter->print(env);
  }
  PairDelim pair(env.out, "", "{\n", "}");
  FOREACH_ASTLIST_NC(Member, members->list, iter2) {
    iter2.data()->print(env);
  }
}

void TS_enumSpec::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TS_classSpec");
  env.out << toString(ql);          // see string toString(class dummy_type*) above
  env.out << toString(cv);
  env.out << "enum ";
  if (name) env.out << name;
  PairDelim pair(env.out, "", "{\n", "}");
  FAKELIST_FOREACH_NC(Enumerator, elts, iter) {
    iter->print(env);
    env.out << "\n";
  }
}

// BaseClass
void BaseClassSpec::print(PrintEnv &env) {
  TreeWalkDebug treeDebug("BaseClassSpec");
  if (isVirtual) env.out << "virtual ";
  if (access!=AK_UNSPECIFIED) env.out << toString(access) << " ";
  env.out << name->toString();
}

// MemberList

// ---------------------- Member ----------------------
void MR_decl::print(PrintEnv &env)
{                   
  TreeWalkDebug treeDebug("MR_decl");
  d->print(env);
}

void MR_func::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("MR_func");
  f->print(env);
}

void MR_access::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("MR_access");
  env.out << toString(k) << ":\n";
}

void MR_usingDecl::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("MR_usingDecl");
  decl->print(env);
}

void MR_template::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("MR_template");
  d->print(env);
}


// -------------------- Enumerator --------------------
void Enumerator::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("Enumerator");
  env.out << name;
  if (expr) {
    env.out << "=";
    expr->print(env);
  }
  env.out << ", ";
}

// -------------------- Declarator --------------------
void Declarator::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("Declarator");

  env.out << var_toString(env, var, decl->getDeclaratorId());
  D_bitfield *b = dynamic_cast<D_bitfield*>(decl);
  if (b) {
    env.out << ":";
    b->bits->print(env);
  }
  if (init) {
    IN_ctor *ctor = dynamic_cast<IN_ctor*>(init);
    if (ctor) {
      // sm: don't print "()" as an IN_ctor initializer (cppstd 8.5 para 8)
      if (ctor->args->isEmpty()) {
        env.out << " /*default-ctor-init*/";
      }
      else {
        // dsw:Constructor arguments.
        PairDelim pair(env.out, "", "(", ")");
        ctor->print(env);       // NOTE: You can NOT factor this line out of the if!
      }
    } else {
      env.out << "=";
      init->print(env);         // Don't pull this out!
    }
  }
}

// ------------------- ExceptionSpec --------------------
void ExceptionSpec::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("ExceptionSpec");
  env.out << "throw"; // Scott says this is right.
  FAKELIST_FOREACH_NC(ASTTypeId, types, iter) {
    iter->print(env);
  }
}

// ---------------------- Statement ---------------------
void Statement::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("Statement");
  env.loc = loc;
  iprint(env);
  //    env.out << ";\n";
}

// no-op
void S_skip::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_skip::iprint");
  env.out << ";\n";
}

void S_label::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_label::iprint");
  env.out << name << ":";
  s->print(env);
}

void S_case::iprint(PrintEnv &env)
{                    
  TreeWalkDebug treeDebug("S_case::iprint");
  env.out << "case";
  expr->print(env);
  env.out << ":";
  s->print(env);
}

void S_default::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_default::iprint");
  env.out << "default:";
  s->print(env);
}

void S_expr::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_expr::iprint");
  expr->print(env);
  env.out << ";\n";
}

void S_compound::iprint(PrintEnv &env)
{ 
  TreeWalkDebug treeDebug("S_compound::iprint");
  PairDelim pair(env.out, "", "{\n", "}\n");
  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    iter.data()->print(env);
  }
}

void S_if::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_if::iprint");
  {
    PairDelim pair(env.out, "if", "(", ")");
    cond->print(env);
  }
  thenBranch->print(env);
  env.out << "else ";
  elseBranch->print(env);
}

void S_switch::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_switch::iprint");
  {
    PairDelim pair(env.out, "switch", "(", ")");
    cond->print(env);
  }
  branches->print(env);
}

void S_while::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_while::iprint");
  {
    PairDelim pair(env.out, "while", "(", ")");
    cond->print(env);
  }
  body->print(env);
}

void S_doWhile::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_doWhile::iprint");
  {
    PairDelim pair(env.out, "do");
    body->print(env);
  }
  {
    PairDelim pair(env.out, "while", "(", ")");
    expr->print(env);
  }
  env.out << ";\n";
}

void S_for::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_for::iprint");
  {
    PairDelim pair(env.out, "for", "(", ")");
    init->print(env);
    // this one not needed as the declaration provides one
    //          env.out << ";";
    cond->print(env);
    env.out << ";";
    after->print(env);
  }
  body->print(env);
}

void S_break::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_break::iprint");
  env.out << "break";
  env.out << ";\n";
}

void S_continue::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_continue::iprint");
  env.out << "continue";
  env.out << ";\n";
}

void S_return::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_return::iprint");
  env.out << "return";
  if (expr) expr->print(env);
  env.out << ";\n";
}

void S_goto::iprint(PrintEnv &env)
{
  // dsw: When doing a control-flow pass, keep a current function so
  // we know where to look for the label.
  TreeWalkDebug treeDebug("S_goto::iprint");
  env.out << "goto ";
  env.out << target;
  env.out << ";\n";
}

void S_decl::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_decl::iprint");
  decl->print(env);
  //      env.out << ";\n";
}

void S_try::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_try::iprint");
  env.out << "try";
  body->print(env);
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->print(env);
  }
}

void S_asm::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_asm::iprint");
  env.out << "asm(" << text << ");\n";
}

void S_namespaceDecl::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("S_namespaceDecl::iprint");
  decl->print(env);
}


// ------------------- Condition --------------------
// CN = ConditioN

// this situation: if (gronk()) {...
void CN_expr::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("CN_expr");
  expr->print(env);
}

// this situation: if (bool b=gronk()) {...
void CN_decl::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("CN_decl");
  typeId->print(env);
}

// ------------------- Handler ----------------------
// catch clause
void Handler::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("Handler");
  {
    PairDelim pair(env.out, "catch", "(", ")");
    if (isEllipsis()) {
      env.out << "...";
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
  TreeWalkDebug treeDebug("FullExpression");
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
  TreeWalkDebug treeDebug("Expression");
  PairDelim pair(env.out, "", "(", ")"); // this will put parens around every expression
  iprint(env);
}

string Expression::exprToString() const
{              
  TreeWalkDebug treeDebug("Expression::exprToString");
  stringBuilder sb;
  CodeOutputStream codeOut(sb);
  TypePrinter typePrinter;
  PrintEnv env(codeOut, typePrinter);
  
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
  TreeWalkDebug treeDebug("E_boolLit::iprint");
  env.out << b;
}

void E_intLit::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_intLit::iprint");
  // FIX: do this correctly from the internal representation
  // fails to print the trailing U for an unsigned int.
//    env.out << i;
  env.out << text;
}

void E_floatLit::iprint(PrintEnv &env)
{                                
  TreeWalkDebug treeDebug("E_floatLit::iprint");
  // FIX: do this correctly from the internal representation
  // this fails to print ".0" for a float/double that happens to lie
  // on an integer boundary
//    env.out << d;
  // doing it this way also preserves the trailing "f" for float
  // literals
  env.out << text;
}

void E_stringLit::iprint(PrintEnv &env)
{                                                                     
  TreeWalkDebug treeDebug("E_stringLit::iprint");
  
  E_stringLit *p = this;
  while (p) {
    env.out << p->text;
    p = p->continuation;
    if (p) {
      env.out << " ";
    }
  }
}

void E_charLit::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_charLit::iprint");
  env.out << text;
}

void E_this::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_this::iprint");
  env.out << "this";
}

// modified from STemplateArgument::toString()
void printSTemplateArgument(PrintEnv &env, STemplateArgument const *sta)
{
  switch (sta->kind) {
    default: xfailure("bad kind");
    case STemplateArgument::STA_NONE:
      env.out << string("STA_NONE");
      break;
    case STemplateArgument::STA_TYPE:
      env.out << env.typePrinter.print(sta->value.t); // assume 'type' if no comment
      break;
    case STemplateArgument::STA_INT:
      env.out << stringc << "/*int*/ " << sta->value.i;
      break;
    case STemplateArgument::STA_REFERENCE:
      env.out << stringc << "/*ref*/ " << sta->value.v->name;
      break;
    case STemplateArgument::STA_POINTER:
      env.out << stringc << "/*ptr*/ &" << sta->value.v->name;
      break;
    case STemplateArgument::STA_MEMBER:
      env.out << stringc
              << "/*member*/ &" << sta->value.v->scope->curCompound->name
              << "::" << sta->value.v->name;
      break;
    case STemplateArgument::STA_DEPEXPR:
      sta->getDepExpr()->print(env);
      break;
    case STemplateArgument::STA_TEMPLATE:
      env.out << string("template (?)");
      break;
  }
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
  env.out << "<";
  int ct=0;
  for (; !iter.isDone(); iter.adv()) {
    if (ct++ > 0) {
      env.out << ", ";
    }
    printSTemplateArgument(env, iter.data());
  }
  env.out << ">";
}

void E_variable::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_variable::iprint");
  if (var && var->isBoundTemplateParam()) {
    // this is a bound template variable, so print its value instead
    // of printing its name
    xassert(var->value);
    var->value->print(env);
  }
  else {
    env.out << name->qualifierString() << name->getName();
    printTemplateArgs(env, var);
  }
}

void printArgExprList(FakeList<ArgExpression> *list, PrintEnv &env)
{
  TreeWalkDebug treeDebug("printArgExprList");
  bool first_time = true;
  FAKELIST_FOREACH_NC(ArgExpression, list, iter) {
    if (first_time) first_time = false;
    else env.out << ", ";
    iter->expr->print(env);
  }
}

void E_funCall::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_funCall::iprint");
  func->print(env);
  PairDelim pair(env.out, "", "(", ")");
  printArgExprList(args, env);
}

void E_constructor::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_constructor::iprint");
  env.out << env.typePrinter.print(type);
  PairDelim pair(env.out, "", "(", ")");
  printArgExprList(args, env);
}

void E_fieldAcc::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_fieldAcc::iprint");
  obj->print(env);
  env.out << ".";
  if (field &&
      !field->type->isDependent()) {
    env.out << field->name;
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
    env.out << fieldName->toString();
  }
}

void E_arrow::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_arrow::iprint");
  
  // E_arrow shouldn't normally be present in code that is to be
  // prettyprinted, so it doesn't much matter what this does.
  obj->print(env);
  env.out << "->";
  fieldName->print(env);
}

void E_sizeof::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_sizeof::iprint");
  // NOTE parens are not necessary because it's an expression, not a
  // type.
  env.out << "sizeof";
  expr->print(env);           // putting parens in here so we are safe wrt precedence
}

// dsw: unary expression?
void E_unary::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_unary::iprint");
  env.out << toString(op);
  expr->print(env);
}

void E_effect::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_effect::iprint");
  if (!isPostfix(op)) env.out << toString(op);
  expr->print(env);
  if (isPostfix(op)) env.out << toString(op);
}

// dsw: binary operator.
void E_binary::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_binary::iprint");
  e1->print(env);
  if (op != BIN_BRACKETS) {
    env.out << toString(op);
    e2->print(env);
  }
  else {
    env.out << "[";
    e2->print(env);
    env.out << "]";
  }
}

void E_addrOf::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_addrOf::iprint");
  env.out << "&";
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
  TreeWalkDebug treeDebug("E_deref::iprint");
  env.out << "*";
  ptr->print(env);
}

// C-style cast
void E_cast::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_cast::iprint");
  {
    PairDelim pair(env.out, "", "(", ")");
    ctype->print(env);
  }
  expr->print(env);
}

// ? : syntax
void E_cond::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_cond::iprint");
  cond->print(env);
  env.out << "?";
  // In gcc it is legal to omit the 'then' part;
  // http://gcc.gnu.org/onlinedocs/gcc-3.4.1/gcc/Conditionals.html#Conditionals
  if (th) {
    th->print(env);
  }
  env.out << ":";
  el->print(env);
}

void E_sizeofType::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_sizeofType::iprint");
  PairDelim pair(env.out, "sizeof", "(", ")"); // NOTE yes, you do want the parens because argument is a type.
  atype->print(env);
}

void E_assign::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_assign::iprint");
  target->print(env);
  if (op!=BIN_ASSIGN) env.out << toString(op);
  env.out << "=";
  src->print(env);
}

void E_new::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_new::iprint");
  if (colonColon) env.out << "::";
  env.out << "new ";
  if (placementArgs) {
    PairDelim pair(env.out, "", "(", ")");
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
    env.out << t->leftString() << " [";
    arraySize->print(env);
    env.out << "]" << t->rightString();
  }

  if (ctorArgs) {
    PairDelim pair(env.out, "", "(", ")");
    printArgExprList(ctorArgs->list, env);
  }
}

void E_delete::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_delete::iprint");
  if (colonColon) env.out << "::";
  env.out << "delete";
  if (array) env.out << "[]";
  // dsw: this can be null because elaboration can remove syntax when
  // it is replaced with other syntax
  if (expr) {
    expr->print(env);
  }
}

void E_throw::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_throw::iprint");
  env.out << "throw";
  if (expr) expr->print(env);
}

// C++-style cast
void E_keywordCast::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_keywordCast::iprint");
  env.out << toString(key);
  {
    PairDelim pair(env.out, "", "<", ">");
    ctype->print(env);
  }
  PairDelim pair(env.out, "", "(", ")");
  expr->print(env);
}

// RTTI: typeid(expression)
void E_typeidExpr::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_typeidExpr::iprint");
  PairDelim pair(env.out, "typeid", "(", ")");
  expr->print(env);
}

// RTTI: typeid(type)
void E_typeidType::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_typeidType::iprint");
  PairDelim pair(env.out, "typeid", "(", ")");
  ttype->print(env);
}

void E_grouping::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("E_grouping::iprint");
  
  // sm: given that E_grouping is now in the tree, and prints its
  // parentheses, perhaps we could eliminate some of the
  // paren-printing above?
  //PairDelim pair(env.out, "", "(", ")");
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
  TreeWalkDebug treeDebug("IN_expr");
  e->print(env);
}

// int x[] = {1, 2, 3};
//           ^^^^^^^^^ only
void IN_compound::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("IN_compound");
  PairDelim pair(env.out, "", "{\n", "\n}");
  bool first_time = true;
  FOREACH_ASTLIST_NC(Initializer, inits, iter) {
    if (first_time) first_time = false;
    else env.out << ",\n";
    iter.data()->print(env);
  }
}

void IN_ctor::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("IN_ctor");
  printArgExprList(args, env);
}

// InitLabel

// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::print(PrintEnv &env)
{ 
  TreeWalkDebug treeDebug("TemplateDeclaration");

  env.out << "template <";
  int ct=0;
  FAKELIST_FOREACH_NC(TemplateParameter, params, iter) {
    if (ct++ > 0) {
      env.out << ", ";
    }
    iter->print(env);
  }
  env.out << ">\n";

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
      env.out << inst->toQualifiedString() << ";    // decl but not defn\n";
    }
  }
}

void TD_func::iprint(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TD_func");
  f->print(env);

  // print instantiations
  Variable *var = f->nameAndParams->var;
  if (var->isTemplate() &&      // for complete specializations, don't print
      !var->templateInfo()->isPartialInstantiation()) {     // nor partial inst
    env.out << "#if 0    // instantiations of ";
    env.typePrinter.print(var->type, (var->name? var->name : "/*anon*/"));
    env.out << "\n";
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

    env.out << "#endif   // instantiations of " << var->name << "\n\n";
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
      env.out << "#if 0    // instantiations of " << ct->name << "\n";

      SFOREACH_OBJLIST(Variable, ti->instantiations, iter) {
        Variable const *instV = iter.data();

        env.out << "// ";
        env.out << env.typePrinter.print(instV->type);
        CompoundType *instCT = instV->type->asCompoundType();
        if (instCT->syntax) {
          env.out << "\n";
          instCT->syntax->print(env);
          env.out << ";\n";
        }
        else {
          env.out << ";     // body not instantiated\n";
        }
      }
      env.out << "#endif   // instantiations of " << ct->name << "\n\n";
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
  TreeWalkDebug treeDebug("TP_type");
  env.out << "class " << name;
                          
  if (defaultType) {
    env.out << " = ";
    defaultType->print(env);
  }
}

void TP_nontype::print(PrintEnv &env)
{
  TreeWalkDebug treeDebug("TP_nontype");
  param->print(env);
}


// -------------------- TemplateArgument ------------------
void TA_type::print(PrintEnv &env)
{
  // dig down to prevent printing "/*anon*/" since template
  // type arguments are always anonymous so it's just clutter
  env.out << type->decl->var->type->toCString();
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
  env.out << "namespace " << alias << " = ";
  original->print(env);
  env.out << ";\n";
}

void ND_usingDecl::print(PrintEnv &env)
{
  env.out << "using ";
  name->print(env);
  env.out << ";\n";
}

void ND_usingDir::print(PrintEnv &env)
{
  env.out << "using namespace ";
  name->print(env);
  env.out << ";\n";
}


// EOF
