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
  bool isTreeNodePtr(char const *type);

  bool isListType(char const *type);
  string extractListType(char const *type);

  // canned output sequences
  void doNotEdit();
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

    // search among defined classes for this name
    FOREACH_ASTLIST(ToplevelForm, file.forms, form) {
      ASTClass const *c = form.data()->ifASTClassC();
      if (!c) continue;

      if (c->name == base) {
        // found it in a superclass
        return true;
      }

      // check the subclasses
      FOREACH_ASTLIST(ASTCtor, c->ctors, ctor) {
        if (ctor.data()->name == base) {
          // found it in a subclass
          return true;
        }
      }
    }
  }

  // not a pointer type, or else failed to find it among defined classes
  return false;
}


// is this type a use of my ASTList template?
bool Gen::isListType(char const *type)
{
  // do a fairly coarse analysis..
  return 0==strncmp(type, "ASTList <", 9);
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


// ------------------ generation of the header -----------------------
// translation-wide state
class HGen : public Gen {
private:        // funcs
  void emitVerbatim(TF_verbatim const &v);
  void emitASTClass(ASTClass const &cls);
  static char const *virtualIfChildren(ASTClass const &cls);
  void emitCtorFields(ASTList<CtorArg> const &args);
  void emitCtorDefn(char const *className, ASTList<CtorArg> const &args);
  void emitCommonFuncs(char const *virt);
  void emitUserDecls(ASTList<UserDecl> const &decls);
  void visitCtor(ASTClass const &parent, ASTCtor const &ctor);

public:         // funcs
  HGen(char const *srcFname, char const *destFname, ASTSpecFile const &file)
    : Gen(srcFname, destFname, file)
  {}
  void emitFile();
};


// emit header code for an entire AST spec file
void HGen::emitFile()
{
  string includeLatch = translate(basename(destFname), "a-z.", "A-Z_");

  // header of comments
  out << "// " << basename(destFname) << "\n";
  doNotEdit();
  out << "// generated automatically by astgen, from " << basename(srcFname) << "\n";
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
    ASTClass const *c = form.data()->ifASTClassC();
    if (c) {
      out << "class " << c->name << ";\n";

      FOREACH_ASTLIST(ASTCtor, c->ctors, ctor) {
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

      case ToplevelForm::ASTCLASS:
        emitASTClass(*( form.data()->asASTClassC() ));
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
  out << v.code;
}


STATICDEF char const *HGen::virtualIfChildren(ASTClass const &cls)
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
void HGen::emitASTClass(ASTClass const &cls)
{
  doNotEdit();
  out << "class " << cls.name << " {\n";

  emitCtorFields(cls.superCtor);
  emitCtorDefn(cls.name, cls.superCtor);

  // destructor
  char const *virt = virtualIfChildren(cls);
  out << "  " << virt << "~" << cls.name << "();\n";
  out << "\n";

  // declare the child kind selector
  if (cls.hasChildren()) {
    out << "  enum Kind { ";
    FOREACH_ASTLIST(ASTCtor, cls.ctors, ctor) {
      out << ctor.data()->kindName() << ", ";
    }
    out << "NUM_KINDS };\n";

    out << "  virtual Kind kind() const = 0;\n";
    out << "\n";
  }

  // declare checked downcast functions
  {
    FOREACH_ASTLIST(ASTCtor, cls.ctors, ctor) {
      // declare the const downcast
      ASTCtor const &c = *(ctor.data());
      out << "  DECL_AST_DOWNCASTS(" << c.name << ")\n";
    }
    out << "\n";
  }

  emitCommonFuncs(virt);

  emitUserDecls(cls.decls);

  // close the declaration of the parent class
  out << "};\n";
  out << "\n";

  // print declarations for all child classes
  {
    FOREACH_ASTLIST(ASTCtor, cls.ctors, ctor) {
      visitCtor(cls, *(ctor.data()));
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
      out << "  " << arg.data()->type << " " << arg.data()->name << ";\n";
    }
  }
  out << "\n";
}


// emit the definition of the constructor itself
void HGen::emitCtorDefn(char const *className, ASTList<CtorArg> const &args)
{
  // declare the constructor
  {
    out << "public:      // funcs\n";
    out << "  " << className << "(";

    // list of formal parameters to the constructor
    {
      int ct = 0;
      FOREACH_ASTLIST(CtorArg, args, arg) {
        if (ct++ > 0) {
          out << ", ";
        }

        out << arg.data()->type << " ";
        if (isListType(arg.data()->type)) {
          // lists are constructed by passing pointers
          out << "*";
        }
        out << "_" << arg.data()->name;      // prepend underscore to param's name
      }
    }
    out << ")";

    // declare initializers
    {
      int ct = 0;
      FOREACH_ASTLIST(CtorArg, args, arg) {
        if (ct++ > 0) {
          out << ", ";
        }
        else {
          out << " : ";
        }

        // initialize the field with the formal argument
        out << arg.data()->name << "(_" << arg.data()->name << ")";
      }
    }

    // empty ctor body (later I'll add a way for user code to go here)
    out << " {}\n";
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
void HGen::emitUserDecls(ASTList<UserDecl> const &decls)
{
  FOREACH_ASTLIST(UserDecl, decls, decl) {
    out << "  " << toString(decl.data()->access) << ": " << decl.data()->code << "\n";
  }
}

// emit declaration for a specific class instance constructor
void HGen::visitCtor(ASTClass const &parent, ASTCtor const &ctor)
{
  out << "class " << ctor.name << " : public " << parent.name << " {\n";

  emitCtorFields(ctor.args);
  emitCtorDefn(ctor.name, ctor.args);

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

  // close the decl
  out << "};\n";
  out << "\n";
}


// --------------------- generation of C++ code file --------------------------
class CGen : public Gen {
public:
  CGen(char const *srcFname, char const *destFname, ASTSpecFile const &file)
    : Gen(srcFname, destFname, file)
  {}

  void emitFile();
  void emitVerbatim(TF_verbatim const &v);
  void emitASTClass(ASTClass const &cls);
  void emitDestructor(char const *clsname, ASTList<CtorArg> const &args);
  void emitPrintCtorArgs(ASTList<CtorArg> const &args);
};



void CGen::emitFile()
{
  out << "// " << basename(destFname) << "\n";
  doNotEdit();
  out << "// automatically generated by astgen, from " << basename(srcFname) << "\n";
  out << "\n";
  out << "#include \"ast.gen.h\"      // this module\n";
  out << "\n";
  out << "\n";

  FOREACH_ASTLIST(ToplevelForm, file.forms, form) {
    ASTSWITCHC(ToplevelForm, form.data()) {
      ASTCASEC(TF_verbatim, v) {
        emitVerbatim(*v);
      }
      ASTNEXTC(ASTClass, c) {
        emitASTClass(*c);
      }
      ASTENDCASEC
    }
  }
}


void CGen::emitVerbatim(TF_verbatim const &)
{
  // do nothing for verbatim ... that goes into the header ..
}

void CGen::emitASTClass(ASTClass const &cls)
{
  out << "// ------------------ " << cls.name << " -------------------\n";
  doNotEdit();

  // class destructor
  emitDestructor(cls.name, cls.superCtor);
  
  // debugPrint
  out << "void " << cls.name << "::debugPrint(ostream &os, int indent) const\n";
  out << "{\n";
  if (!cls.hasChildren()) {
    // childless superclasses print headers; otherwise the subclass
    // prints the header
    out << "  PRINT_HEADER(" << cls.name << ");\n";
    out << "\n";
  }

  emitPrintCtorArgs(cls.superCtor);

  out << "}\n";
  out << "\n";

  // constructors (class hierarchy children)
  FOREACH_ASTLIST(ASTCtor, cls.ctors, ctoriter) {
    ASTCtor const &ctor = *(ctoriter.data());

    // downcast function
    out << "DEFN_AST_DOWNCASTS(" << cls.name << ", "
                                 << ctor.name << ", "
                                 << ctor.kindName() << ")\n";
    out << "\n";

    // subclass destructor
    emitDestructor(ctor.name, ctor.args);

    // subclass debugPrint
    out << "void " << ctor.name << "::debugPrint(ostream &os, int indent) const\n";
    out << "{\n";
    out << "  PRINT_HEADER(" << ctor.name << ");\n";
    out << "\n";

    // call the superclass's fn to get its data members
    out << "  " << cls.name << "::debugPrint(os, indent);\n";
    out << "\n";

    emitPrintCtorArgs(ctor.args);

    out << "}\n";
    out << "\n";
  }

  out << "\n";
}


void CGen::emitDestructor(char const *clsname, ASTList<CtorArg> const &args)
{
  out << clsname << "::~" << clsname << "()\n";
  out << "{\n";

  FOREACH_ASTLIST(CtorArg, args, argiter) {
    CtorArg const &arg = *(argiter.data());

    if (isListType(arg.type)) {
      // explicitly destroy list elements, because it's easy to do, and
      // because if there is a problem, it's much easier to see its
      // role in a debugger backtrace
      out << "  " << arg.name << ".deleteAll();\n";
    }
    else if (arg.owner) {
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
      out << "  PRINT_LIST(" << extractListType(arg.type) << ", "
                             << arg.name << ");\n";
    }
    else if (isTreeNodePtr(arg.type)) {
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

  if (argc != 2) {
    cout << "usage: " << argv[0] << " ast-spec-file\n";
    return;
  }
  char const *srcFname = argv[1];

  // parse the grammar spec
  Owner<ASTSpecFile> ast;
  ast = readAbstractGrammar(srcFname);

  // generate the header
  string base = replace(srcFname, ".ast", "");
  string destFname = base & ".gen.h";
  cout << "writing " << destFname << "...\n";
  HGen hg(srcFname, destFname, *ast);
  hg.emitFile();

  // generated the c++ code
  destFname = base & ".gen.cc";
  cout << "writing " << destFname << "...\n";
  CGen cg(srcFname, destFname, *ast);
  cg.emitFile();
}

ARGS_MAIN
