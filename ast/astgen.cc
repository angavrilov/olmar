// astgen.cc
// program to generate C++ code from an AST specification

#include "agrampar.h"      // readAbstractGrammar
#include "test.h"          // ARGS_MAIN
#include "trace.h"         // TRACE_ARGS
#include "owner.h"         // Owner
#include "ckheap.h"        // checkHeap
#include "strutil.h"       // replace, translate, localTimeString

#include <string.h>        // strncmp
#include <fstream.h>       // ofstream


// ------------------ shared gen functions ----------------------
class Gen {
protected:        // data
  string srcFname;           // name of source file
  string destFname;          // name of output file
  ofstream out;              // output stream
  ASTSpecFile const &file;   // AST specification

public:
  Gen(char const *srcFname, char const *destFname, ASTSpecFile const &file);
  ~Gen();

  // type queries
  bool isTreeNode(char const *type);
  bool isTreeNodePtr(char const *type);

  bool isListType(char const *type);
  bool isTreeListType(char const *type);
  string extractListType(char const *type);

  // shared output sequences
  void doNotEdit();
  void emitFiltered(ASTList<Annotation> const &decls, AccessCtl mode,
                    char const *indent);
};


Gen::Gen(char const *srcfn, char const *destfn, ASTSpecFile const &f)
  : srcFname(srcfn),
    destFname(destfn),
    out(destfn),
    file(f)
{
  if (!out) {
    throw_XOpen(destfn);
  }
}

Gen::~Gen()
{}


bool Gen::isTreeNodePtr(char const *type)
{
  if (type[strlen(type)-1] == '*') {
    // is pointer type; get base type
    string base = trimWhitespace(string(type, strlen(type)-1));

    return isTreeNode(base);
  }

  // not a pointer type
  return false;
}


bool Gen::isTreeNode(char const *base)
{
  // search among defined classes for this name
  FOREACH_ASTLIST(ToplevelForm, file.forms, form) {
    TF_class const *c = form.data()->ifTF_classC();
    if (!c) continue;

    if (c->super->name.equals(base)) {
      // found it in a superclass
      return true;
    }

    // check the subclasses
    FOREACH_ASTLIST(ASTClass, c->ctors, ctor) {
      if (ctor.data()->name.equals(base)) {
        // found it in a subclass
        return true;
      }
    }
  }

  return false;
}


// is this type a use of my ASTList template?
bool Gen::isListType(char const *type)
{
  // do a fairly coarse analysis..
  return 0==strncmp(type, "ASTList <", 9);
}

// is it a list type, with the elements being tree nodes?
bool Gen::isTreeListType(char const *type)
{
  return isListType(type) && isTreeNode(type+9);
}

// given a type for which 'isListType' returns true, extract
// the type in the template argument angle brackets
string Gen::extractListType(char const *type)
{
  xassert(isListType(type));
  char const *langle = strchr(type, '<');
  char const *rangle = strchr(type, '>');
  xassert(langle && rangle);
  return trimWhitespace(string(langle+1, rangle-langle-1));
}


// I scatter this throughout the generated code as pervasive
// reminders that these files shouldn't be edited -- I often
// accidentally edit them and then have to backtrack and reapply
// the changes to where they were supposed to go ..
void Gen::doNotEdit()
{
  out << "// *** DO NOT EDIT ***\n";
}


void Gen::emitFiltered(ASTList<Annotation> const &decls, AccessCtl mode,
                       char const *indent)
{
  FOREACH_ASTLIST(Annotation, decls, iter) {
    if (iter.data()->kind() == Annotation::USERDECL) {
      UserDecl const &decl = *( iter.data()->asUserDeclC() );
      if (decl.access == mode) {
        out << indent << decl.code << ";\n";
      }                                                     
    }
  }
}


// ------------------ generation of the header -----------------------
// translation-wide state
class HGen : public Gen {
private:        // funcs
  void emitVerbatim(TF_verbatim const &v);
  void emitTFClass(TF_class const &cls);
  static char const *virtualIfChildren(TF_class const &cls);
  void emitCtorFields(ASTList<CtorArg> const &args);
  void emitCtorFormal(int &ct, CtorArg const *arg);
  void emitCtorDefn(ASTClass const &cls, ASTClass const *parent);
  void emitCommonFuncs(char const *virt);
  void emitUserDecls(ASTList<Annotation> const &decls);
  void emitCtor(ASTClass const &ctor, ASTClass const &parent);

public:         // funcs
  HGen(char const *srcFname, char const *destFname, ASTSpecFile const &file)
    : Gen(srcFname, destFname, file)
  {}
  void emitFile();
};


// emit header code for an entire AST spec file
void HGen::emitFile()
{
  string includeLatch = translate(sm_basename(destFname), "a-z.", "A-Z_");

  // header of comments
  out << "// " << sm_basename(destFname) << "\n";
  doNotEdit();
  out << "// generated automatically by astgen, from " << sm_basename(srcFname) << "\n";
  // the inclusion of the date introduces gratuitous changes when the
  // tool is re-run for whatever reason
  //out << "//   on " << localTimeString() << "\n";
  out << "\n";
  out << "#ifndef " << includeLatch << "\n";
  out << "#define " << includeLatch << "\n";
  out << "\n";
  out << "#include \"asthelp.h\"        // helpers for generated code\n";
  out << "\n";

  // forward-declare all the classes
  out << "// fwd decls\n";
  FOREACH_ASTLIST(ToplevelForm, file.forms, form) {
    TF_class const *c = form.data()->ifTF_classC();
    if (c) {
      out << "class " << c->super->name << ";\n";

      FOREACH_ASTLIST(ASTClass, c->ctors, ctor) {
        out << "class " << ctor.data()->name << ";\n";
      }
    }
  }
  out << "\n";

  // process each directive
  FOREACH_ASTLIST(ToplevelForm, file.forms, form) {
    switch (form.data()->kind()) {
      case ToplevelForm::TF_VERBATIM:
        emitVerbatim(*( form.data()->asTF_verbatimC() ));
        break;

      case ToplevelForm::TF_IMPL_VERBATIM:
        // nop
        break;

      case ToplevelForm::TF_CLASS:
        emitTFClass(*( form.data()->asTF_classC() ));
        break;

      default:
        xfailure("bad AST kind code");
    }

    out << "\n";
  }

  out << "#endif // " << includeLatch << "\n";
}


// emit a verbatim section
void HGen::emitVerbatim(TF_verbatim const &v)
{
  doNotEdit();
  out << v.code;
}


STATICDEF char const *HGen::virtualIfChildren(TF_class const &cls)
{
  if (cls.hasChildren()) {
    // since this class has children, make certain functions virtual
    return "virtual ";
  }
  else {
    // no children, no need to introduce a vtable
    return "";
  }
}


// emit declaration for a class ("phylum")
void HGen::emitTFClass(TF_class const &cls)
{
  doNotEdit();
  out << "class " << cls.super->name << " {\n";

  emitCtorFields(cls.super->args);
  emitCtorDefn(*(cls.super), NULL /*parent*/);

  // destructor
  char const *virt = virtualIfChildren(cls);
  out << "  " << virt << "~" << cls.super->name << "();\n";
  out << "\n";

  // declare the child kind selector
  if (cls.hasChildren()) {
    out << "  enum Kind { ";
    FOREACH_ASTLIST(ASTClass, cls.ctors, ctor) {
      out << ctor.data()->kindName() << ", ";
    }
    out << "NUM_KINDS };\n";

    out << "  virtual Kind kind() const = 0;\n";
    out << "\n";
    out << "  static char const * const kindNames[NUM_KINDS];\n";
    out << "  char const *kindName() const { return kindNames[kind()]; }\n";
    out << "\n";
  }

  // declare checked downcast functions
  {
    FOREACH_ASTLIST(ASTClass, cls.ctors, ctor) {
      // declare the const downcast
      ASTClass const &c = *(ctor.data());
      out << "  DECL_AST_DOWNCASTS(" << c.name << ", " << c.kindName() << ")\n";
    }
    out << "\n";
  }

  emitCommonFuncs(virt);

  emitUserDecls(cls.super->decls);

  // close the declaration of the parent class
  out << "};\n";
  out << "\n";

  // print declarations for all child classes
  {
    FOREACH_ASTLIST(ASTClass, cls.ctors, ctor) {
      emitCtor(*(ctor.data()), *(cls.super));
    }
  }

  out << "\n";
}


// emit data fields implied by the constructor
void HGen::emitCtorFields(ASTList<CtorArg> const &args)
{
  out << "public:      // data\n";

  // go over the arguments in the ctor and declare fields for them
  {
    FOREACH_ASTLIST(CtorArg, args, arg) {
      char const *star = "";
      if (isTreeNode(arg.data()->type)) {
        // make it a pointer in the concrete representation
        star = "*";
      }

      out << "  " << arg.data()->type << " " << star << arg.data()->name << ";\n";
    }
  }
  out << "\n";
}


// emit the declaration of a formal argument to a constructor
void HGen::emitCtorFormal(int &ct, CtorArg const *arg)
{
  // put commas between formals
  if (ct++ > 0) {
    out << ", ";
  }

  char const *type = arg->type;
  out << type << " ";
  if (isListType(type) ||
      isTreeNode(type) ||
      0==strcmp(type, "LocString")) {
    // lists and subtrees and LocStrings are constructed by passing pointers
    trace("putStar") << "putting star for " << type << endl;
    out << "*";
  }
  else {
    trace("putStar") << "NOT putting star for " << type << endl;
  }

  out << "_" << arg->name;      // prepend underscore to param's name
}


// emit the definition of the constructor itself
void HGen::emitCtorDefn(ASTClass const &cls, ASTClass const *parent)
{
  // declare the constructor
  {
    out << "public:      // funcs\n";
    out << "  " << cls.name << "(";

    // list of formal parameters to the constructor
    {
      int ct = 0;
      if (parent) {
        FOREACH_ASTLIST(CtorArg, parent->args, parg) {
          emitCtorFormal(ct, parg.data());
        }
      }
      FOREACH_ASTLIST(CtorArg, cls.args, arg) {
        emitCtorFormal(ct, arg.data());
      }
    }
    out << ")";

    // pass args to superclass, and initialize fields
    {
      int ct = 0;

      if (parent) {
        out << " : " << parent->name << "(";
        FOREACH_ASTLIST(CtorArg, parent->args, parg) {
          if (ct++ > 0) {
            out << ", ";
          }
          // pass the formal arg to the parent ctor
          out << "_" << parg.data()->name;
        }
        ct++;     // make sure we print a comma, below
        out << ")";
      }

      FOREACH_ASTLIST(CtorArg, cls.args, arg) {
        if (ct++ > 0) {
          out << ", ";
        }
        else {
          out << " : ";    // only used when 'parent' is NULL
        }

        // initialize the field with the formal argument
        out << arg.data()->name << "(_" << arg.data()->name << ")";
      }
    }

    // insert user's ctor code
    out << " {\n";
    emitFiltered(cls.decls, AC_CTOR, "    ");
    out << "  }\n";
  }
}

// emit functions that are declared in every tree node
void HGen::emitCommonFuncs(char const *virt)
{
  // declare the one function they all have
  out << "  " << virt << "void debugPrint(ostream &os, int indent) const;\n";
  out << "\n";
}

// emit user-supplied declarations
void HGen::emitUserDecls(ASTList<Annotation> const &decls)
{
  FOREACH_ASTLIST(Annotation, decls, iter) {
    // in the header, we only look at userdecl annotations
    if (iter.data()->kind() == Annotation::USERDECL) {
      UserDecl const &decl = *( iter.data()->asUserDeclC() );
      if (decl.access == AC_PUBLIC ||
          decl.access == AC_PRIVATE ||
          decl.access == AC_PROTECTED) {
        out << "  " << toString(decl.access) << ": " << decl.code << ";\n";
      }
      if (decl.access == AC_PUREVIRT) {
        // this is the parent class: declare it pure virtual
        out << "  public: virtual " << decl.code << "=0;\n";
      }
    }
  }
}

// emit declaration for a specific class instance constructor
void HGen::emitCtor(ASTClass const &ctor, ASTClass const &parent)
{
  out << "class " << ctor.name << " : public " << parent.name << " {\n";

  emitCtorFields(ctor.args);
  emitCtorDefn(ctor, &parent);

  // destructor
  out << "  virtual ~" << ctor.name << "();\n";
  out << "\n";

  // type tag
  out << "  virtual Kind kind() const { return " << ctor.kindName() << "; }\n";
  out << "  enum { TYPE_TAG = " << ctor.kindName() << " };\n";
  out << "\n";

  // common functions
  emitCommonFuncs("virtual ");

  emitUserDecls(ctor.decls);
  
  // emit implementation declarations for parent's pure virtuals
  FOREACH_ASTLIST(Annotation, parent.decls, iter) {
    UserDecl const *decl = iter.data()->ifUserDeclC();
    if (decl && decl->access == AC_PUREVIRT) {
      out << "  public: virtual " << decl->code << ";\n";
    }
  }

  // close the decl
  out << "};\n";
  out << "\n";
}


// --------------------- generation of C++ code file --------------------------
class CGen : public Gen {
public:
  string hdrFname;      // name of associated .h file

public:
  CGen(char const *srcFname, char const *destFname, ASTSpecFile const &file,
       char const *hdr)
    : Gen(srcFname, destFname, file),
      hdrFname(hdr)
  {}

  void emitFile();
  void emitTFClass(TF_class const &cls);
  void emitDestructor(ASTClass const &cls);
  void emitPrintCtorArgs(ASTList<CtorArg> const &args);
  void emitCustomCode(ASTList<Annotation> const &list, char const *tag);
};



void CGen::emitFile()
{
  out << "// " << sm_basename(destFname) << "\n";
  doNotEdit();
  out << "// automatically generated by astgen, from " << sm_basename(srcFname) << "\n";
  out << "\n";
  out << "#include \"" << hdrFname << "\"      // this module\n";
  out << "\n";
  out << "\n";

  FOREACH_ASTLIST(ToplevelForm, file.forms, form) {
    ASTSWITCHC(ToplevelForm, form.data()) {
      //ASTCASEC(TF_verbatim, v) {
      //  // nop
      //}
      ASTCASEC(TF_impl_verbatim, v) {
        doNotEdit();
        out << v->code;
      }
      ASTNEXTC(TF_class, c) {
        emitTFClass(*c);
      }
      ASTENDCASECD
    }
  }
}


void CGen::emitTFClass(TF_class const &cls)
{
  out << "// ------------------ " << cls.super->name << " -------------------\n";
  doNotEdit();

  // class destructor
  emitDestructor(*(cls.super));

  // kind name map
  if (cls.hasChildren()) {
    out << "char const * const " << cls.super->name << "::kindNames["
        <<   cls.super->name << "::NUM_KINDS] = {\n";
    FOREACH_ASTLIST(ASTClass, cls.ctors, ctor) {
      out << "  \"" << ctor.data()->name << "\",\n";
    }
    out << "};\n";
    out << "\n";
  }

  // debugPrint
  out << "void " << cls.super->name << "::debugPrint(ostream &os, int indent) const\n";
  out << "{\n";
  if (!cls.hasChildren()) {
    // childless superclasses print headers; otherwise the subclass
    // prints the header
    out << "  PRINT_HEADER(" << cls.super->name << ");\n";
    out << "\n";
  }

  // 10/31/01: decided I wanted custom debug print first, since it's
  // often much shorter (and more important) than the subtrees
  emitCustomCode(cls.super->decls, "debugPrint");
  emitPrintCtorArgs(cls.super->args);

  out << "}\n";
  out << "\n";

  // constructors (class hierarchy children)
  FOREACH_ASTLIST(ASTClass, cls.ctors, ctoriter) {
    ASTClass const &ctor = *(ctoriter.data());

    // downcast function
    out << "DEFN_AST_DOWNCASTS(" << cls.super->name << ", "
                                 << ctor.name << ", "
                                 << ctor.kindName() << ")\n";
    out << "\n";

    // subclass destructor
    emitDestructor(ctor);

    // subclass debugPrint
    out << "void " << ctor.name << "::debugPrint(ostream &os, int indent) const\n";
    out << "{\n";
    out << "  PRINT_HEADER(" << ctor.name << ");\n";
    out << "\n";

    // call the superclass's fn to get its data members
    out << "  " << cls.super->name << "::debugPrint(os, indent);\n";
    out << "\n";

    emitCustomCode(ctor.decls, "debugPrint");
    emitPrintCtorArgs(ctor.args);

    out << "}\n";
    out << "\n";
  }

  out << "\n";
}


void CGen::emitCustomCode(ASTList<Annotation> const &list, char const *tag)
{
  FOREACH_ASTLIST(Annotation, list, iter) {
    CustomCode const *cc = iter.data()->ifCustomCodeC();
    if (cc && cc->qualifier.equals(tag)) {
      out << "  " << cc->code << ";\n";
    }
  }
}


void CGen::emitDestructor(ASTClass const &cls)
{
  out << cls.name << "::~" << cls.name << "()\n";
  out << "{\n";

  // user's code first
  emitFiltered(cls.decls, AC_DTOR, "  ");

  FOREACH_ASTLIST(CtorArg, cls.args, argiter) {
    CtorArg const &arg = *(argiter.data());

    if (isTreeListType(arg.type)) {
      // explicitly destroy list elements, because it's easy to do, and
      // because if there is a problem, it's much easier to see its
      // role in a debugger backtrace
      out << "  " << arg.name << ".deleteAll();\n";
    }
    else if (isListType(arg.type)) {
      // we don't own the list elements; it's *essential* to
      // explicitly remove the elements; this is a hack, since the
      // ideal solution is to make a variant of ASTList which is
      // explicitly serf pointers.. the real ASTList doesn't have
      // a removeAll method (since it's an owner list), and rather
      // than corrupting that interface I'll emit the code each time..
      out << "  while (" << arg.name << ".isNotEmpty()) {\n"
          << "    " << arg.name << ".removeFirst();\n"
          << "  }\n";
      //out << "  " << arg.name << ".removeAll();\n";
    }
    else if (arg.owner || isTreeNode(arg.type)) {
      out << "  delete " << arg.name << ";\n";
    }
  }

  out << "}\n";
  out << "\n";
}


void CGen::emitPrintCtorArgs(ASTList<CtorArg> const &args)
{
  FOREACH_ASTLIST(CtorArg, args, argiter) {
    CtorArg const &arg = *(argiter.data());
    if (arg.type.equals("string")) {
      out << "  PRINT_STRING(" << arg.name << ");\n";
    }
    else if (isListType(arg.type)) {
      // for now, I'll continue to assume that any class that appears
      // in ASTList<> is compatible with the printing regime here
      out << "  PRINT_LIST(" << extractListType(arg.type) << ", "
                             << arg.name << ");\n";
    }
    else if (isTreeNode(arg.type) ||
             (isTreeNodePtr(arg.type) && arg.owner)) {
      // don't print subtrees that are possibly shared or circular
      out << "  PRINT_SUBTREE(" << arg.name << ");\n";
    }
    else if (arg.type.equals("bool")) {
      out << "  PRINT_BOOL(" << arg.name << ");\n";
    }
    else {
      // catch-all ..
      out << "  PRINT_GENERIC(" << arg.name << ");\n";
    }
  }
}



// --------------------- toplevel control ----------------------
void entry(int argc, char **argv)
{
  TRACE_ARGS();
  checkHeap();

  if (argc < 2) {
    cout << "usage: " << argv[0] << " [options] ast-spec-file\n"
         << "  options:\n"
         << "    -bname   output filenames are name.{h,cc}\n"
         << "             (default replaces .ast with .{h,cc})\n"
         ;

    return;
  }
                            
  char const *basename = NULL;      // nothing set

  argv++;
  while (argv[0][0] == '-') {
    if (argv[0][1] == 'b') {
      basename = argv[0]+2;
    }
    else {
      cout << "unknown option: " << argv[0] << "\n";
      exit(2);
    }
    argv++;
  }                              
  if (!argv[0]) {
    cout << "missing ast spec file name\n";
    exit(2);
  }

  char const *srcFname = argv[0];

  // parse the grammar spec
  Owner<ASTSpecFile> ast;
  ast = readAbstractGrammar(srcFname);

  // generate the header
  string base = replace(srcFname, ".ast", "");
  if (basename) {
    base = basename;
  }

  string hdrFname = base & ".h";
  cout << "writing " << hdrFname << "...\n";
  HGen hg(srcFname, hdrFname, *ast);
  hg.emitFile();

  // generated the c++ code
  string codeFname = base & ".cc";
  cout << "writing " << codeFname << "...\n";
  CGen cg(srcFname, codeFname, *ast, hdrFname);
  cg.emitFile();
}

ARGS_MAIN
