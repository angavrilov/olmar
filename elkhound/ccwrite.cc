// ccwrite.cc
// code for ccwrite.h

#include "ccwrite.h"      // this module
#include "gramanl.h"      // GrammarAnalysis, Grammar, etc.
#include "strutil.h"      // quoted
#include "syserr.h"       // xsyserror

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


// ----------------- code emitters -----------------
// emit the C++ code for a nonterminal's semantic functions
void emitSemFuns(ostream &os, Grammar const *g,
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
     << "  : NonterminalNode(red)\n"
     << "{}\n"
     << "\n"
     << "\n"
     ;

  // loop over all declared functions
  for (StringDict::IterC declaration(nonterm->funDecls);
       !declaration.isDone(); declaration.next()) {
    string name = declaration.key();
    string decl = declaration.value();

    // write the function prologue
    os << semFuncDecl(name, decl, nonterm) << "\n"
          "{\n"
          "  switch (onlyProductionIndex()) {\n"
          ;

    // loop over all productions for this nonterminal
    FOREACH_OBJLIST(Production, g->productions, prodIter) {
      Production const *prod = prodIter.data();
      if (prod->left != nonterm) { continue; }

      string body = prod->functions.queryf(name);

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
      }

      // write the user's code
      os << "\n"
         << "      // begin user code\n"
         << body
         << "      // end user code\n"
         << "      break;\n"    // catch missing returns, or for void fns
         << "    }\n"
         << "\n";

    } // end of loop over productions

    // write function epilogue; if it's a non-void function,
    // rely on compiler warnings to catch missing 'return's
    os << "    default:\n"
          "      xfailure(\"bad production code\");\n"
          "      throw 0;    // silence warning\n"
          "  }\n"
          "}\n\n\n"
          ;

  } // end of loop over declared functions in the nonterminal
}


// emit the C++ declaration for a nonterminal's node type class
void emitClassDecl(ostream &os, Nonterminal const *nonterm)
{
  // type declaration prologue
  os << "class " << nodeTypeName(nonterm) << " : public NonterminalNode {\n"
     << "public:\n"
     << "  " << nodeTypeName(nonterm) << "(Reduction *red);\n"
     ;

  // loop over all declared functions
  for (StringDict::IterC declaration(nonterm->funDecls);
       !declaration.isDone(); declaration.next()) {
    string name = declaration.key();
    string decl = declaration.value();

    // emit declaration
    os << "  " << decl << ";    // " << name << "\n";
  }

  // type declaration epilogue
  os << "};\n"
     << "\n";
}


// emit the array of info about each nonterminal
void GrammarAnalysis::emitTypeCtorMap(ostream &os) const
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
  ofstream os(fname);
  if (!os) {
    xsyserror("open", fname);
  }

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

  os << "// -------- user-supplied semantics epilogue --------\n"
     << g->semanticsEpilogue
     << "\n"
     << "\n"
     << "// end of " << fname << "\n";
}


// emit the file of C++ node type declarations
void emitSemFunDeclFile(char const *fname, GrammarAnalysis const *g)
{
  ofstream os(fname);
  if (!os) {
    xsyserror("open", fname);
  }

  // file header
  os << "// " << fname << "\n"
     << "// declarations for tree node types\n"
     << "// NOTE: automatically generated file -- editing inadvisable\n"
     << "\n"
     << "#ifndef " << headerFileLatch(fname) << "\n"
     << "#define " << headerFileLatch(fname) << "\n"
     << "\n"
     << "#include \"glrtree.h\"     // NonterminalNode\n"
     << "\n"
     << "\n"
     << "// ------------ user-supplied prologue ----------\n"
     << g->semanticsPrologue
     << "\n"
     << "\n"
     << "// --------- generated class declarations --------\n"
     ;

  // class declarations
  FOREACH_OBJLIST(Nonterminal, g->nonterminals, iter) {
    emitClassDecl(os, iter.data());
  }

  os << "#endif // " << headerFileLatch(fname) << "\n"
     << "// end of " << fname << "\n";
}
