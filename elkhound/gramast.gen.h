// gramast.gen.h
// *** DO NOT EDIT ***
// generated automatically by astgen, from gramast.ast

#ifndef GRAMAST_GEN_H
#define GRAMAST_GEN_H

#include "asthelp.h"        // helpers for generated code

// fwd decls
class GrammarAST;
class ParseParam;
class Terminals;
class TermDecl;
class TermType;
class PrecSpec;
class SpecFunc;
class NontermDecl;
class ProdDecl;
class RHSElt;
class RH_name;
class RH_string;
class RH_prec;


  #include "locstr.h"       // LocString
  #include "asockind.h"     // AssocKind

// *** DO NOT EDIT ***
class GrammarAST {
public:      // data
  LocString verbatimCode;
  ParseParam *param;
  Terminals *terms;
  ASTList <NontermDecl > nonterms;

public:      // funcs
  GrammarAST(LocString *_verbatimCode, ParseParam *_param, Terminals *_terms, ASTList <NontermDecl > *_nonterms) : verbatimCode(_verbatimCode), param(_param), terms(_terms), nonterms(_nonterms) {
  }
  ~GrammarAST();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class ParseParam {
public:      // data
  bool present;
  LocString type;
  LocString name;

public:      // funcs
  ParseParam(bool _present, LocString *_type, LocString *_name) : present(_present), type(_type), name(_name) {
  }
  ~ParseParam();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class Terminals {
public:      // data
  ASTList <TermDecl > decls;
  ASTList <TermType > types;
  ASTList <PrecSpec > prec;

public:      // funcs
  Terminals(ASTList <TermDecl > *_decls, ASTList <TermType > *_types, ASTList <PrecSpec > *_prec) : decls(_decls), types(_types), prec(_prec) {
  }
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
  TermDecl(int _code, LocString *_name, LocString *_alias) : code(_code), name(_name), alias(_alias) {
  }
  ~TermDecl();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class TermType {
public:      // data
  LocString name;
  LocString type;
  ASTList <SpecFunc > funcs;

public:      // funcs
  TermType(LocString *_name, LocString *_type, ASTList <SpecFunc > *_funcs) : name(_name), type(_type), funcs(_funcs) {
  }
  ~TermType();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class PrecSpec {
public:      // data
  AssocKind kind;
  ASTList <LocString > tokens;

public:      // funcs
  PrecSpec(AssocKind _kind, ASTList <LocString > *_tokens) : kind(_kind), tokens(_tokens) {
  }
  ~PrecSpec();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class SpecFunc {
public:      // data
  LocString name;
  ASTList <LocString > formals;
  LocString code;

public:      // funcs
  SpecFunc(LocString *_name, ASTList <LocString > *_formals, LocString *_code) : name(_name), formals(_formals), code(_code) {
  }
  ~SpecFunc();


  void debugPrint(ostream &os, int indent) const;

  public:  LocString nthFormal(int i) const
    { return *( formals.nthC(i) ); };
};



// *** DO NOT EDIT ***
class NontermDecl {
public:      // data
  LocString name;
  LocString type;
  ASTList <SpecFunc > funcs;
  ASTList <ProdDecl > productions;

public:      // funcs
  NontermDecl(LocString *_name, LocString *_type, ASTList <SpecFunc > *_funcs, ASTList <ProdDecl > *_productions) : name(_name), type(_type), funcs(_funcs), productions(_productions) {
  }
  ~NontermDecl();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class ProdDecl {
public:      // data
  ASTList <RHSElt > rhs;
  LocString actionCode;

public:      // funcs
  ProdDecl(ASTList <RHSElt > *_rhs, LocString *_actionCode) : rhs(_rhs), actionCode(_actionCode) {
  }
  ~ProdDecl();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class RHSElt {
public:      // data

public:      // funcs
  RHSElt() {
  }
  virtual ~RHSElt();

  enum Kind { RH_NAME, RH_STRING, RH_PREC, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(RH_name)
  DECL_AST_DOWNCASTS(RH_string)
  DECL_AST_DOWNCASTS(RH_prec)

  virtual void debugPrint(ostream &os, int indent) const;

};

class RH_name : public RHSElt {
public:      // data
  LocString tag;
  LocString name;

public:      // funcs
  RH_name(LocString *_tag, LocString *_name) : RHSElt(), tag(_tag), name(_name) {
  }
  virtual ~RH_name();

  virtual Kind kind() const { return RH_NAME; }
  enum { TYPE_TAG = RH_NAME };

  virtual void debugPrint(ostream &os, int indent) const;

};

class RH_string : public RHSElt {
public:      // data
  LocString tag;
  LocString str;

public:      // funcs
  RH_string(LocString *_tag, LocString *_str) : RHSElt(), tag(_tag), str(_str) {
  }
  virtual ~RH_string();

  virtual Kind kind() const { return RH_STRING; }
  enum { TYPE_TAG = RH_STRING };

  virtual void debugPrint(ostream &os, int indent) const;

};

class RH_prec : public RHSElt {
public:      // data
  LocString tokName;

public:      // funcs
  RH_prec(LocString *_tokName) : RHSElt(), tokName(_tokName) {
  }
  virtual ~RH_prec();

  virtual Kind kind() const { return RH_PREC; }
  enum { TYPE_TAG = RH_PREC };

  virtual void debugPrint(ostream &os, int indent) const;

};



#endif // GRAMAST_GEN_H
