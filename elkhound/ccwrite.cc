// ccwrite.cc
// code for ccwrite.h

#include "ccwrite.h"      // this module
#include "gramanl.h"      // GrammarAnalysis, Grammar, etc.
#include "strutil.h"      // quoted
#include "emitcode.h"     // EmitCode

#include <string.h>       // strstr
#include <fstream.h>      // ofstream
#include <ctype.h>        // toupper


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


// return a declaration of a semantic function:
//   decl: declaration as appeared in the grammar file
//   name: extracted function name from 'decl'
//   nonterm: symbol the function will be associated with
string semFuncDecl(string name, string decl, Nonterminal const *nonterm)
{
  // find where the name appears in 'decl'; assumes that
  // first such occurrance is the right place....
  char const *nameInDecl = strstr(decl, name);
  xassert(nameInDecl);

  return stringc
    << string(decl, nameInDecl-decl)      // first part
    << nodeTypeName(nonterm) << "::"      // insert node type
    << nameInDecl;                        // second part
}


// foo.h -> __FOO_H
string headerFileLatch(char const *fname)
{
  string ret = stringc << "__" << fname;
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
// emit the C++ code for a nonterminal's semantic functions
void emitSemFuns(EmitCode &os, Grammar const *g,
                 Nonterminal const *nonterm)
{
  // comment to separate types
  os << "// ------------- " << nonterm->name << " -------------\n";

  // emit the node type ctor
  string typeName = nodeTypeName(nonterm);
  os << "NonterminalNode *" << nodeCtorName(nonterm) << "(Reduction *red)\n"
     << "{\n"
     << "  return new " << typeName << "(red);\n"
     << "}\n"
     << "\n"
     << typeName << "::" << typeName << "(Reduction *red)\n"
     << "  : " << g->treeNodeBaseClass << "(red)\n"
     << "{}\n"
     << "\n"
     << "\n"
     ;

  // loop over all declared functions
  for (LitCodeDict::Iter declaration(nonterm->funDecls);
       !declaration.isDone(); declaration.next()) {
    string name = declaration.key();
    LiteralCode const *decl = declaration.value();

    // write the function prologue
    os << lineDirective(decl->loc)
       << semFuncDecl(name, decl->code, nonterm) << "\n"
       << restoreLine
       << "{\n"
       << "  if (tracingSys(\"sem-fns\")) {\n"
       << "    cout << locString() << \": in "
         << semFuncDecl(name, decl->code, nonterm) << "\\n\";\n"
       << "  }\n"
       << "  switch (onlyProductionIndex()) {\n"
          ;

    // loop over all productions for this nonterminal
    FOREACH_OBJLIST(Production, g->productions, prodIter) {
      Production const *prod = prodIter.data();
      if (prod->left != nonterm) { continue; }

      LiteralCode const *body = prod->functions.queryfC(name);

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
          os << "      " << nodeTypeName(sym) << " const &"
             << tag << " = *((" << nodeTypeName(sym)
             << " const *)getOnlyChild(" << childNum
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
      os << "\n"
         << "      // begin user code\n"
         << lineDirective(body->loc)
         << body->code << "\n"
         << restoreLine
         << "      // end user code\n"
         << "      break;\n"    // catch missing returns, or for void fns
         << "    }\n"
         << "\n";

    } // end of loop over productions

    // write function epilogue; if it's a non-void function,
    // rely on compiler warnings to catch missing 'return's
    os << "    default:\n"
          "      xfailure(\"bad production code\");\n"
          //"      throw 0;    // silence warning\n"
          "  }\n"
          "}\n\n\n"
          ;

  } // end of loop over declared functions in the nonterminal
}


// emit the C++ declaration for a nonterminal's node type class
void emitClassDecl(EmitCode &os, Grammar const *g, Nonterminal const *nonterm)
{
  // type declaration prologue
  os << "class " << nodeTypeName(nonterm)
       << " : public " << g->treeNodeBaseClass << " {\n"
     << "public:\n"
     << "  " << nodeTypeName(nonterm) << "(Reduction *red);\n"
     ;

  // loop over all declared functions
  for (LitCodeDict::Iter declaration(nonterm->funDecls);
       !declaration.isDone(); declaration.next()) {
    string name = declaration.key();
    LiteralCode const *decl = declaration.value();

    // emit declaration
    os << lineDirective(decl->loc)
       << "  " << decl->code << ";    // " << name << "\n"
       << restoreLine
       ;
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
        "NonterminalNode *make_empty_Node(Reduction *red)\n"
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
    os << "// -------- user-supplied semantics epilogue --------\n"
       << lineDirective(g->semanticsEpilogue->loc)
       << g->semanticsEpilogue->code << "\n"
       << restoreLine
       << "\n"
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
    os << "// ------------ user-supplied prologue ----------\n"
       << lineDirective(g->semanticsPrologue->loc)
       << g->semanticsPrologue->code << "\n"
       << restoreLine
       << "\n"
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
