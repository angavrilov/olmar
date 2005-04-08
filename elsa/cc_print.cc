// cc_print.cc            see license.txt for copyright and terms of use
// code for cc_print.h

// Adapted from cc_tcheck.cc by Daniel Wilkerson dsw@cs.berkeley.edu

// This is a tree walk that prints out a functionally equivalent C++
// program to the original.

#include "cc_print.h"           // this module
#include "trace.h"              // trace
#include "strutil.h"            // string utilities

#include <stdlib.h>             // getenv

// forward in this file
void printSTemplateArgument(PrintEnv &env, CodeOutStream &out, STemplateArgument const *sta);

// where code output goes
// sm: I've replaced uses of this with 'env' instead
//CodeOutStream global_code_out(cout);

// set this environment variable to see the twalk_layer debugging
// output
OStreamOutStream treeWalkOut0(cout);
TreeWalkOutStream treeWalkOut(treeWalkOut0, getenv("TWALK_VERBOSE"));

// This is a dummy global so that this file will compile both in
// default mode and in qualifiers mode.
class dummy_type;               // This does nothing.
dummy_type *ql;
string toString(class dummy_type*) {return "";}

// **** class CodeOutStream

CodeOutStream::~CodeOutStream()
{
  if (buffered_newlines) {
    cout << "**************** ERROR.  "
         << "You called my destructor before making sure all the buffered newlines\n"
         << "were flushed (by, say, calling finish())\n";
  }
}

void CodeOutStream::printIndentation(int n) {
  for (int i=0; i<n; ++i) {
    out << "  ";
  }
}

void CodeOutStream::printWhileInsertingIndentation(int n, rostring message) {
  int len = message.length();
  for(int i=0; i<len; ++i) {
    char c = message[i];
    out << c;
    if (c == '\n') printIndentation(n);
  }
}

void CodeOutStream::finish()
{
  // NOTE: it is probably an error if depth is ever > 0 at this point.
  //      printf("BUFFERED NEWLINES: %d\n", buffered_newlines);
  stringBuilder s;
  for(;buffered_newlines>1;buffered_newlines--) s << "\n";
  printWhileInsertingIndentation(depth,s);
  xassert(buffered_newlines == 1 || buffered_newlines == 0);
  if (buffered_newlines) {
    buffered_newlines--;
    out << "\n";                // don't indent after last one
  }
  flush();
}

CodeOutStream & CodeOutStream::operator << (ostream& (*manipfunc)(ostream& outs))
{
  // sm: just assume it's "endl"; the only better thing I could
  // imagine doing is pointer comparisons with some other well-known
  // omanips, since we certainly can't execute it...
  if (buffered_newlines) {
    out << endl;
    printIndentation(depth);
  } else buffered_newlines++;
  out.flush();
  return *this;
}

CodeOutStream & CodeOutStream::operator << (char const *message)
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

  printWhileInsertingIndentation(depth, message2);
  return *this;
}

// **** class PairDelim

PairDelim::PairDelim(CodeOutStream &out, rostring message, rostring open, char const *close0)
    : close(close0), out(out)
{
  out << message;
  out << " ";
  out << open;
  if (strchr(toCStr(open), '{')) out.down();
}

PairDelim::PairDelim(CodeOutStream &out, rostring message)
  : close(""), out(out)
{
  out << message;
  out << " ";
}

PairDelim::~PairDelim() {
  if (strchr(close, '}')) out.up();
  out << close;
}

// **** class TreeWalkOutStream

void TreeWalkOutStream::indent() {
  out << endl;
  out.flush();
  for(int i=0; i<depth; ++i) out << " ";
  out.flush();
  out << ":::::";
  out.flush();
}

TreeWalkOutStream & TreeWalkOutStream::operator << (ostream& (*manipfunc)(ostream& outs))
{
  if (on) out << manipfunc;
  return *this;
}

// **** class TreeWalkDebug

TreeWalkDebug::TreeWalkDebug(char *message, TreeWalkOutStream &out)
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

// **** class TypePrinterC

void TypePrinterC::print(OutStream &out, Type const *type, char const *name)
{
  // temporarily suspend the Type::toCString, Variable::toCString(),
  // etc. methods
  Restorer<bool> res0(global_mayUseTypeAndVarToCString, false);
  out << print(type, name);
  // old way
//    out << type->toCString(name);
}


// **************** AtomicType

string TypePrinterC::print(AtomicType const *atomic)
{
  // roll our own virtual dispatch
  switch(atomic->getTag()) {
  default: xfailure("bad tag");
  case AtomicType::T_SIMPLE:              return print(atomic->asSimpleTypeC());
  case AtomicType::T_COMPOUND:            return print(atomic->asCompoundTypeC());
  case AtomicType::T_ENUM:                return print(atomic->asEnumTypeC());
  case AtomicType::T_TYPEVAR:             return print(atomic->asTypeVariableC());
  case AtomicType::T_PSEUDOINSTANTIATION: return print(atomic->asPseudoInstantiationC());
  case AtomicType::T_DEPENDENTQTYPE:      return print(atomic->asDependentQTypeC());
  }
}

string TypePrinterC::print(SimpleType const *simpleType)
{
  return simpleTypeName(simpleType->type);
}

string TypePrinterC::print(CompoundType const *cpdType)
{
  stringBuilder sb;

  TemplateInfo *tinfo = cpdType->templateInfo();
  bool hasParams = tinfo && tinfo->params.isNotEmpty();
  if (hasParams) {
    sb << tinfo->paramsToCString() << " ";
  }

  if (!tinfo || hasParams) {   
    // only say 'class' if this is like a class definition, or
    // if we're not a template, since template instantiations
    // usually don't include the keyword 'class' (this isn't perfect..
    // I think I need more context)
    sb << CompoundType::keywordName(cpdType->keyword) << " ";
  }

  sb << (cpdType->instName? cpdType->instName : "/*anonymous*/");

  // template arguments are now in the name
  // 4/22/04: they were removed from the name a long time ago;
  //          but I'm just now uncommenting this code
  // 8/03/04: restored the original purpose of 'instName', so
  //          once again that is name+args, so this code is not needed
  //if (tinfo && tinfo->arguments.isNotEmpty()) {
  //  sb << sargsToString(tinfo->arguments);
  //}

  return sb;
}

string TypePrinterC::print(EnumType const *enumType)
{
  return stringc << "enum " << (enumType->name? enumType->name : "/*anonymous*/");
}

string TypePrinterC::print(TypeVariable const *typeVar)
{
  // use the "typename" syntax instead of "class", to distinguish
  // this from an ordinary class, and because it's syntax which
  // more properly suggests the ability to take on *any* type,
  // not just those of classes
  //
  // but, the 'typename' syntax can only be used in some specialized
  // circumstances.. so I'll suppress it in the general case and add
  // it explicitly when printing the few constructs that allow it
  //
  // 8/09/04: sm: truncated down to just the name, since the extra
  // clutter was annoying and not that helpful
  return stringc //<< "/""*typevar"
//                   << "typedefVar->serialNumber:"
//                   << (typedefVar ? typedefVar->serialNumber : -1)
                 //<< "*/"
                 << typeVar->name;
}

string TypePrinterC::print(PseudoInstantiation const *pseudoInst)
{
  stringBuilder sb0;
  StringBuilderOutStream out0(sb0);
  CodeOutStream codeOut(out0);
  PrintEnv env(*this);          // Yuck!
  // FIX: what about the env.loc?

  codeOut << pseudoInst->name;

  // NOTE: This was inlined from sargsToString; it would read as
  // follows:
//    codeOut << sargsToString(pseudoInst->args);
  codeOut << "<";
  int ct=0;
  FOREACH_OBJLIST(STemplateArgument, pseudoInst->args, iter) {
    if (ct++ > 0) {
      codeOut << ", ";
    }
    printSTemplateArgument(env, codeOut, iter.data());
  }
  codeOut << ">";

  codeOut.finish();
  return sb0;
}

string TypePrinterC::print(DependentQType const *depType)
{
  stringBuilder sb0;
  StringBuilderOutStream out0(sb0);
  CodeOutStream codeOut(out0);
  PrintEnv env(*this);          // Yuck!
  // FIX: what about the env.loc?

  codeOut << print(depType->first) << "::";
  depType->rest->print(env, codeOut);

  codeOut.finish();
  return sb0;
}


// **************** [Compound]Type

string TypePrinterC::print(Type const *type)
{
  if (type->isCVAtomicType()) {
    // special case a single atomic type, so as to avoid
    // printing an extra space
    CVAtomicType const *cvatomic = type->asCVAtomicTypeC();
    return stringc
      << print(cvatomic->atomic)
      << cvToString(cvatomic->cv);
  }
  else {
    return stringc
      << printLeft(type)
      << printRight(type);
  }
}

string TypePrinterC::print(Type const *type, char const *name)
{
  // print the inner parentheses if the name is omitted
  bool innerParen = (name && name[0])? false : true;

  #if 0    // wrong
  // except, if this type is a pointer, then omit the parens anyway;
  // we only need parens when the type is a function or array and
  // the name is missing
  if (isPointerType()) {
    innerParen = false;
  }
  #endif // 0

  stringBuilder s;
  s << printLeft(type, innerParen);
  s << (name? name : "/*anon*/");
  s << printRight(type, innerParen);
  return s;
}

string TypePrinterC::printRight(Type const *type, bool innerParen)
{
  // roll our own virtual dispatch
  switch(type->getTag()) {
  default: xfailure("illegal tag");
  case Type::T_ATOMIC:          return printRight(type->asCVAtomicTypeC(), innerParen);
  case Type::T_POINTER:         return printRight(type->asPointerTypeC(), innerParen);
  case Type::T_REFERENCE:       return printRight(type->asReferenceTypeC(), innerParen);
  case Type::T_FUNCTION:        return printRight(type->asFunctionTypeC(), innerParen);
  case Type::T_ARRAY:           return printRight(type->asArrayTypeC(), innerParen);
  case Type::T_POINTERTOMEMBER: return printRight(type->asPointerToMemberTypeC(), innerParen);
  }
}

string TypePrinterC::printLeft(Type const *type, bool innerParen)
{
  // roll our own virtual dispatch
  switch(type->getTag()) {
  default: xfailure("illegal tag");
  case Type::T_ATOMIC:          return printLeft(type->asCVAtomicTypeC(), innerParen);
  case Type::T_POINTER:         return printLeft(type->asPointerTypeC(), innerParen);
  case Type::T_REFERENCE:       return printLeft(type->asReferenceTypeC(), innerParen);
  case Type::T_FUNCTION:        return printLeft(type->asFunctionTypeC(), innerParen);
  case Type::T_ARRAY:           return printLeft(type->asArrayTypeC(), innerParen);
  case Type::T_POINTERTOMEMBER: return printLeft(type->asPointerToMemberTypeC(), innerParen);
  }
}

string TypePrinterC::printLeft(CVAtomicType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  s << print(type->atomic);
  s << cvToString(type->cv);

  // this is the only mandatory space in the entire syntax
  // for declarations; it separates the type specifier from
  // the declarator(s)
  s << " ";

  return s;
}

string TypePrinterC::printRight(CVAtomicType const *type, bool /*innerParen*/)
{
  return "";
}

string TypePrinterC::printLeft(PointerType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  s << printLeft(type->atType, false /*innerParen*/);
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << "(";
  }
  s << "*";
  if (type->cv) {
    // 1/03/03: added this space so "Foo * const arf" prints right (t0012.cc)
    s << cvToString(type->cv) << " ";
  }
  return s;
}

string TypePrinterC::printRight(PointerType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << ")";
  }
  s << printRight(type->atType, false /*innerParen*/);
  return s;
}

string TypePrinterC::printLeft(ReferenceType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  s << printLeft(type->atType, false /*innerParen*/);
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << "(";
  }
  s << "&";
  return s;
}

string TypePrinterC::printRight(ReferenceType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << ")";
  }
  s << printRight(type->atType, false /*innerParen*/);
  return s;
}

string TypePrinterC::printLeft(FunctionType const *type, bool innerParen)
{
  stringBuilder sb;

  // FIX: FUNC TEMPLATE LOSS
  // template parameters
//    if (templateInfo) {
//      sb << templateInfo->paramsToCString() << " ";
//    }

  // return type and start of enclosing type's description
  if (type->flags & (/*FF_CONVERSION |*/ FF_CTOR | FF_DTOR)) {
    // don't print the return type, it's implicit

    // 7/18/03: changed so we print ret type for FF_CONVERSION,
    // since otherwise I can't tell what it converts to!
  }
  else {
    sb << printLeft(type->retType);
  }

  // NOTE: we do *not* propagate 'innerParen'!
  if (innerParen) {
    sb << "(";
  }

  return sb;
}

string TypePrinterC::printRight(FunctionType const *type, bool innerParen)
{
  // I split this into two pieces because the Cqual++ concrete
  // syntax puts $tainted into the middle of my rightString,
  // since it's following the placement of 'const' and 'volatile'
  return stringc
    << printRightUpToQualifiers(type, innerParen)
    << printRightQualifiers(type, type->getReceiverCV())
    << printRightAfterQualifiers(type);
}

string TypePrinterC::printRightUpToQualifiers(FunctionType const *type, bool innerParen)
{
  // finish enclosing type
  stringBuilder sb;
  if (innerParen) {
    sb << ")";
  }

  // arguments
  sb << "(";
  int ct=0;
  SFOREACH_OBJLIST(Variable, type->params, iter) {
    ct++;
    if (type->isMethod() && ct==1) {
      // don't actually print the first parameter;
      // the 'm' stands for nonstatic member function
      sb << "/""*m: " << print(iter.data()->type) << " *""/ ";
      continue;
    }
    if (ct >= 3 || (!type->isMethod() && ct>=2)) {
      sb << ", ";
    }
    sb << printAsParameter(iter.data());
  }

  if (type->acceptsVarargs()) {
    if (ct++ > 0) {
      sb << ", ";
    }
    sb << "...";
  }

  sb << ")";

  return sb;
}

string TypePrinterC::printRightQualifiers(FunctionType const *type, CVFlags cv)
{
  if (cv) {
    return stringc << " " << ::toString(cv);
  }
  else {
    return "";
  }
}

string TypePrinterC::printRightAfterQualifiers(FunctionType const *type)
{
  stringBuilder sb;

  // exception specs
  if (type->exnSpec) {
    sb << " throw(";
    int ct=0;
    SFOREACH_OBJLIST(Type, type->exnSpec->types, iter) {
      if (ct++ > 0) {
        sb << ", ";
      }
      sb << print(iter.data());
    }
    sb << ")";
  }

  // hook for verifier syntax
  printExtraRightmostSyntax(type, sb);

  // finish up the return type
  sb << printRight(type->retType);

  return sb;
}

void TypePrinterC::printExtraRightmostSyntax(FunctionType const *type, stringBuilder &)
{}

string TypePrinterC::printLeft(ArrayType const *type, bool /*innerParen*/)
{
  return printLeft(type->eltType);
}

string TypePrinterC::printRight(ArrayType const *type, bool /*innerParen*/)
{
  stringBuilder sb;

  if (type->hasSize()) {
    sb << "[" << type->size << "]";
  }
  else {
    sb << "[]";
  }

  sb << printRight(type->eltType);

  return sb;
}

string TypePrinterC::printLeft(PointerToMemberType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  s << printLeft(type->atType, false /*innerParen*/);
  s << " ";
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << "(";
  }
  s << type->inClassNAT->name << "::*";
  s << cvToString(type->cv);
  return s;
}

string TypePrinterC::printRight(PointerToMemberType const *type, bool /*innerParen*/)
{
  stringBuilder s;
  if (type->atType->isFunctionType() ||
      type->atType->isArrayType()) {
    s << ")";
  }
  s << printRight(type->atType, false /*innerParen*/);
  return s;
}

string TypePrinterC::printAsParameter(Variable const *var)
{
  stringBuilder sb;
  if (var->type->isTypeVariable()) {
    // type variable's name, then the parameter's name (if any)
    sb << var->type->asTypeVariable()->name;
    if (var->name) {
      sb << " " << var->name;
    }
  }
  else {
    sb << print(var->type, var->name);
  }

  if (var->value) {
    sb << renderExpressionAsString(" = ", var->value);
  }
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
void printDeclaration
  (PrintEnv &env,
   CodeOutStream &out,
                             
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
  TreeWalkDebug treeDebug("printDeclaration");

  // mask off flags used for internal purposes, so all that's
  // left is the flags that were present in the source
  dflags = (DeclFlags)(dflags & DF_SOURCEFLAGS);
  if (dflags) {
    out << toString(dflags) << " ";
  }

  // the string name after all of the qualifiers; if this is
  // a special function, we're getting the encoded version
  StringRef finalName = pqname? pqname->getName() : NULL;

  if (finalName && 0==strcmp(finalName, "conversion-operator")) {
    // special syntax for conversion operators; first the keyword
    out << "operator ";

    // then the return type and the function designator
    env.typePrinter.print(out, type->asFunctionTypeC()->retType);
    out << " ()";
  }

  else if (finalName && 0==strcmp(finalName, "constructor-special")) {
    // extract the class name, which can be found by looking up
    // the name of the scope which contains the associated variable
    env.typePrinter.print(out, type, var->scope->curCompound->name);
  }

  else {
    if (finalName) {
      stringBuilder sb0;
      StringBuilderOutStream out0(sb0);
      CodeOutStream codeOut0(out0);
      if (pqname) {
        codeOut0 << pqname->qualifierString();
      }
      codeOut0 << finalName;
      if (type->isFunctionType() &&
          var->templateInfo() &&
          var->templateInfo()->isCompleteSpecOrInstantiation()) {
        // print the spec/inst args after the function name;
        //
        // NOTE: This was inlined from sargsToString; it used to read as follows:
//          codeOut0 << sargsToString(var->templateInfo()->arguments);
        codeOut0 << "<";
        int ct=0;
        FOREACH_OBJLIST_NC(STemplateArgument, var->templateInfo()->arguments, iter) {
          if (ct++ > 0) {
            codeOut0 << ", ";
          }
          printSTemplateArgument(env, codeOut0, iter.data());
        }
        codeOut0 << ">";
      }

      codeOut0 << var->namePrintSuffix();    // hook for verifier
      codeOut0.finish();

      env.typePrinter.print(out, type, sb0);
    } else {
      env.typePrinter.print(out, type, finalName);
    }
  }
}


// more specialized version of the previous function
void printVar(PrintEnv &env, CodeOutStream &out,
              Variable *var, PQName const * /*nullable*/ pqname)
{
  TreeWalkDebug treeDebug("printVar");
  printDeclaration(env, out, var->flags, var->type, pqname, var);
}


// this is a prototype for a function down near E_funCall::iprint
void printArgExprList(PrintEnv &env, CodeOutStream &out, FakeList<ArgExpression> *list);


// ------------------- TranslationUnit --------------------
void TranslationUnit::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TranslationUnit");
  FOREACH_ASTLIST_NC(TopForm, topForms, iter) {
    iter.data()->print(env, out);
  }
}

// --------------------- TopForm ---------------------
void TF_decl::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TF_decl");
  env.loc = loc;
  decl->print(env, out);
}

void TF_func::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TF_func");
  out << endl;
  env.loc = loc;
  f->print(env, out);
}

void TF_template::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TF_template");
  env.loc = loc;
  td->print(env, out);
}

void TF_explicitInst::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TF_explicitInst");
  env.loc = loc;
  out << "template ";
  d->print(env, out);
}

void TF_linkage::print(PrintEnv &env, CodeOutStream &out)
{         
  TreeWalkDebug treeDebug("TF_linkage");
  env.loc = loc;
  out << "extern " << linkageType;
  PairDelim pair(out, "", " {\n", "}\n");
  forms->print(env, out);
}

void TF_one_linkage::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TF_one_linkage");
  env.loc = loc;
  out << "extern " << linkageType << " ";
  form->print(env, out);
}

void TF_asm::print(PrintEnv &env, CodeOutStream &out)
{    
  TreeWalkDebug treeDebug("TF_asm");
  env.loc = loc;
  out << "asm(" << text << ");\n";
}

void TF_namespaceDefn::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TF_namespaceDefn");
  env.loc = loc;
  out << "namespace " << (name? name : "/*anon*/") << " {\n";
  FOREACH_ASTLIST_NC(TopForm, forms, iter) {
    iter.data()->print(env, out);
  }
  out << "} /""* namespace " << (name? name : "(anon)") << " */\n";
}

void TF_namespaceDecl::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TF_namespaceDecl");
  env.loc = loc;
  decl->print(env, out);
}


// --------------------- Function -----------------
void Function::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("Function");

  printDeclaration(env, out, dflags, funcType,
                   nameAndParams->getDeclaratorId(),
                   nameAndParams->var);

  if (instButNotTchecked()) {
    // this is an unchecked instantiation
    out << "; // not instantiated\n";
    return;
  }

  if (inits) {
    out << ":";
    bool first_time = true;
    FAKELIST_FOREACH_NC(MemberInit, inits, iter) {
      if (first_time) first_time = false;
      else out << ",";
      // NOTE: eventually will be able to figure out if we are
      // initializing a base class or a member variable.  There will
      // be a field added to class MemberInit that will say.
      PairDelim pair(out, iter->name->toString(), "(", ")");
      printArgExprList(env, out, iter->args);
    }
  }

  if (handlers) out << "\ntry";

  if (body->stmts.isEmpty()) {
    // more concise
    out << " {}\n";
  }
  else {
    body->print(env, out);
  }

  if (handlers) {
    FAKELIST_FOREACH_NC(Handler, handlers, iter) {
      iter->print(env, out);
    }
  }
}


// MemberInit

// -------------------- Declaration -------------------
void Declaration::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("Declaration");
  if(spec->isTS_classSpec()) {
    spec->asTS_classSpec()->print(env, out);
    out << ";\n";
  }
  else if(spec->isTS_enumSpec()) {
    spec->asTS_enumSpec()->print(env, out);
    out << ";\n";
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
      out << toString(extras) << " ";
    }

    // TODO: this will not work if there is more than one declarator ...

    iter->print(env, out);
    out << ";" << endl;
  }
}

//  -------------------- ASTTypeId -------------------
void ASTTypeId::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("ASTTypeId");
  env.typePrinter.print(out, getType());
  if (decl->getDeclaratorId()) {
    out << " ";
    decl->getDeclaratorId()->print(env, out);
  }
  
  if (decl->init) {
    out << " = ";
    decl->init->print(env, out);
  }
}

// ---------------------- PQName -------------------
void printTemplateArgumentList
  (PrintEnv &env, CodeOutStream &out, /*fakelist*/TemplateArgument *args)
{
  int ct=0;
  while (args) {
    if (!args->isTA_templateUsed()) {
      if (ct++ > 0) {
        out << ", ";
      }
      args->print(env, out);
    }

    args = args->next;
  }
}

void PQ_qualifier::print(PrintEnv &env, CodeOutStream &out)
{
  if (templateUsed()) {
    out << "template ";
  }

  out << qualifier;
  if (templArgs/*isNotEmpty*/) {
    out << "<";
    printTemplateArgumentList(env, out, templArgs);
    out << ">";
  }
  out << "::";
  rest->print(env, out);
}

void PQ_name::print(PrintEnv &env, CodeOutStream &out)
{
  out << name;
}

void PQ_operator::print(PrintEnv &env, CodeOutStream &out)
{
  out << fakeName;
}

void PQ_template::print(PrintEnv &env, CodeOutStream &out)
{
  if (templateUsed()) {
    out << "template ";
  }

  out << name << "<";
  printTemplateArgumentList(env, out, templArgs);
  out << ">";
}


// --------------------- TypeSpecifier --------------
void TS_name::print(PrintEnv &env, CodeOutStream &out)
{
  xassert(0);                   // I'll bet this is never called.
//    TreeWalkDebug treeDebug("TS_name");
//    out << toString(ql);          // see string toString(class dummy_type*) above
//    name->print(env, out);
}

void TS_simple::print(PrintEnv &env, CodeOutStream &out)
{
  xassert(0);                   // I'll bet this is never called.
//    TreeWalkDebug treeDebug("TS_simple");
//    out << toString(ql);          // see string toString(class dummy_type*) above
}

void TS_elaborated::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TS_elaborated");
  env.loc = loc;
  out << toString(ql);          // see string toString(class dummy_type*) above
  out << toString(keyword) << " ";
  name->print(env, out);
}

void TS_classSpec::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TS_classSpec");
  out << toString(ql);          // see string toString(class dummy_type*) above
  out << toString(cv);
  out << toString(keyword) << " ";
  if (name) out << name->toString();
  bool first_time = true;
  FAKELIST_FOREACH_NC(BaseClassSpec, bases, iter) {
    if (first_time) {
      out << " : ";
      first_time = false;
    }
    else out << ", ";
    iter->print(env, out);
  }
  PairDelim pair(out, "", "{\n", "}");
  FOREACH_ASTLIST_NC(Member, members->list, iter2) {
    iter2.data()->print(env, out);
  }
}

void TS_enumSpec::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TS_classSpec");
  out << toString(ql);          // see string toString(class dummy_type*) above
  out << toString(cv);
  out << "enum ";
  if (name) out << name;
  PairDelim pair(out, "", "{\n", "}");
  FAKELIST_FOREACH_NC(Enumerator, elts, iter) {
    iter->print(env, out);
    out << "\n";
  }
}

// BaseClass
void BaseClassSpec::print(PrintEnv &env, CodeOutStream &out) {
  TreeWalkDebug treeDebug("BaseClassSpec");
  if (isVirtual) out << "virtual ";
  if (access!=AK_UNSPECIFIED) out << toString(access) << " ";
  out << name->toString();
}

// MemberList

// ---------------------- Member ----------------------
void MR_decl::print(PrintEnv &env, CodeOutStream &out)
{                   
  TreeWalkDebug treeDebug("MR_decl");
  d->print(env, out);
}

void MR_func::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("MR_func");
  f->print(env, out);
}

void MR_access::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("MR_access");
  out << toString(k) << ":\n";
}

void MR_usingDecl::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("MR_usingDecl");
  decl->print(env, out);
}

void MR_template::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("MR_template");
  d->print(env, out);
}


// -------------------- Enumerator --------------------
void Enumerator::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("Enumerator");
  out << name;
  if (expr) {
    out << "=";
    expr->print(env, out);
  }
  out << ", ";
}

// -------------------- Declarator --------------------
void Declarator::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("Declarator");

  printVar(env, out, var, decl->getDeclaratorId());
  D_bitfield *b = dynamic_cast<D_bitfield*>(decl);
  if (b) {
    out << ":";
    b->bits->print(env, out);
  }
  if (init) {
    IN_ctor *ctor = dynamic_cast<IN_ctor*>(init);
    if (ctor) {
      // sm: don't print "()" as an IN_ctor initializer (cppstd 8.5 para 8)
      if (ctor->args->isEmpty()) {
        out << " /*default-ctor-init*/";
      }
      else {
        // dsw:Constructor arguments.
        PairDelim pair(out, "", "(", ")");
        ctor->print(env, out);       // NOTE: You can NOT factor this line out of the if!
      }
    } else {
      out << "=";
      init->print(env, out);         // Don't pull this out!
    }
  }
}

// ------------------- ExceptionSpec --------------------
void ExceptionSpec::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("ExceptionSpec");
  out << "throw"; // Scott says this is right.
  FAKELIST_FOREACH_NC(ASTTypeId, types, iter) {
    iter->print(env, out);
  }
}

// ---------------------- Statement ---------------------
void Statement::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("Statement");
  env.loc = loc;
  iprint(env, out);
  //    out << ";\n";
}

// no-op
void S_skip::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_skip::iprint");
  out << ";\n";
}

void S_label::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_label::iprint");
  out << name << ":";
  s->print(env, out);
}

void S_case::iprint(PrintEnv &env, CodeOutStream &out)
{                    
  TreeWalkDebug treeDebug("S_case::iprint");
  out << "case";
  expr->print(env, out);
  out << ":";
  s->print(env, out);
}

void S_default::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_default::iprint");
  out << "default:";
  s->print(env, out);
}

void S_expr::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_expr::iprint");
  expr->print(env, out);
  out << ";\n";
}

void S_compound::iprint(PrintEnv &env, CodeOutStream &out)
{ 
  TreeWalkDebug treeDebug("S_compound::iprint");
  PairDelim pair(out, "", "{\n", "}\n");
  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    iter.data()->print(env, out);
  }
}

void S_if::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_if::iprint");
  {
    PairDelim pair(out, "if", "(", ")");
    cond->print(env, out);
  }
  thenBranch->print(env, out);
  out << "else ";
  elseBranch->print(env, out);
}

void S_switch::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_switch::iprint");
  {
    PairDelim pair(out, "switch", "(", ")");
    cond->print(env, out);
  }
  branches->print(env, out);
}

void S_while::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_while::iprint");
  {
    PairDelim pair(out, "while", "(", ")");
    cond->print(env, out);
  }
  body->print(env, out);
}

void S_doWhile::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_doWhile::iprint");
  {
    PairDelim pair(out, "do");
    body->print(env, out);
  }
  {
    PairDelim pair(out, "while", "(", ")");
    expr->print(env, out);
  }
  out << ";\n";
}

void S_for::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_for::iprint");
  {
    PairDelim pair(out, "for", "(", ")");
    init->print(env, out);
    // this one not needed as the declaration provides one
    //          out << ";";
    cond->print(env, out);
    out << ";";
    after->print(env, out);
  }
  body->print(env, out);
}

void S_break::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_break::iprint");
  out << "break";
  out << ";\n";
}

void S_continue::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_continue::iprint");
  out << "continue";
  out << ";\n";
}

void S_return::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_return::iprint");
  out << "return";
  if (expr) expr->print(env, out);
  out << ";\n";
}

void S_goto::iprint(PrintEnv &env, CodeOutStream &out)
{
  // dsw: When doing a control-flow pass, keep a current function so
  // we know where to look for the label.
  TreeWalkDebug treeDebug("S_goto::iprint");
  out << "goto ";
  out << target;
  out << ";\n";
}

void S_decl::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_decl::iprint");
  decl->print(env, out);
  //      out << ";\n";
}

void S_try::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_try::iprint");
  out << "try";
  body->print(env, out);
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->print(env, out);
  }
}

void S_asm::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_asm::iprint");
  out << "asm(" << text << ");\n";
}

void S_namespaceDecl::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("S_namespaceDecl::iprint");
  decl->print(env, out);
}


// ------------------- Condition --------------------
// CN = ConditioN

// this situation: if (gronk()) {...
void CN_expr::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("CN_expr");
  expr->print(env, out);
}

// this situation: if (bool b=gronk()) {...
void CN_decl::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("CN_decl");
  typeId->print(env, out);
}

// ------------------- Handler ----------------------
// catch clause
void Handler::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("Handler");
  {
    PairDelim pair(out, "catch", "(", ")");
    if (isEllipsis()) {
      out << "...";
    }
    else {
      typeId->print(env, out);
    }
  }
  body->print(env, out);
}


// ------------------- Full Expression print -----------------------
void FullExpression::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("FullExpression");
  // FIX: for now I omit printing the declarations of the temporaries
  // since we really don't have a syntax for it.  We would have to
  // print some curlies somewhere to make it legal to parse it back in
  // again, and we aren't using E_statement, so it would not reflect
  // the actual ast.
  expr->print(env, out);
}


// ------------------- Expression print -----------------------

// dsw: Couldn't we have fewer functions for printing out an
// expression?  Or at least name them in a way that reveals some sort
// of system.

void Expression::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("Expression");
  PairDelim pair(out, "", "(", ")"); // this will put parens around every expression
  iprint(env, out);
}

string Expression::exprToString() const
{              
  TreeWalkDebug treeDebug("Expression::exprToString");
  stringBuilder sb;
  StringBuilderOutStream out0(sb);
  CodeOutStream codeOut(out0);
  TypePrinterC typePrinter;
  PrintEnv env(typePrinter);
  
  // sm: I think all the 'print' methods should be 'const', but
  // I'll leave such a change up to this module's author (dsw)
  const_cast<Expression*>(this)->print(env, codeOut);
  codeOut.flush();

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


void E_boolLit::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_boolLit::iprint");
  out << b;
}

void E_intLit::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_intLit::iprint");
  // FIX: do this correctly from the internal representation
  // fails to print the trailing U for an unsigned int.
//    out << i;
  out << text;
}

void E_floatLit::iprint(PrintEnv &env, CodeOutStream &out)
{                                
  TreeWalkDebug treeDebug("E_floatLit::iprint");
  // FIX: do this correctly from the internal representation
  // this fails to print ".0" for a float/double that happens to lie
  // on an integer boundary
//    out << d;
  // doing it this way also preserves the trailing "f" for float
  // literals
  out << text;
}

void E_stringLit::iprint(PrintEnv &env, CodeOutStream &out)
{                                                                     
  TreeWalkDebug treeDebug("E_stringLit::iprint");
  
  E_stringLit *p = this;
  while (p) {
    out << p->text;
    p = p->continuation;
    if (p) {
      out << " ";
    }
  }
}

void E_charLit::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_charLit::iprint");
  out << text;
}

void E_this::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_this::iprint");
  out << "this";
}

// modified from STemplateArgument::toString()
void printSTemplateArgument(PrintEnv &env, CodeOutStream &out, STemplateArgument const *sta)
{
  switch (sta->kind) {
    default: xfailure("bad kind");
    case STemplateArgument::STA_NONE:
      out << string("STA_NONE");
      break;
    case STemplateArgument::STA_TYPE:
      env.typePrinter.print(out, sta->value.t); // assume 'type' if no comment
      break;
    case STemplateArgument::STA_INT:
      out << stringc << "/*int*/ " << sta->value.i;
      break;
    case STemplateArgument::STA_REFERENCE:
      out << stringc << "/*ref*/ " << sta->value.v->name;
      break;
    case STemplateArgument::STA_POINTER:
      out << stringc << "/*ptr*/ &" << sta->value.v->name;
      break;
    case STemplateArgument::STA_MEMBER:
      out << stringc
              << "/*member*/ &" << sta->value.v->scope->curCompound->name
              << "::" << sta->value.v->name;
      break;
    case STemplateArgument::STA_DEPEXPR:
      sta->getDepExpr()->print(env, out);
      break;
    case STemplateArgument::STA_TEMPLATE:
      out << string("template (?)");
      break;
  }
}

// print template args, if any
void printTemplateArgs(PrintEnv &env, CodeOutStream &out, Variable *var)
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
  out << "<";
  int ct=0;
  for (; !iter.isDone(); iter.adv()) {
    if (ct++ > 0) {
      out << ", ";
    }
    printSTemplateArgument(env, out, iter.data());
  }
  out << ">";
}

void E_variable::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_variable::iprint");
  if (var && var->isBoundTemplateParam()) {
    // this is a bound template variable, so print its value instead
    // of printing its name
    xassert(var->value);
    var->value->print(env, out);
  }
  else {
    out << name->qualifierString() << name->getName();
    printTemplateArgs(env, out, var);
  }
}

void printArgExprList(PrintEnv &env, CodeOutStream &out, FakeList<ArgExpression> *list)
{
  TreeWalkDebug treeDebug("printArgExprList");
  bool first_time = true;
  FAKELIST_FOREACH_NC(ArgExpression, list, iter) {
    if (first_time) first_time = false;
    else out << ", ";
    iter->expr->print(env, out);
  }
}

void E_funCall::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_funCall::iprint");
  func->print(env, out);
  PairDelim pair(out, "", "(", ")");
  printArgExprList(env, out, args);
}

void E_constructor::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_constructor::iprint");
  env.typePrinter.print(out, type);
  PairDelim pair(out, "", "(", ")");
  printArgExprList(env, out, args);
}

void E_fieldAcc::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_fieldAcc::iprint");
  obj->print(env, out);
  out << ".";
  if (field &&
      !field->type->isDependent()) {
    out << field->name;
    printTemplateArgs(env, out, field);
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
    out << fieldName->toString();
  }
}

void E_arrow::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_arrow::iprint");
  
  // E_arrow shouldn't normally be present in code that is to be
  // prettyprinted, so it doesn't much matter what this does.
  obj->print(env, out);
  out << "->";
  fieldName->print(env, out);
}

void E_sizeof::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_sizeof::iprint");
  // NOTE parens are not necessary because it's an expression, not a
  // type.
  out << "sizeof";
  expr->print(env, out);           // putting parens in here so we are safe wrt precedence
}

// dsw: unary expression?
void E_unary::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_unary::iprint");
  out << toString(op);
  expr->print(env, out);
}

void E_effect::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_effect::iprint");
  if (!isPostfix(op)) out << toString(op);
  expr->print(env, out);
  if (isPostfix(op)) out << toString(op);
}

// dsw: binary operator.
void E_binary::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_binary::iprint");
  e1->print(env, out);
  if (op != BIN_BRACKETS) {
    out << toString(op);
    e2->print(env, out);
  }
  else {
    out << "[";
    e2->print(env, out);
    out << "]";
  }
}

void E_addrOf::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_addrOf::iprint");
  out << "&";
  if (expr->isE_variable()) {
    // could be forming ptr-to-member, do not parenthesize
    expr->iprint(env, out);
  }
  else {
    expr->print(env, out);
  }
}

void E_deref::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_deref::iprint");
  out << "*";
  ptr->print(env, out);
}

// C-style cast
void E_cast::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_cast::iprint");
  {
    PairDelim pair(out, "", "(", ")");
    ctype->print(env, out);
  }
  expr->print(env, out);
}

// ? : syntax
void E_cond::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_cond::iprint");
  cond->print(env, out);
  out << "?";
  // In gcc it is legal to omit the 'then' part;
  // http://gcc.gnu.org/onlinedocs/gcc-3.4.1/gcc/Conditionals.html#Conditionals
  if (th) {
    th->print(env, out);
  }
  out << ":";
  el->print(env, out);
}

void E_sizeofType::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_sizeofType::iprint");
  PairDelim pair(out, "sizeof", "(", ")"); // NOTE yes, you do want the parens because argument is a type.
  atype->print(env, out);
}

void E_assign::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_assign::iprint");
  target->print(env, out);
  if (op!=BIN_ASSIGN) out << toString(op);
  out << "=";
  src->print(env, out);
}

void E_new::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_new::iprint");
  if (colonColon) out << "::";
  out << "new ";
  if (placementArgs) {
    PairDelim pair(out, "", "(", ")");
    printArgExprList(env, out, placementArgs);
  }

  if (!arraySize) {
    // no array size, normal type-id printing is fine
    atype->print(env, out);
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
    out << t->leftString() << " [";
    arraySize->print(env, out);
    out << "]" << t->rightString();
  }

  if (ctorArgs) {
    PairDelim pair(out, "", "(", ")");
    printArgExprList(env, out, ctorArgs->list);
  }
}

void E_delete::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_delete::iprint");
  if (colonColon) out << "::";
  out << "delete";
  if (array) out << "[]";
  // dsw: this can be null because elaboration can remove syntax when
  // it is replaced with other syntax
  if (expr) {
    expr->print(env, out);
  }
}

void E_throw::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_throw::iprint");
  out << "throw";
  if (expr) expr->print(env, out);
}

// C++-style cast
void E_keywordCast::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_keywordCast::iprint");
  out << toString(key);
  {
    PairDelim pair(out, "", "<", ">");
    ctype->print(env, out);
  }
  PairDelim pair(out, "", "(", ")");
  expr->print(env, out);
}

// RTTI: typeid(expression)
void E_typeidExpr::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_typeidExpr::iprint");
  PairDelim pair(out, "typeid", "(", ")");
  expr->print(env, out);
}

// RTTI: typeid(type)
void E_typeidType::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_typeidType::iprint");
  PairDelim pair(out, "typeid", "(", ")");
  ttype->print(env, out);
}

void E_grouping::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("E_grouping::iprint");
  
  // sm: given that E_grouping is now in the tree, and prints its
  // parentheses, perhaps we could eliminate some of the
  // paren-printing above?
  //PairDelim pair(out, "", "(", ")");
  //
  // update:  Actually, it's a problem for E_grouping to print parens
  // because it messes up idempotency.  And, if we restored idempotency
  // by turning off paren-printing elsewhere, then we'd have a subtle
  // long-term problem that AST transformations would be required to
  // insert E_grouping when composing new expression trees, and that
  // would suck.  So I'll let E_grouping be a no-op, and continue to
  // idly plan some sort of precedence-aware paren-inserter mechanism.

  expr->iprint(env, out);    // iprint means Expression won't put parens either
}

// ----------------------- Initializer --------------------

// this is under a declaration
// int x = 3;
//         ^ only
void IN_expr::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("IN_expr");
  e->print(env, out);
}

// int x[] = {1, 2, 3};
//           ^^^^^^^^^ only
void IN_compound::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("IN_compound");
  PairDelim pair(out, "", "{\n", "\n}");
  bool first_time = true;
  FOREACH_ASTLIST_NC(Initializer, inits, iter) {
    if (first_time) first_time = false;
    else out << ",\n";
    iter.data()->print(env, out);
  }
}

void IN_ctor::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("IN_ctor");
  printArgExprList(env, out, args);
}

// InitLabel

// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::print(PrintEnv &env, CodeOutStream &out)
{ 
  TreeWalkDebug treeDebug("TemplateDeclaration");

  out << "template <";
  int ct=0;
  FAKELIST_FOREACH_NC(TemplateParameter, params, iter) {
    if (ct++ > 0) {
      out << ", ";
    }
    iter->print(env, out);
  }
  out << ">\n";

  iprint(env, out);
}

void printFuncInstantiations(PrintEnv &env, CodeOutStream &out, Variable const *var)
{
  TemplateInfo *ti = var->templateInfo();
  SFOREACH_OBJLIST(Variable, ti->instantiations, iter) {
    Variable const *inst = iter.data();
    if (inst->funcDefn) {
      inst->funcDefn->print(env, out);
    }
    else {
      out << inst->toQualifiedString() << ";    // decl but not defn\n";
    }
  }
}

void TD_func::iprint(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TD_func");
  f->print(env, out);

  // print instantiations
  Variable *var = f->nameAndParams->var;
  if (var->isTemplate() &&      // for complete specializations, don't print
      !var->templateInfo()->isPartialInstantiation()) {     // nor partial inst
    out << "#if 0    // instantiations of ";
    // NOTE: inlined from Variable::toCString()
    env.typePrinter.print(out, var->type, (var->name? var->name : "/*anon*/"));
    out << var->namePrintSuffix() << "\n";
    printFuncInstantiations(env, out, var);

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
        printFuncInstantiations(env, out, iter.data());
      }
    }

    out << "#endif   // instantiations of " << var->name << "\n\n";
  }
}

void TD_decl::iprint(PrintEnv &env, CodeOutStream &out)
{
  d->print(env, out);

  // print instantiations
  if (d->spec->isTS_classSpec()) {
    CompoundType *ct = d->spec->asTS_classSpec()->ctype;
    TemplateInfo *ti = ct->typedefVar->templateInfo();
    if (!ti->isCompleteSpec()) {
      out << "#if 0    // instantiations of " << ct->name << "\n";

      SFOREACH_OBJLIST(Variable, ti->instantiations, iter) {
        Variable const *instV = iter.data();

        out << "// ";
        env.typePrinter.print(out, instV->type);
        CompoundType *instCT = instV->type->asCompoundType();
        if (instCT->syntax) {
          out << "\n";
          instCT->syntax->print(env, out);
          out << ";\n";
        }
        else {
          out << ";     // body not instantiated\n";
        }
      }
      out << "#endif   // instantiations of " << ct->name << "\n\n";
    }
  }
  else {
    // it could be a forward declaration of a template class;
    // do nothing more
  }
}

void TD_tmember::iprint(PrintEnv &env, CodeOutStream &out)
{
  d->print(env, out);
}


// ------------------- TemplateParameter ------------------
// sm: this isn't used..
void TP_type::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TP_type");
  out << "class " << name;
                          
  if (defaultType) {
    out << " = ";
    defaultType->print(env, out);
  }
}

void TP_nontype::print(PrintEnv &env, CodeOutStream &out)
{
  TreeWalkDebug treeDebug("TP_nontype");
  param->print(env, out);
}


// -------------------- TemplateArgument ------------------
void TA_type::print(PrintEnv &env, CodeOutStream &out)
{
  // dig down to prevent printing "/*anon*/" since template
  // type arguments are always anonymous so it's just clutter
  env.typePrinter.print(out, type->decl->var->type);
}

void TA_nontype::print(PrintEnv &env, CodeOutStream &out)
{
  expr->print(env, out);
}

void TA_templateUsed::print(PrintEnv &env, CodeOutStream &out)
{
  // the caller should have recognized the presence of TA_templateUsed,
  // adjusted its printing accordingly, and then skipped this element
  xfailure("do not print TA_templateUsed");
}


// -------------------- NamespaceDecl ---------------------
void ND_alias::print(PrintEnv &env, CodeOutStream &out)
{
  out << "namespace " << alias << " = ";
  original->print(env, out);
  out << ";\n";
}

void ND_usingDecl::print(PrintEnv &env, CodeOutStream &out)
{
  out << "using ";
  name->print(env, out);
  out << ";\n";
}

void ND_usingDir::print(PrintEnv &env, CodeOutStream &out)
{
  out << "using namespace ";
  name->print(env, out);
  out << ";\n";
}


// EOF
