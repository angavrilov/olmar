// gramast.gen.cc
// *** DO NOT EDIT ***
// automatically generated by astgen, from gramast.ast

#include "gramast.gen.h"      // this module


// ------------------ GrammarAST -------------------
// *** DO NOT EDIT ***
GrammarAST::~GrammarAST()
{
  delete terms;
  nonterms.deleteAll();
}

void GrammarAST::debugPrint(ostream &os, int indent) const
{
  PRINT_HEADER(GrammarAST);

  PRINT_SUBTREE(terms);
  PRINT_LIST(NontermDecl, nonterms);
}


// ------------------ Terminals -------------------
// *** DO NOT EDIT ***
Terminals::~Terminals()
{
  decls.deleteAll();
  types.deleteAll();
}

void Terminals::debugPrint(ostream &os, int indent) const
{
  PRINT_HEADER(Terminals);

  PRINT_LIST(TermDecl, decls);
  PRINT_LIST(TermType, types);
}


// ------------------ TermDecl -------------------
// *** DO NOT EDIT ***
TermDecl::~TermDecl()
{
}

void TermDecl::debugPrint(ostream &os, int indent) const
{
  PRINT_HEADER(TermDecl);

  PRINT_GENERIC(code);
  PRINT_GENERIC(name);
  PRINT_GENERIC(alias);
}


// ------------------ TermType -------------------
// *** DO NOT EDIT ***
TermType::~TermType()
{
}

void TermType::debugPrint(ostream &os, int indent) const
{
  PRINT_HEADER(TermType);

  PRINT_GENERIC(name);
  PRINT_GENERIC(type);
}


// ------------------ NontermDecl -------------------
// *** DO NOT EDIT ***
NontermDecl::~NontermDecl()
{
  productions.deleteAll();
}

void NontermDecl::debugPrint(ostream &os, int indent) const
{
  PRINT_HEADER(NontermDecl);

  PRINT_GENERIC(name);
  PRINT_GENERIC(type);
  PRINT_LIST(ProdDecl, productions);
}


// ------------------ ProdDecl -------------------
// *** DO NOT EDIT ***
ProdDecl::~ProdDecl()
{
  rhs.deleteAll();
}

void ProdDecl::debugPrint(ostream &os, int indent) const
{
  PRINT_HEADER(ProdDecl);

  PRINT_LIST(RHSElt, rhs);
  PRINT_GENERIC(actionCode);
}


// ------------------ RHSElt -------------------
// *** DO NOT EDIT ***
RHSElt::~RHSElt()
{
}

void RHSElt::debugPrint(ostream &os, int indent) const
{
}

DEFN_AST_DOWNCASTS(RHSElt, RH_name, RH_NAME)

RH_name::~RH_name()
{
}

void RH_name::debugPrint(ostream &os, int indent) const
{
  PRINT_HEADER(RH_name);

  RHSElt::debugPrint(os, indent);

  PRINT_GENERIC(name);
}

DEFN_AST_DOWNCASTS(RHSElt, RH_taggedName, RH_TAGGEDNAME)

RH_taggedName::~RH_taggedName()
{
}

void RH_taggedName::debugPrint(ostream &os, int indent) const
{
  PRINT_HEADER(RH_taggedName);

  RHSElt::debugPrint(os, indent);

  PRINT_GENERIC(tag);
  PRINT_GENERIC(name);
}

DEFN_AST_DOWNCASTS(RHSElt, RH_string, RH_STRING)

RH_string::~RH_string()
{
}

void RH_string::debugPrint(ostream &os, int indent) const
{
  PRINT_HEADER(RH_string);

  RHSElt::debugPrint(os, indent);

  PRINT_GENERIC(str);
}

DEFN_AST_DOWNCASTS(RHSElt, RH_taggedString, RH_TAGGEDSTRING)

RH_taggedString::~RH_taggedString()
{
}

void RH_taggedString::debugPrint(ostream &os, int indent) const
{
  PRINT_HEADER(RH_taggedString);

  RHSElt::debugPrint(os, indent);

  PRINT_GENERIC(tag);
  PRINT_GENERIC(str);
}


