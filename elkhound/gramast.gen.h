// gramast.gen.h
// *** DO NOT EDIT ***
// generated automatically by astgen, from gramast.ast

#ifndef GRAMAST_GEN_H
#define GRAMAST_GEN_H

#include "asthelp.h"        // helpers for generated code

// fwd decls
class GrammarAST;
class Terminals;
class TermDecl;
class TermType;
class NontermDecl;
class ProdDecl;
class RHSElt;
class RH_name;
class RH_taggedName;
class RH_string;
class RH_taggedString;


  #include "locstr.h"       // LocString

// *** DO NOT EDIT ***
class GrammarAST {
public:      // data
  LocString verbatimCode;
  Terminals *terms;
  ASTList <NontermDecl > nonterms;

public:      // funcs
  GrammarAST(LocString *_verbatimCode, Terminals *_terms, ASTList <NontermDecl > *_nonterms) : verbatimCode(_verbatimCode), terms(_terms), nonterms(_nonterms) {}
  ~GrammarAST();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class Terminals {
public:      // data
  ASTList <TermDecl > decls;
  ASTList <TermType > types;

public:      // funcs
  Terminals(ASTList <TermDecl > *_decls, ASTList <TermType > *_types) : decls(_decls), types(_types) {}
  ~Terminals();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class TermDecl {
public:      // data
  int code;
  LocString name;
  LocString alias;

public:      // funcs
  TermDecl(int _code, LocString *_name, LocString *_alias) : code(_code), name(_name), alias(_alias) {}
  ~TermDecl();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class TermType {
public:      // data
  LocString name;
  LocString type;

public:      // funcs
  TermType(LocString *_name, LocString *_type) : name(_name), type(_type) {}
  ~TermType();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class NontermDecl {
public:      // data
  LocString name;
  LocString type;
  ASTList <ProdDecl > productions;

public:      // funcs
  NontermDecl(LocString *_name, LocString *_type, ASTList <ProdDecl > *_productions) : name(_name), type(_type), productions(_productions) {}
  ~NontermDecl();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class ProdDecl {
public:      // data
  ASTList <RHSElt > rhs;
  LocString actionCode;

public:      // funcs
  ProdDecl(ASTList <RHSElt > *_rhs, LocString *_actionCode) : rhs(_rhs), actionCode(_actionCode) {}
  ~ProdDecl();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class RHSElt {
public:      // data

public:      // funcs
  RHSElt() {}
  virtual ~RHSElt();

  enum Kind { RH_NAME, RH_TAGGEDNAME, RH_STRING, RH_TAGGEDSTRING, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(RH_name)
  DECL_AST_DOWNCASTS(RH_taggedName)
  DECL_AST_DOWNCASTS(RH_string)
  DECL_AST_DOWNCASTS(RH_taggedString)

  virtual void debugPrint(ostream &os, int indent) const;

};

class RH_name : public RHSElt {
public:      // data
  LocString name;

public:      // funcs
  RH_name(LocString *_name) : RHSElt(), name(_name) {}
  virtual ~RH_name();

  virtual Kind kind() const { return RH_NAME; }
  enum { TYPE_TAG = RH_NAME };

  virtual void debugPrint(ostream &os, int indent) const;

};

class RH_taggedName : public RHSElt {
public:      // data
  LocString tag;
  LocString name;

public:      // funcs
  RH_taggedName(LocString *_tag, LocString *_name) : RHSElt(), tag(_tag), name(_name) {}
  virtual ~RH_taggedName();

  virtual Kind kind() const { return RH_TAGGEDNAME; }
  enum { TYPE_TAG = RH_TAGGEDNAME };

  virtual void debugPrint(ostream &os, int indent) const;

};

class RH_string : public RHSElt {
public:      // data
  LocString str;

public:      // funcs
  RH_string(LocString *_str) : RHSElt(), str(_str) {}
  virtual ~RH_string();

  virtual Kind kind() const { return RH_STRING; }
  enum { TYPE_TAG = RH_STRING };

  virtual void debugPrint(ostream &os, int indent) const;

};

class RH_taggedString : public RHSElt {
public:      // data
  LocString tag;
  LocString str;

public:      // funcs
  RH_taggedString(LocString *_tag, LocString *_str) : RHSElt(), tag(_tag), str(_str) {}
  virtual ~RH_taggedString();

  virtual Kind kind() const { return RH_TAGGEDSTRING; }
  enum { TYPE_TAG = RH_TAGGEDSTRING };

  virtual void debugPrint(ostream &os, int indent) const;

};



#endif // GRAMAST_GEN_H
