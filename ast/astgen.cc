// astgen.cc            see license.txt for copyright and terms of use
// program to generate C++ code from an AST specification

#include "agrampar.h"      // readAbstractGrammar
#include "test.h"          // ARGS_MAIN
#include "trace.h"         // TRACE_ARGS
#include "owner.h"         // Owner
#include "ckheap.h"        // checkHeap
#include "strutil.h"       // replace, translate, localTimeString
#include "sobjlist.h"      // SObjList
#include "srcloc.h"        // SourceLocManager
#include "strtokp.h"       // StrtokParse

#include <string.h>        // strncmp
#include <fstream.h>       // ofstream
#include <stdlib.h>        // exit
#include <ctype.h>         // isalnum


// this is the name of the visitor interface class, or ""
// if the user does not want a visitor
string visitorName;
inline bool wantVisitor() { return visitorName.length() != 0; }

// list of all TF_classes in the input, useful for certain
// applications which don't care about other forms
SObjList<TF_class> allClasses;

// true if the user wants the xmlPrint stuff
bool wantXMLPrint = false;

// true if the user wants the gdb() functions
bool wantGDB = false;


// ------------------ shared gen functions ----------------------
class Gen {
protected:        // data
  string srcFname;                  // name of source file
  ObjList<string> const &modules;   // extension modules
  string destFname;                 // name of output file
  ofstream out;                     // output stream
  ASTSpecFile const &file;          // AST specification

public:
  Gen(char const *srcFname, ObjList<string> const &modules,
      char const *destFname, ASTSpecFile const &file);
  ~Gen();

  // type queries
  bool isTreeNode(char const *type);
  bool isTreeNodePtr(char const *type);
  string extractNodeType(char const *type);

  bool isListType(char const *type);
  bool isFakeListType(char const *type);
  bool isTreeListType(char const *type);
  string extractListType(char const *type);

  // shared output sequences
  void headerComments();
  void doNotEdit();
  void emitFiltered(ASTList<Annotation> const &decls, AccessCtl mode,
                    char const *indent);
};


Gen::Gen(char const *srcfn, ObjList<string> const &mods,
         char const *destfn, ASTSpecFile const &f)
  : srcFname(srcfn),
    modules(mods),
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
  SFOREACH_OBJLIST(TF_class, allClasses, iter) {
    TF_class const *c = iter.data();

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


// get just the first alphanumeric word
string Gen::extractNodeType(char const *type)
{
  char const *end = type;
  while (isalnum(*end) || *end=='_') {
    end++;
  }
  return string(type, end-type);
}


// is this type a use of my ASTList template?
bool Gen::isListType(char const *type)
{
  // do a fairly coarse analysis.. (the space before "<" is
  // there because the type string is actually parsed by the
  // grammar, and as it assembles it back into a string it
  // inserts a space after every name-like token)
  return 0==strncmp(type, "ASTList <", 9);
}

// similar for FakeList
bool Gen::isFakeListType(char const *type)
{
  return 0==strncmp(type, "FakeList <", 9);
}

// is it a list type, with the elements being tree nodes?
bool Gen::isTreeListType(char const *type)
{
  return isListType(type) && isTreeNode(extractListType(type));
}

// given a type for which 'is[Fake]ListType' returns true, extract
// the type in the template argument angle brackets; this is used
// to get the name of the type so we can pass it to the macros
// which print list contents
string Gen::extractListType(char const *type)
{
  xassert(isListType(type) || isFakeListType(type));
  char const *langle = strchr(type, '<');
  char const *rangle = strchr(type, '>');
  xassert(langle && rangle);
  return trimWhitespace(string(langle+1, rangle-langle-1));
}


// given a string that comes from a user's declaration of a field
// of a class, extract the type and the name; this assumes that,
// syntactically, they separate cleanly (without the name in the
// middle of the type syntax)
void parseFieldDecl(string &type, string &name, char const *decl)
{
  // it's not trivial to extract the name of the field from
  // its declaration.. so let's use a simple heuristic: it's
  // probably the last sequence of non-whitespace alphanum chars
  StrtokParse tok(decl, " \t*()[]<>,");
  
  // now, find the offset of the start of the last token
  int ofs = tok.offset(tok.tokc()-1);
  
  // extract the parts
  type = trimWhitespace(string(decl, ofs));
  name = trimWhitespace(decl+ofs);
}

string extractFieldType(char const *decl)
{
  string t, n;
  parseFieldDecl(t, n, decl);
  return t;
}

string extractFieldName(char const *decl)
{
  string t, n;
  parseFieldDecl(t, n, decl);
  return n;
}


// I scatter this throughout the generated code as pervasive
// reminders that these files shouldn't be edited -- I often
// accidentally edit them and then have to backtrack and reapply
// the changes to where they were supposed to go ..
void Gen::doNotEdit()
{
  out << "// *** DO NOT EDIT ***\n";
}


void Gen::headerComments()
{
  out << "// " << sm_basename(destFname) << "\n";
  doNotEdit();
  out << "// generated automatically by astgen, from " << sm_basename(srcFname) << "\n";
  
  if (modules.count()) {
    out << "// active extension modules:";
    FOREACH_OBJLIST(string, modules, iter) {
      out << " " << *(iter.data());
    }
    out << "\n";
  }

  // the inclusion of the date introduces gratuitous changes when the
  // tool is re-run for whatever reason
  //out << "//   on " << localTimeString() << "\n";
  
  out << "\n";
}


void Gen::emitFiltered(ASTList<Annotation> const &decls, AccessCtl mode,
                       char const *indent)
{
  FOREACH_ASTLIST(Annotation, decls, iter) {
    if (iter.data()->kind() == Annotation::USERDECL) {
      UserDecl const &decl = *( iter.data()->asUserDeclC() );
      if (decl.access() == mode) {
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
  void emitBaseClassDecls(ASTClass const &cls, int ct);
  static char const *virtualIfChildren(TF_class const &cls);
  void emitCtorFields(ASTList<CtorArg> const &args);
  void emitCtorFormal(int &ct, CtorArg const *arg);
  void emitCtorDefn(ASTClass const &cls, ASTClass const *parent);
  void emitCommonFuncs(char const *virt);
  void emitUserDecls(ASTList<Annotation> const &decls);
  void emitCtor(ASTClass const &ctor, ASTClass const &parent);
  void emitVisitorInterface();

public:         // funcs
  HGen(char const *srcFname, ObjList<string> const &modules,
       char const *destFname, ASTSpecFile const &file)
    : Gen(srcFname, modules, destFname, file)
  {}
  void emitFile();
};


// emit header code for an entire AST spec file
void HGen::emitFile()
{
  string includeLatch = translate(sm_basename(destFname), "a-z.", "A-Z_");

  headerComments();

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
  out << "\n\n";

  if (wantVisitor()) {
    out << "// visitor interface class\n"
        << "class " << visitorName << ";\n\n";
  }

  // do all the enums first; this became necessary when I had an
  // enum in an extension, since the use of the enum ended up
  // before the enum itself, due to the use being in a class that
  // was defined in the base module          
  {
    FOREACH_ASTLIST(ToplevelForm, file.forms, form) {
      if (form.data()->isTF_enum()) {
        TF_enum const *e = form.data()->asTF_enumC();
        out << "enum " << e->name << " {\n";
        FOREACH_ASTLIST(string, e->enumerators, iter) {
          out << "  " << *(iter.data()) << ",\n";
        }
        out << "};\n"
            << "\n"
            << "char const *toString(" << e->name << ");\n"
            << "\n"
            << "\n"
            ;
      }
    }
  }

  // process each (non-enum) directive
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
        // ignore other toplevel forms (e.g. TF_option)
        break;
    }

    out << "\n";
  }

  if (wantVisitor()) {
    emitVisitorInterface();
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
  out << "class " << cls.super->name;
  emitBaseClassDecls(*(cls.super), 0 /*ct*/);
  out << " {\n";

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
      out << ctor.data()->classKindName() << ", ";
    }
    out << "NUM_KINDS };\n";

    out << "  virtual Kind kind() const = 0;\n";
    out << "\n";
    out << "  static char const * const kindNames[NUM_KINDS];\n";
    out << "  char const *kindName() const { return kindNames[kind()]; }\n";
    out << "\n";
  }
  else {
    // even if the phylum doesn't have children, it's nice to be able
    // to ask an AST node for its name (e.g. in templatized code that
    // deals with arbitrary AST node types), so define the
    // 'kindName()' method anyway
    out << "  char const *kindName() const { return \"" << cls.super->name << "\"; }\n";
  }

  // declare checked downcast functions
  {
    FOREACH_ASTLIST(ASTClass, cls.ctors, ctor) {
      // declare the const downcast
      ASTClass const &c = *(ctor.data());
      out << "  DECL_AST_DOWNCASTS(" << c.name << ", " << c.classKindName() << ")\n";
    }
    out << "\n";
  }

  // declare clone function
  if (cls.hasChildren()) {
    out << "  virtual " << cls.super->name << " *clone() const=0;\n";
  }
  else {
    out << "  " << cls.super->name << " *clone() const;\n";
  }
  out << "\n";

  emitCommonFuncs(virt);

  if (wantGDB) {
    out << "  void gdb() const;\n\n";
  }

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


void HGen::emitBaseClassDecls(ASTClass const &cls, int ct)
{
  FOREACH_ASTLIST(BaseClass, cls.bases, iter) {
    out << ((ct++ > 0)? ", " : " : ")
        << toString(iter.data()->access) << " "
        << iter.data()->name
        ;
  }
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

  // emit default value, if any  
  if (arg->defaultValue.length() > 0) {
    out << " = " << arg->defaultValue;
  }
}

 
// true if 'ud' seems to declare a function, as opposed to data
bool isFuncDecl(UserDecl const *ud)
{
  return ud->amod->hasMod("virtual") ||
         ud->amod->hasMod("func");
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

      // initialize fields that have initializers
      FOREACH_ASTLIST(Annotation, cls.decls, ann) {
        if (!ann.data()->isUserDecl()) continue;
        UserDecl const *ud = ann.data()->asUserDeclC();
        if (ud->init.length() == 0) continue;
        if (isFuncDecl(ud)) continue;       // don't do this for functions!

        if (ct++ > 0) {
          out << ", ";
        }
        else {
          out << " : ";
        }

        out << extractFieldName(ud->code) << "(" << ud->init << ")";
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
  // declare the functions they all have
  out << "  " << virt << "void debugPrint(ostream &os, int indent, char const *subtreeName = \"tree\") const;\n";
  if (wantXMLPrint) {
    out << "  " << virt << "void xmlPrint(ostream &os, int indent) const;\n";
  }
  
  if (wantVisitor()) {
    // visitor traversal entry point
    out << "  " << virt << "void traverse(" << visitorName << " &vis);\n";
  }

  out << "\n";
}

// emit user-supplied declarations
void HGen::emitUserDecls(ASTList<Annotation> const &decls)
{
  FOREACH_ASTLIST(Annotation, decls, iter) {
    // in the header, we only look at userdecl annotations
    if (iter.data()->kind() == Annotation::USERDECL) {
      UserDecl const &decl = *( iter.data()->asUserDeclC() );
      if (decl.access() == AC_PUBLIC ||
          decl.access() == AC_PRIVATE ||
          decl.access() == AC_PROTECTED) {
        out << "  " << toString(decl.access()) << ": ";

        if (decl.amod->hasMod("virtual")) {
          out << "virtual ";
        }
        out << decl.code;

        if (isFuncDecl(&decl) && decl.init.length() > 0) {
          out << " = " << decl.init;     // the "=0" of a pure virtual function
        }
        out << ";";

        // emit field flags as comments, to help debug astgen
        if (decl.amod->mods.count()) {
          out << "   //";
          FOREACH_ASTLIST(string, decl.amod->mods, mod) {
            out << " " << *(mod.data());
          }
        }
        out << "\n";
      }
      if (decl.access() == AC_PUREVIRT) {
        // this is the parent class: declare it pure virtual
        out << "  public: virtual " << decl.code << "=0;\n";
      }
    }
  }
}

// emit declaration for a specific class instance constructor
void HGen::emitCtor(ASTClass const &ctor, ASTClass const &parent)
{
  out << "class " << ctor.name << " : public " << parent.name;
  emitBaseClassDecls(ctor, 1 /*ct*/);
  out << " {\n";

  emitCtorFields(ctor.args);
  emitCtorDefn(ctor, &parent);

  // destructor
  out << "  virtual ~" << ctor.name << "();\n";
  out << "\n";

  // type tag
  out << "  virtual Kind kind() const { return " << ctor.classKindName() << "; }\n";
  out << "  enum { TYPE_TAG = " << ctor.classKindName() << " };\n";
  out << "\n";

  // common functions
  emitCommonFuncs("virtual ");

  // clone function (take advantage of covariant return types)
  out << "  virtual " << ctor.name << " *clone() const;\n"
      << "\n";

  emitUserDecls(ctor.decls);
  
  // emit implementation declarations for parent's pure virtuals
  FOREACH_ASTLIST(Annotation, parent.decls, iter) {
    UserDecl const *decl = iter.data()->ifUserDeclC();
    if (!decl) continue;
    
    if (decl->access() == AC_PUREVIRT) {
      out << "  public: virtual " << decl->code << ";\n";
    }
    else if (decl->amod->hasMod("virtual")) {
      out << "  " << toString(decl->access()) 
          << ": virtual " << decl->code << ";\n";
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
  CGen(char const *srcFname, ObjList<string> const &modules,
       char const *destFname, ASTSpecFile const &file,
       char const *hdr)
    : Gen(srcFname, modules, destFname, file),
      hdrFname(hdr)
  {}

  void emitFile();
  void emitTFClass(TF_class const &cls);
  void emitDestructor(ASTClass const &cls);
  void emitDestroyField(bool isOwner, char const *type, char const *name);
  void emitPrintCtorArgs(ASTList<CtorArg> const &args);
  void emitPrintFields(ASTList<Annotation> const &decls);
  void emitPrintField(char const *print,
                      bool isOwner, char const *type, char const *name);

  void emitXmlPrintCtorArgs(ASTList<CtorArg> const &args);
  void emitCustomCode(ASTList<Annotation> const &list, char const *tag);

  void emitCloneCtorArg(CtorArg const *arg, int &ct);
  void emitCloneCode(ASTClass const *super, ASTClass const *sub);

  void emitVisitorImplementation();
  void emitTraverse(ASTClass const *c, ASTClass const * /*nullable*/ super,
                    bool hasChildren);
};



void CGen::emitFile()
{
  headerComments();

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
      ASTNEXTC(TF_enum, e) {
        out << "char const *toString(" << e->name << " x)\n"
            << "{\n"
            << "  static char const * const map[] = {\n";
        FOREACH_ASTLIST(string, e->enumerators, iter) {
          out << "    \"" << *(iter.data()) << "\",\n";
        }     
        out << "  };\n"
            << "  xassert((unsigned)x < TABLESIZE(map));\n"
            << "  return map[x];\n"
            << "};\n"
            << "\n"
            << "\n"
            ;
        break;
      }
      ASTENDCASECD
    }
  }
  out << "\n\n";

  if (wantVisitor()) {
    emitVisitorImplementation();
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
  out << "void " << cls.super->name << "::debugPrint(ostream &os, int indent, char const *subtreeName) const\n";
  out << "{\n";
  if (!cls.hasChildren()) {
    // childless superclasses get the preempt in the superclass;
    // otherwise it goes into the child classes
    emitCustomCode(cls.super->decls, "preemptDebugPrint");

    // childless superclasses print headers; otherwise the subclass
    // prints the header
    out << "  PRINT_HEADER(subtreeName, " << cls.super->name << ");\n";
    out << "\n";
  }

  // 10/31/01: decided I wanted custom debug print first, since it's
  // often much shorter (and more important) than the subtrees
  emitCustomCode(cls.super->decls, "debugPrint");
  emitPrintCtorArgs(cls.super->args);
  emitPrintFields(cls.super->decls);

  out << "}\n";
  out << "\n";

  // gdb()
  if (wantGDB) {
    out << "void " << cls.super->name << "::gdb() const\n"
        << "  { debugPrint(cout, 0); }\n"
        << "\n"
        ;
  }

  // dsw: xmlPrint
  if (wantXMLPrint) {
    out << "void " << cls.super->name << "::xmlPrint(ostream &os, int indent) const\n";
    out << "{\n";
    if (!cls.hasChildren()) {
      // dsw: Haven't figured out what this subsection means yet.
//        // childless superclasses get the preempt in the superclass;
//        // otherwise it goes into the child classes
//        emitCustomCode(cls.super->decls, "preemptDebugPrint");

//        // childless superclasses print headers; otherwise the subclass
//        // prints the header
      out << "  XMLPRINT_HEADER(" << cls.super->name << ");\n";
      out << "\n";
    }

    // dsw: This must be in case the client .ast code overrides it.
    // 10/31/01: decided I wanted custom debug print first, since it's
    // often much shorter (and more important) than the subtrees
//      emitCustomCode(cls.super->decls, "xmlPrint");
    emitXmlPrintCtorArgs(cls.super->args);

    if (!cls.hasChildren()) {
      out << "  XMLPRINT_FOOTER(" << cls.super->name << ");\n";
    }
    out << "}\n";
    out << "\n";
  }


  // clone for childless superclasses
  if (!cls.hasChildren()) {
    emitCloneCode(cls.super, NULL /*sub*/);
  }


  // constructors (class hierarchy children)
  FOREACH_ASTLIST(ASTClass, cls.ctors, ctoriter) {
    ASTClass const &ctor = *(ctoriter.data());

    // downcast function
    out << "DEFN_AST_DOWNCASTS(" << cls.super->name << ", "
                                 << ctor.name << ", "
                                 << ctor.classKindName() << ")\n";
    out << "\n";

    // subclass destructor
    emitDestructor(ctor);

    // subclass debugPrint
    out << "void " << ctor.name << "::debugPrint(ostream &os, int indent, char const *subtreeName) const\n";
    out << "{\n";

    // the debug print preempter is declared in the outer "class",
    // but inserted into the print methods of the inner "constructors"
    emitCustomCode(cls.super->decls, "preemptDebugPrint");

    out << "  PRINT_HEADER(subtreeName, " << ctor.name << ");\n";
    out << "\n";

    // call the superclass's fn to get its data members
    out << "  " << cls.super->name << "::debugPrint(os, indent, subtreeName);\n";
    out << "\n";

    emitCustomCode(ctor.decls, "debugPrint");
    emitPrintCtorArgs(ctor.args);
    emitPrintFields(ctor.decls);

    out << "}\n";
    out << "\n";

    if (wantXMLPrint) {
      // subclass xmlPrint
      out << "void " << ctor.name << "::xmlPrint(ostream &os, int indent) const\n";
      out << "{\n";

      // the xml print preempter is declared in the outer "class",
      // but inserted into the print methods of the inner "constructors"
      emitCustomCode(cls.super->decls, "preemptXmlPrint");

      out << "  XMLPRINT_HEADER(" << ctor.name << ");\n";
      out << "\n";

      // call the superclass's fn to get its data members
      out << "  " << cls.super->name << "::xmlPrint(os, indent);\n";
      out << "\n";

      // dsw: I assume this is not vital in the generic case, so I
      // leave it out for now.
//        emitCustomCode(ctor.decls, "xmlPrint");
      emitXmlPrintCtorArgs(ctor.args);

      // dsw: probably don't need the name; take it out later if not
      out << "  XMLPRINT_FOOTER(" << ctor.name << ");\n";
      out << "\n";

      out << "}\n";
      out << "\n";
    }

    // clone for subclasses
    emitCloneCode(cls.super, &ctor);
  }

  out << "\n";
}


void CGen::emitCustomCode(ASTList<Annotation> const &list, char const *tag)
{
  FOREACH_ASTLIST(Annotation, list, iter) {
    CustomCode const *cc = iter.data()->ifCustomCodeC();
    if (cc && cc->qualifier.equals(tag)) {
      out << "  " << cc->code << ";\n";

      // conceptually mutable..
      const_cast<bool&>(cc->used) = true;
    }
  }
}


void CGen::emitDestructor(ASTClass const &cls)
{
  out << cls.name << "::~" << cls.name << "()\n";
  out << "{\n";

  // user's code first
  emitFiltered(cls.decls, AC_DTOR, "  ");
  
  // constructor arguments
  FOREACH_ASTLIST(CtorArg, cls.args, argiter) {
    CtorArg const &arg = *(argiter.data());
    emitDestroyField(arg.isOwner, arg.type, arg.name);
  }
                          
  // owner fields
  FOREACH_ASTLIST(Annotation, cls.decls, iter) {
    if (!iter.data()->isUserDecl()) continue;
    UserDecl const *ud = iter.data()->asUserDeclC();
    if (!ud->amod->hasMod("owner")) continue;

    emitDestroyField(true /*isOwner*/,
                     extractFieldType(ud->code),
                     extractFieldName(ud->code));
  }

  out << "}\n";
  out << "\n";
}

void CGen::emitDestroyField(bool isOwner, char const *type, char const *name)
{
  if (isTreeListType(type)) {
    // explicitly destroy list elements, because it's easy to do, and
    // because if there is a problem, it's much easier to see its
    // role in a debugger backtrace
    out << "  " << name << ".deleteAll();\n";
  }
  else if (isListType(type)) {
    if (0==strcmp(extractListType(type), "LocString")) {
      // these are owned even though they aren't actually tree nodes
      out << "  " << name << ".deleteAll();\n";

      // TODO: this analysis is duplicated below, during cloning;
      // the astgen tool should do a better job of encapsulating
      // the relationships (particularly owning/non-owning) between
      // its parts, instead of doing ad-hoc type inspection in random
      // places during emission
    }
    else {
      // we don't own the list elements; it's *essential* to
      // explicitly remove the elements; this is a hack, since the
      // ideal solution is to make a variant of ASTList which is
      // explicitly serf pointers.. the real ASTList doesn't have
      // a removeAll method (since it's an owner list), and rather
      // than corrupting that interface I'll emit the code each time..
      out << "  while (" << name << ".isNotEmpty()) {\n"
          << "    " << name << ".removeFirst();\n"
          << "  }\n";
      //out << "  " << name << ".removeAll();\n";
    }
  }
  else if (isOwner || isTreeNode(type)) {
    out << "  delete " << name << ";\n";
  }
}


void CGen::emitPrintCtorArgs(ASTList<CtorArg> const &args)
{
  FOREACH_ASTLIST(CtorArg, args, argiter) {
    CtorArg const &arg = *(argiter.data());

    emitPrintField("PRINT", arg.isOwner, arg.type, arg.name);
  }
}

void CGen::emitPrintFields(ASTList<Annotation> const &decls)
{
  FOREACH_ASTLIST(Annotation, decls, iter) {
    if (!iter.data()->isUserDecl()) continue;
    UserDecl const *ud = iter.data()->asUserDeclC();
    if (!ud->amod->hasMod("field")) continue;

    emitPrintField("PRINT",
                   ud->amod->hasMod("owner"),
                   extractFieldType(ud->code),
                   extractFieldName(ud->code));
  }
}

void CGen::emitPrintField(char const *print,
                          bool isOwner, char const *type, char const *name)
{
  if (0==strcmp(type, "string")) {
    out << "  " << print << "_STRING(" << name << ");\n";
  }
  else if (isListType(type)) {
    // for now, I'll continue to assume that any class that appears
    // in ASTList<> is compatible with the printing regime here
    out << "  " << print << "_LIST(" << extractListType(type) << ", "
        << name << ");\n";
  }
  else if (isFakeListType(type)) {
    // similar printing approach for FakeLists
    out << "  " << print << "_FAKE_LIST(" << extractListType(type) << ", "
        << name << ");\n";
  }
  else if (isTreeNode(type) ||
           (isTreeNodePtr(type) && isOwner)) {
    // don't print subtrees that are possibly shared or circular
    out << "  " << print << "_SUBTREE(" << name << ");\n";
  }
  else if (0==strcmp(type, "bool")) {
    out << "  " << print << "_BOOL(" << name << ");\n";
  }
  else {
    // catch-all ..
    out << "  " << print << "_GENERIC(" << name << ");\n";
  }
}


void CGen::emitXmlPrintCtorArgs(ASTList<CtorArg> const &args)
{
  FOREACH_ASTLIST(CtorArg, args, argiter) {
    CtorArg const &arg = *(argiter.data());
    
    emitPrintField("XMLPRINT", arg.isOwner, arg.type, arg.name);
  }
}


void CGen::emitCloneCtorArg(CtorArg const *arg, int &ct)
{
  if (ct++ > 0) {
    out << ",";
  }
  out << "\n    ";

  if (isTreeListType(arg->type)) {
    // clone an ASTList of tree nodes
    out << "cloneASTList(" << arg->name << ")";
  }
  else if (isListType(arg->type)) {
    if (0==strcmp(extractListType(arg->type), "LocString")) {
      // these are owned, so clone deeply
      out << "cloneASTList(" << arg->name << ")";
    }
    else {
      // clone an ASTList of non-tree nodes
      out << "shallowCloneASTList(" << arg->name << ")";
    }
  }
  else if (isFakeListType(arg->type)) {
    // clone a FakeList (of tree nodes, we assume..)
    out << "cloneFakeList(" << arg->name << ")";
  }
  else if (isTreeNode(arg->type)) {
    // clone a tree node
    out << arg->name << "? " << arg->name << "->clone() : NULL";
  }
  else if (0==strcmp(arg->type, "LocString")) {
    // clone a LocString; we store objects, but pass pointers
    out << arg->name << ".clone()";
  }
  else {
    // pass the non-tree node's value directly
    out << arg->name;
  }
}


void CGen::emitCloneCode(ASTClass const *super, ASTClass const *sub)
{
  char const *name = sub? sub->name : super->name;
  out << name << " *" << name << "::clone() const\n"
      << "{\n"
      << "  " << name << " *ret = new " << name << "(";

  // clone each of the superclass ctor arguments
  int ct=0;
  FOREACH_ASTLIST(CtorArg, super->args, iter) {
    emitCloneCtorArg(iter.data(), ct);
  }

  // and likewise for the subclass ctor arguments
  if (sub) {
    FOREACH_ASTLIST(CtorArg, sub->args, iter) {
      emitCloneCtorArg(iter.data(), ct);
    }
  }

  out << "\n"
      << "  );\n";

  // custom clone code
  emitCustomCode(super->decls, "clone");
  if (sub) {
    emitCustomCode(sub->decls, "clone");
  }

  out << "  return ret;\n"
      << "}\n"
      << "\n";
}


// -------------------------- visitor ---------------------------
void HGen::emitVisitorInterface()
{
  out << "// the visitor interface class\n"
      << "class " << visitorName << " {\n"
      << "public:\n";
  SFOREACH_OBJLIST(TF_class, allClasses, iter) {
    TF_class const *c = iter.data();

    out << "  virtual bool visit" << c->super->name << "("
        <<   c->super->name << " *obj);\n"
        << "  virtual void postvisit" << c->super->name << "("
        <<   c->super->name << " *obj);\n";
  }
  out << "};\n\n";
}


void CGen::emitVisitorImplementation()
{
  out << "// default no-op visitor\n";
  SFOREACH_OBJLIST(TF_class, allClasses, iter) {
    TF_class const *c = iter.data();

    out << "bool " << visitorName << "::visit" << c->super->name << "("
        <<   c->super->name << " *obj) { return true; }\n"
        << "void " << visitorName << "::postvisit" << c->super->name << "("
        <<   c->super->name << " *obj) {}\n";
  }
  out << "\n\n";

  // implementations of traversal functions
  SFOREACH_OBJLIST(TF_class, allClasses, iter) {
    TF_class const *c = iter.data();

    // superclass traversal
    emitTraverse(c->super, NULL /*super*/, c->hasChildren());

    // subclass traversal
    FOREACH_ASTLIST(ASTClass, c->ctors, iter) {
      ASTClass const *sub = iter.data();

      emitTraverse(sub, c->super, false /*hasChildren*/);
    }
  }
}


void CGen::emitTraverse(ASTClass const *c, ASTClass const * /*nullable*/ super,
                        bool hasChildren)
{
  out << "void " << c->name << "::traverse("
      <<   visitorName << " &vis)\n"
      << "{\n";

  // name of the 'visit' method that applies to this class;
  // these methods are always named according to the least-derived
  // class in the hierarchy
  string visitName = stringc << "visit" << (super? super : c)->name;

  // we only call 'visit' in the most-derived classes; this of course
  // assumes that classes with children are never themselves instantiated
  if (!hasChildren) {
    // visit this node; if the visitor doesn't want to traverse
    // the children, then bail
    out << "  if (!vis." << visitName << "(this)) { return; }\n\n";
  }
  else {
    out << "  // no 'visit' because it's handled by subclasses\n\n";
  }

  if (super) {
    // traverse superclass ctor args, if this class has one
    out << "  " << super->name << "::traverse(vis);\n"
        << "\n";
  }

  // traverse into the ctor arguments
  FOREACH_ASTLIST(CtorArg, c->args, iter) {
    CtorArg const *arg = iter.data();

    if (isTreeNode(arg->type) || isTreeNodePtr(arg->type)) {
      // traverse it directly
      string type = extractNodeType(arg->type);
      out << "  if (" << arg->name << ") { " << arg->name << "->traverse(vis); }\n";
    }

    else if ((isListType(arg->type) || isFakeListType(arg->type)) &&
             isTreeNode(extractListType(arg->type))) {
      // list of tree nodes: iterate and traverse
      string eltType = extractListType(arg->type);

      // compute list accessor names
      char const *iterMacroName = "FOREACH_ASTLIST_NC";
      char const *iterElt = ".data()";
      if (isFakeListType(arg->type)) {
        iterMacroName = "FAKELIST_FOREACH_NC";
        iterElt = "";
      }

      out << "  " << iterMacroName << "("
          <<   eltType << ", " << arg->name << ", iter) {\n"
          << "    iter" << iterElt << "->traverse(vis);\n"
          << "  }\n";
    }
  }

  // do any additional traversal action specified by the user
  emitCustomCode(c->decls, "traverse");

  if (!hasChildren) {
    // if we did the preorder visit on the way in, do the
    // postorder visit now
    out << "  vis.post" << visitName << "(this);\n";
  }
  else {
    out << "  // no 'postvisit' either\n";
  }

  out << "}\n\n";
}


// -------------------- extension merging ------------------
// the 'allClasses' list is filled in after merging, so I
// can't use it in this section
#define allClasses NO!

void mergeClass(ASTClass *base, ASTClass *ext)
{
  xassert(base->name.equals(ext->name));
  trace("merge") << "merging class: " << ext->name << endl;

  // move all ctor args to the base
  while (ext->args.isNotEmpty()) {
    base->args.append(ext->args.removeFirst());
  }

  // and same for annotations
  while (ext->decls.isNotEmpty()) {
    base->decls.append(ext->decls.removeFirst());
  }
}


void mergeEnum(TF_enum *base, TF_enum *ext)
{
  xassert(base->name.equals(ext->name));
  trace("merge") << "merging enum: " << ext->name << endl;

  while (ext->enumerators.isNotEmpty()) {
    base->enumerators.append(ext->enumerators.removeFirst());
  }
}


ASTClass *findClass(TF_class *base, char const *name)
{
  FOREACH_ASTLIST_NC(ASTClass, base->ctors, iter) {
    if (iter.data()->name.equals(name)) {
      return iter.data();
    }
  }
  return NULL;   // not found
}

void mergeSuperclass(TF_class *base, TF_class *ext)
{
  // should only get here for same-named classes
  xassert(base->super->name.equals(ext->super->name));
  trace("merge") << "merging superclass: " << ext->super->name << endl;

  // merge the superclass ctor args and annotations
  mergeClass(base->super, ext->super);

  // for each subclass, either add it or merge it
  while (ext->ctors.isNotEmpty()) {
    ASTClass * /*owner*/ c = ext->ctors.removeFirst();

    ASTClass *prev = findClass(base, c->name);
    if (prev) {
      mergeClass(prev, c);
      delete c;
    }
    else {
      // add it wholesale
      trace("merge") << "adding subclass: " << c->name << endl;
      base->ctors.append(c);
    }
  }
}


TF_class *findSuperclass(ASTSpecFile *base, char const *name)
{
  // I can *not* simply iterate over 'allClasses', because that
  // list is created *after* merging!
  FOREACH_ASTLIST_NC(ToplevelForm, base->forms, iter) {
    ToplevelForm *tf = iter.data();
    if (tf->isTF_class() &&
        tf->asTF_class()->super->name.equals(name)) {
      return tf->asTF_class();
    }
  }
  return NULL;    // not found
}

TF_enum *findEnum(ASTSpecFile *base, char const *name)
{
  FOREACH_ASTLIST_NC(ToplevelForm, base->forms, iter) {
    ToplevelForm *tf = iter.data();
    if (tf->isTF_enum() &&
        tf->asTF_enum()->name.equals(name)) {
      return tf->asTF_enum();
    }
  }
  return NULL;    // not found
}

void mergeExtension(ASTSpecFile *base, ASTSpecFile *ext)
{
  // for each toplevel form, either add it or merge it
  int ct = 0;
  while (ext->forms.isNotEmpty()) {
    ToplevelForm * /*owner*/ tf = ext->forms.removeFirst();

    if (tf->isTF_class()) {
      TF_class *c = tf->asTF_class();

      // is the superclass name something present in the
      // base specification?
      TF_class *prev = findSuperclass(base, c->super->name);
      if (prev) {
        mergeSuperclass(prev, c);
        delete c;
      }
      else {
        // add the whole class
        trace("merge") << "adding new superclass: " << c->super->name << endl;
        base->forms.append(c);
      }
    }

    else if (tf->isTF_enum()) {
      TF_enum *e = tf->asTF_enum();

      // is the enum present in the base?
      TF_enum *prev = findEnum(base, e->name);
      if (prev) {
        mergeEnum(prev, e);
        delete e;
      }
      else {
        // add the whole enum
        trace("merge") << "adding new enum: " << e->name << endl;
        base->forms.append(e);
      }
    }

    else {
      // verbatim or option: just add it directly

      if (ct == 0) {
        // *first* verbatim: goes into a special place in the
        // base, before any classes but after any base verbatims
        int i;
        for (i=0; i < base->forms.count(); i++) {
          ToplevelForm *baseForm = base->forms.nth(i);

          if (baseForm->isTF_class()) {
            // ok, this is the first class, so stop here
            // and we'll insert at 'i', thus inserting
            // just before this class
            break;
          }
        }

        // insert the base so it becomes position 'i'
        trace("merge") << "inserting extension verbatim near top\n";
        base->forms.insertAt(tf, i);
      }

      else {
        // normal processing: append everything
        trace("merge") << "appending extension verbatim/option section\n";
        base->forms.append(tf);
      }
    }

    ct++;
  }
}

// re-enable allClasses
#undef allClasses


// --------------------- toplevel control ----------------------
void checkUnusedCustoms(ASTClass const *c)
{
  FOREACH_ASTLIST(Annotation, c->decls, iter) {
    Annotation const *a = iter.data();
    
    if (a->isCustomCode()) {
      CustomCode const *cc = a->asCustomCodeC();
      if (cc->used == false) {
        cout << "warning: unused custom code `" << cc->qualifier << "'\n";
      }
    }
  }
}


void entry(int argc, char **argv)
{
  TRACE_ARGS();
  checkHeap();
  SourceLocManager mgr;

  if (argc < 2) {
    cout << "usage: " << argv[0] << " [options] file.ast [extension.ast [...]]\n"
         << "  options:\n"
         << "    -o<name>   output filenames are name.{h,cc}\n"
         << "               (default is \"file\" for \"file.ast\")\n"
         << "    -v         verbose operation, particularly for merging\n"
         ;

    return;
  }

  char const *basename = NULL;      // nothing set

  argv++;
  while (argv[0][0] == '-') {
    if (argv[0][1] == 'b' ||        // 'b' is for compatibility
        argv[0][1] == 'o') {
      if (argv[0][2]) {
        // name specified directly after the -o
        basename = argv[0]+2;
      }
      else if (argv[1]) {
        // name follows the -o option
        basename = argv[1];
        argv++;
      }
    }
    else if (argv[0][1] == 'v') {
      traceAddSys("merge");
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
  argv++;

  // parse the grammar spec
  Owner<ASTSpecFile> ast;
  ast = readAbstractGrammar(srcFname);

  // parse and merge extension modules
  ObjList<string> modules;
  while (*argv) {
    char const *fname = *argv;
    argv++;
    
    modules.append(new string(fname));

    Owner<ASTSpecFile> extension;
    extension = readAbstractGrammar(fname);

    mergeExtension(ast, extension);
  }

  // scan options, and fill 'allClasses'
  {
    FOREACH_ASTLIST_NC(ToplevelForm, ast->forms, iter) {
      if (iter.data()->isTF_option()) {
        TF_option const *op = iter.data()->asTF_optionC();

        if (op->name.equals("visitor")) {
          if (op->args.count() != 1) {
            cout << "'visitor' option requires one argument\n";
            exit(2);
          }

          if (visitorName.length() > 0) {
            // It would be conceivable to allow multiple visitors, but
            // I don't see any advantage to doing so.  If the extension
            // simply changes the name, then the resulting error messages
            // (compilation errors from parts of the system using the
            // old name) are not obvious to diagnose.
            cout << "error: there is already a visitor class, called "
                 << visitorName << "\n";
            cout << "you should can use (subclass) that one\n";
            exit(2);
          }

          // name of the visitor interface class
          visitorName = *( op->args.firstC() );
        }
        else if (op->name.equals("xmlPrint")) {
          wantXMLPrint = true;
        }
        else if (op->name.equals("gdb")) {
          wantGDB = true;
        }
        else {
          cout << "unknown option: " << op->name << endl;
          exit(2);
        }
      }

      else if (iter.data()->isTF_class()) {
        allClasses.prepend(iter.data()->asTF_class());
      }
    }
  }

  // I built it in reverse for O(n) performance
  allClasses.reverse();


  // generate the header
  string base = replace(srcFname, ".ast", "");
  if (basename) {
    base = basename;
  }

  string hdrFname = base & ".h";
  cout << "writing " << hdrFname << "...\n";
  HGen hg(srcFname, modules, hdrFname, *ast);
  hg.emitFile();

  // generated the c++ code
  string codeFname = base & ".cc";
  cout << "writing " << codeFname << "...\n";
  CGen cg(srcFname, modules, codeFname, *ast, hdrFname);
  cg.emitFile();
  
  
  // check for any unused 'custom' sections; a given section is only
  // used if one of the generation routines asks for it by name, so
  // a mistyped custom section name would not yet have been noticed
  {
    SFOREACH_OBJLIST(TF_class, allClasses, iter) {
      TF_class const *c = iter.data();
      checkUnusedCustoms(c->super);
      
      FOREACH_ASTLIST(ASTClass, c->ctors, subIter) {
        checkUnusedCustoms(subIter.data());
      }
    }
  }
}

ARGS_MAIN
