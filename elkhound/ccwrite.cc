// ccwrite.cc
// code for ccwrite.h

#include "ccwrite.h"      // this module
#include "gramanl.h"      // GrammarAnalysis, Grammar, etc.
#include "strutil.h"      // quoted
#include "emitcode.h"     // EmitCode

#include <string.h>       // strstr
#include <fstream.h>      // ofstream
#include <ctype.h>        // toupper


// prototypes
void emitDisambiguationFun(EmitCode &os, Nonterminal const *nonterm,
                           char const *name, LiteralCode const &decl);


// ------------- name constructors -------------
// return the name of the type created for storing
// parse tree nodes for 'sym'
string nodeTypeName(Symbol const *sym)
{
  return stringc << sym->name << "_Node";
}


// name of the node type ctor function
string nodeCtorName(Symbol const *sym)
{
  return stringc << "make_" << nodeTypeName(sym);
}


// return a declaration but with some characters
// prepended to the name part of 'decl'; 'name' is the
// name that is somewhere in 'decl'
string prefixNameInDecl(char const *prefix, char const *name, char const *decl)
{
  // find where the name appears in 'decl'; assumes that
  // first such occurrance is the right place....
  char const *nameInDecl = strstr(decl, name);
  xassert(nameInDecl);

  return stringc
    << string(decl, nameInDecl-decl)    // first part
    << prefix                           // prepend some name stuff
    << nameInDecl;                      // second part
}


// return a declaration of a semantic function:
//   decl: declaration as appeared in the grammar file
//   name: extracted function name from 'decl'
//   nonterm: symbol the function will be associated with
//   unamb: if true, make this the "unambiguous" version
string semFuncDecl(char const *name, char const *decl,
                   Nonterminal const *nonterm, bool unamb)
{
  stringBuilder sb;
  sb << nodeTypeName(nonterm) << "::";     // class qualifier
  if (unamb) {
    sb << "unamb_";                        // unamb tag
  }

  return prefixNameInDecl(sb, name, decl);
}


// foo.h -> FOO_H
string headerFileLatch(char const *fname)
{
  string ret = fname;
  ret = replace(ret, ".", "_");
  loopi(ret.length()) {
    ret[i] = toupper(ret[i]);
  }
  return ret;
}


// note that #line must be preceeded by a newline

string lineDirective(SourceLocation const &loc)
{
  return stringc << "#line " << loc.line
                 << " \"" << loc.file->filename << "\"\n";
}

stringBuilder &restoreLine(stringBuilder &sb)
{                              
  // little hack..
  EmitCode &os = (EmitCode&)sb;

  // +1 because we specify what line will be *next*
  int line = os.getLine()+1;
  return os << "#line " << line
            << " \"" << os.getFname() << "\"\n";
}


// ----------------- code emitters -----------------
// arguments to node constructors, specified here to
// collapse redundancy
char const * const nodeCtorArgs =
  "(Reduction *red, ParseTree &tree)";


string getTraceCode(char const *functionDecl)
{
  return stringc
    << "  if (tracingSys(\"sem-fns\")) {\n"
    << "    cout << locString() << \": in " << functionDecl << "\\n\";\n"
    << "  }\n";
}


void insertLiteralCode(EmitCode &os, LiteralCode const &code)
{
  os << lineDirective(code)
     << code << "\n"
     << restoreLine
     ;
}


// emit the C++ code for a nonterminal's semantic functions
void emitSemFuns(EmitCode &os, Grammar const *g,
                 Nonterminal const *nonterm)
{
  // comment to separate types
  os << "// ------------- " << nonterm->name << " -------------\n";

  // emit the node type ctor
  string typeName = nodeTypeName(nonterm);
  os << "NonterminalNode *" << nodeCtorName(nonterm) << nodeCtorArgs << "\n"
     << "{\n"
     << "  return new " << typeName << "(red, tree);\n"
     << "}\n"
     << "\n"
     << typeName << "::" << typeName << nodeCtorArgs << "\n"
     << "  : " << g->treeNodeBaseClass << "(red)\n"
     << "{\n"
     ;

  if (nonterm->constructor) {
    insertLiteralCode(os, nonterm->constructor);
  }

  os << "}\n"
     << "\n"
     ;

  // emit destructor
  os << typeName << "::~" << typeName << "()\n"
     << "{\n";
  if (nonterm->destructor) {
    insertLiteralCode(os, nonterm->destructor);
  }
  //os << "  checkHeap();\n";    // DEBUG
  os << "}\n\n";

  os << "\n";


  // loop over all declared functions
  for (LitCodeDict::Iter declaration(nonterm->funDecls);
       !declaration.isDone(); declaration.next()) {
    string name = declaration.key();
    LiteralCode const &decl = *(declaration.value());

    // if there is a disambiguation function, emit it
    bool hasDisamb = nonterm->disambFuns.isMapped(name);
    if (hasDisamb) {
      emitDisambiguationFun(os, nonterm, name, decl);
    }

    // function name and arguments
    string functionDecl = semFuncDecl(name, decl, nonterm, hasDisamb);

    // write the function prologue for the unambiguous version
    // (or the only version, if there is no disambiguator)
    os << lineDirective(decl)
       << functionDecl << "\n"
       << restoreLine
       << "{\n"
       <<    getTraceCode(functionDecl)
       ;
                                              
    // prefix code
    if (nonterm->funPrefixes.isMapped(name)) {
      insertLiteralCode(os, *nonterm->funPrefixes.queryfC(name));
    }

    // start of switch block
    os << "  switch (onlyProductionIndex()) {\n"
       << "    default:\n"
       << "      xfailure(\"bad production code\");\n"
       << "\n"
       ;

    // loop over all productions for this nonterminal
    FOREACH_OBJLIST(Production, g->productions, prodIter) {
      Production const *prod = prodIter.data();
      if (prod->left != nonterm) { continue; }

      LiteralCode const &body = *(prod->functions.queryfC(name));

      // write the header for this production's action
      xassert(prod->prodIndex != -1);    // otherwise we forgot to set it
      os << "    case " << prod->prodIndex
           << ": {       // " << prod->toString() << "\n";

      // for each RHS element with a tag
      int childNum=0;
      SObjListIter<Symbol> symIter(prod->right);
      ObjListIter<string> tagIter(prod->rightTags);
      for (; !symIter.isDone();
           symIter.adv(), tagIter.adv(), childNum++) {
        Symbol const *sym = symIter.data();
        string tag = *(tagIter.data());
        if (tag.length() == 0) { continue; }    // no tag

        // each RHS element with a tag needs that tag bound
        if (sym->isNonterminal()) {
          // e.g.: B_Node const &b = *((B_Node const *)getOnlyChild(0));
          os << "      " << nodeTypeName(sym) << " &"
             << tag << " = *((" << nodeTypeName(sym)
             << " *)getOnlyChild(" << childNum
             << "));\n";
        }
        else {
          // e.g.: Lexer2Token const &n = getOnlyChildToken(1);
          os << "      Lexer2Token const &"
             << tag << " = getOnlyChildToken(" << childNum
             << ");\n";
        }
        
        // and make sure we don't get unused-var warnings; we generate
        // a binding for every tagged var, without regard to whether that
        // tag is actually used in the semantic function, since it's too
        // hard to find out whether tags are used
        os << "      PRETEND_USED(" << tag << ");\n";
      }

      // write the user's code
      os << "\n";
      insertLiteralCode(os, body);
      os << "      break;\n"    // catch missing returns, or for void fns
         << "    }\n"
         << "\n";

    } // end of loop over productions

    // write function epilogue; if it's a non-void function,
    // rely on compiler warnings to catch missing 'return's
    os << "  }\n"
          "}\n\n\n"
          ;

  } // end of loop over declared functions in the nonterminal
}


void emitDisambiguationFun(EmitCode &os, Nonterminal const *nonterm, 
                           char const *name, LiteralCode const &decl)
{
  // user's code
  LiteralCode const &code = *(nonterm->disambFuns.queryfC(name));

  // full function name and arguments
  string functionDecl = semFuncDecl(name, decl, nonterm, false /*unamb*/);

  // emit whole thing
  os << lineDirective(decl)
     << functionDecl << "\n"
     << restoreLine
     << "{\n"
     <<    getTraceCode(functionDecl);
  insertLiteralCode(os, code);
  os << "}\n"
     << "\n\n"
     ;
}


// emit the C++ declaration for a nonterminal's node type class
void emitClassDecl(EmitCode &os, Grammar const *g, Nonterminal const *nonterm)
{
  // type declaration prologue
  os << "class " << nodeTypeName(nonterm)
       << " : public " << g->treeNodeBaseClass << " {\n"
     << "public:\n"
     ;
   
  // constructor
  os << "  " << nodeTypeName(nonterm) << nodeCtorArgs << ";\n";

  // destructor
  os << "  virtual ~" << nodeTypeName(nonterm) << "();\n";

  // loop over all declared functions
  for (LitCodeDict::Iter declaration(nonterm->funDecls);
       !declaration.isDone(); declaration.next()) {
    string name = declaration.key();
    LiteralCode const &decl = *(declaration.value());

    // emit declaration
    os << lineDirective(decl)
       << "  " << decl << ";    // " << name << "\n"
       << restoreLine
       ;

    // if there is a disambiguator, the above declaration is
    // for it; so we need one for the unamb version
    if (nonterm->disambFuns.isMapped(name)) {
      os << lineDirective(decl)
         << "  " << prefixNameInDecl("unamb_", name, decl) << ";\n"
         << restoreLine
         ;
    }
  }

  // loop over other declarations
  FOREACH_OBJLIST(LiteralCode, nonterm->declarations, iter) {
    insertLiteralCode(os, *(iter.data()));
  }

  // type declaration epilogue
  os << "};\n"
     << "\n";
}


// emit the array of info about each nonterminal
void GrammarAnalysis::emitTypeCtorMap(EmitCode &os) const
{
  // prologue
  os << "// ---------- nonterm info map ----------\n";
  os << "// this is put into the map so...\n"
        "NonterminalNode *make_empty_Node" << nodeCtorArgs << "\n"
        "{\n"
        "   xfailure(\"can't call make_empty_node!\");\n"
        "   return NULL;    // silence warning\n"
        "}\n"
        "\n";
  os << "NonterminalInfo nontermMap[] = {\n";

  // loop over nonterminals
  for (int ntIndex=0; ntIndex < numNonterms; ntIndex++) {
    Nonterminal const *nt = indexedNonterms[ntIndex];

    // map entry: node ctor, nonterm name, commented index
    os << "  { " << nodeCtorName(nt)
       << ", " << quoted(nt->name)
       << " },   // " << ntIndex << "\n";
  }

  // epilogue
  os << "};\n"
     << "\n"
     << "int nontermMapLength = " << numNonterms << ";\n"
     << "\n";
}


// emit the complete C++ file of semantic functions in 'g'
void emitSemFunImplFile(char const *fname, char const *headerFname,
                        GrammarAnalysis const *g)
{
  EmitCode os(fname);

  // file header
  os << "// " << fname << "\n"
     << "// semantic functions for a grammar\n"
     << "// NOTE: automatically generated file -- editing inadvisable\n"
     << "\n"
     << "#include \"" << headerFname << "\"   // user's declarations\n"
     //<< "#include \"ckheap.h\"          // checkHeap\n"
     << "\n"
     << "\n"
     ;

  // semantic functions
  FOREACH_OBJLIST(Nonterminal, g->nonterminals, iter) {
    emitSemFuns(os, g, iter.data());
  }

  // type map
  g->emitTypeCtorMap(os);

  if (g->semanticsEpilogue != NULL) {
    os << "// -------- user-supplied semantics epilogue --------\n";
    insertLiteralCode(os, g->semanticsEpilogue);
    os << "\n"
       << "\n"
       << "// end of " << fname << "\n";
  }
}


// emit the file of C++ node type declarations
void emitSemFunDeclFile(char const *fname, GrammarAnalysis const *g)
{
  EmitCode os(fname);

  // file header
  os << "// " << fname << "\n"
     << "// declarations for tree node types\n"
     << "// NOTE: automatically generated file -- editing inadvisable\n"
     << "\n"
     << "#ifndef " << headerFileLatch(fname) << "\n"
     << "#define " << headerFileLatch(fname) << "\n"
     << "\n"
     << "#include \"glrtree.h\"     // NonterminalNode\n"
     << "#include \"parssppt.h\"    // parser support routines\n"
     << "\n"
     << "\n";

  if (g->semanticsPrologue != NULL) {
    os << "// ------------ user-supplied prologue ----------\n";
    insertLiteralCode(os, g->semanticsPrologue);
    os << "\n"
       << "\n";
  }

  // forward declarations so the member fn decls in the classes
  // can refer to any tree node class
  os << "// --------------- forward decls -----------------\n";
  FOREACH_OBJLIST(Nonterminal, g->nonterminals, iter) {
    os << "class " << nodeTypeName(iter.data()) << ";\n";
  }
  os << "\n\n";

  // class declarations
  os << "// --------- generated class declarations --------\n";
  FOREACH_OBJLIST(Nonterminal, g->nonterminals, iter) {
    emitClassDecl(os, g, iter.data());
  }

  os << "#endif // " << headerFileLatch(fname) << "\n"
     << "// end of " << fname << "\n";
}
