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


// translation-wide state
class Gen {
private:        // data
  string srcFname;         // name of source file
  string destFname;        // name of output file
  ofstream out;            // output stream

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
  Gen(char const *srcFname, char const *destFname);
  ~Gen();
  void emitFile(ASTSpecFile const &file);
};


// compute destination file name from source file name
Gen::Gen(char const *srcfn, char const *destfn)
  : srcFname(srcfn),
    destFname(destfn),
    out(destfn)
{
  if (!out) {
    throw_XOpen(destfn);
  }
}

Gen::~Gen()
{}


// emit header code for an entire AST spec file
void Gen::emitFile(ASTSpecFile const &file)
{
  string includeLatch = translate(destFname, "a-z.", "A-Z_");

  // header of comments
  out << "// " << destFname << "\n";
  out << "// *** DO NOT EDIT ***\n";
  out << "// generated automatically by astgen, from " << srcFname << "\n";
  out << "//   on " << localTimeString() << "\n";
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
void Gen::emitVerbatim(TF_verbatim const &v)
{
  out << v.code;
}


STATICDEF char const *Gen::virtualIfChildren(ASTClass const &cls)
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
void Gen::emitASTClass(ASTClass const &cls)
{
  out << "class " << cls.name << " {\n";

  emitCtorFields(cls.superCtor);
  emitCtorDefn(cls.name, cls.superCtor);

  // destructor
  char const *virt = virtualIfChildren(cls);
  out << "  " << virt << "~" << cls.name << "() {}\n";
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
      out << "  DECL_AST_DOWNCAST(" << c.name << ")\n";
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
void Gen::emitCtorFields(ASTList<CtorArg> const &args)
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


// is this type a use of my ASTList template?
bool isListType(char const *type)
{
  // do a fairly coarse analysis..
  return 0==strncmp(type, "ASTList <", 9);
}


// emit the definition of the constructor itself
void Gen::emitCtorDefn(char const *className, ASTList<CtorArg> const &args)
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
void Gen::emitCommonFuncs(char const *virt)
{
  // declare the one function they all have
  out << "  " << virt << "void debugPrint(ostream &os, int indent) const;\n";
  out << "\n";
}

// emit user-supplied declarations
void Gen::emitUserDecls(ASTList<UserDecl> const &decls)
{
  FOREACH_ASTLIST(UserDecl, decls, decl) {
    out << "  " << toString(decl.data()->access) << ": " << decl.data()->code << "\n";
  }
}

// emit declaration for a specific class instance constructor
void Gen::visitCtor(ASTClass const &parent, ASTCtor const &ctor)
{
  out << "class " << ctor.name << " : public " << parent.name << " {\n";

  emitCtorFields(ctor.args);
  emitCtorDefn(ctor.name, ctor.args);

  // destructor
  out << "  virtual ~" << ctor.name << "() {}\n";
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

  // generated the header
  string base = replace(srcFname, ".ast", "");
  string destFname = base & ".gen.h";
  Gen g(srcFname, destFname);
  g.emitFile(*ast);
}

ARGS_MAIN
