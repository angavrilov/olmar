// gramast.gen.h
// *** DO NOT EDIT ***
// generated automatically by astgen, from gramast.ast

#ifndef GRAMAST_GEN_H
#define GRAMAST_GEN_H

#include "asthelp.h"        // helpers for generated code

// fwd decls
class GrammarAST;
class ToplevelForm;
class TF_terminals;
class TF_nonterminal;
class TF_lit;
class TF_treeNodeBase;
class TermDecl;
class NTBodyElt;
class NT_attr;
class NT_decl;
class NT_elt;
class NT_lit;
class GroupElement;
class GE_form;
class GE_formGroup;
class GE_fbe;
class FormBodyElement;
class FB_action;
class FB_condition;
class FB_treeCompare;
class FB_funDecl;
class FB_funDefn;
class RHSElt;
class RH_ntref;
class RH_termref;
class LiteralCode;
class LC_modifier;
class LC_standAlone;

// *** DO NOT EDIT ***
class GrammarAST {
public:      // data
  ASTList <ToplevelForm > forms;

public:      // funcs
  GrammarAST(ASTList <ToplevelForm > *_forms) : forms(_forms) {}
  ~GrammarAST();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class ToplevelForm {
public:      // data

public:      // funcs
  ToplevelForm() {}
  virtual ~ToplevelForm();

  enum Kind { TF_TERMINALS, TF_NONTERMINAL, TF_LIT, TF_TREENODEBASE, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(TF_terminals)
  DECL_AST_DOWNCASTS(TF_nonterminal)
  DECL_AST_DOWNCASTS(TF_lit)
  DECL_AST_DOWNCASTS(TF_treeNodeBase)

  virtual void debugPrint(ostream &os, int indent) const;

};

class TF_terminals : public ToplevelForm {
public:      // data
  ASTList <TermDecl > terms;

public:      // funcs
  TF_terminals(ASTList <TermDecl > *_terms) : terms(_terms) {}
  virtual ~TF_terminals();

  virtual Kind kind() const { return TF_TERMINALS; }
  enum { TYPE_TAG = TF_TERMINALS };

  virtual void debugPrint(ostream &os, int indent) const;

};

class TF_nonterminal : public ToplevelForm {
public:      // data
  string name;
  ASTList <string > baseClasses;
  ASTList <NTBodyElt > elts;

public:      // funcs
  TF_nonterminal(string _name, ASTList <string > *_baseClasses, ASTList <NTBodyElt > *_elts) : name(_name), baseClasses(_baseClasses), elts(_elts) {}
  virtual ~TF_nonterminal();

  virtual Kind kind() const { return TF_NONTERMINAL; }
  enum { TYPE_TAG = TF_NONTERMINAL };

  virtual void debugPrint(ostream &os, int indent) const;

};

class TF_lit : public ToplevelForm {
public:      // data
  LiteralCode * lit;

public:      // funcs
  TF_lit(LiteralCode * _lit) : lit(_lit) {}
  virtual ~TF_lit();

  virtual Kind kind() const { return TF_LIT; }
  enum { TYPE_TAG = TF_LIT };

  virtual void debugPrint(ostream &os, int indent) const;

};

class TF_treeNodeBase : public ToplevelForm {
public:      // data
  string baseClassName;

public:      // funcs
  TF_treeNodeBase(string _baseClassName) : baseClassName(_baseClassName) {}
  virtual ~TF_treeNodeBase();

  virtual Kind kind() const { return TF_TREENODEBASE; }
  enum { TYPE_TAG = TF_TREENODEBASE };

  virtual void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class TermDecl {
public:      // data
  int code;
  string name;
  string alias;

public:      // funcs
  TermDecl(int _code, string _name, string _alias) : code(_code), name(_name), alias(_alias) {}
  ~TermDecl();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class NTBodyElt {
public:      // data

public:      // funcs
  NTBodyElt() {}
  virtual ~NTBodyElt();

  enum Kind { NT_ATTR, NT_DECL, NT_ELT, NT_LIT, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(NT_attr)
  DECL_AST_DOWNCASTS(NT_decl)
  DECL_AST_DOWNCASTS(NT_elt)
  DECL_AST_DOWNCASTS(NT_lit)

  virtual void debugPrint(ostream &os, int indent) const;

};

class NT_attr : public NTBodyElt {
public:      // data
  string name;

public:      // funcs
  NT_attr(string _name) : name(_name) {}
  virtual ~NT_attr();

  virtual Kind kind() const { return NT_ATTR; }
  enum { TYPE_TAG = NT_ATTR };

  virtual void debugPrint(ostream &os, int indent) const;

};

class NT_decl : public NTBodyElt {
public:      // data
  string declBody;

public:      // funcs
  NT_decl(string _declBody) : declBody(_declBody) {}
  virtual ~NT_decl();

  virtual Kind kind() const { return NT_DECL; }
  enum { TYPE_TAG = NT_DECL };

  virtual void debugPrint(ostream &os, int indent) const;

};

class NT_elt : public NTBodyElt {
public:      // data
  GroupElement * elt;

public:      // funcs
  NT_elt(GroupElement * _elt) : elt(_elt) {}
  virtual ~NT_elt();

  virtual Kind kind() const { return NT_ELT; }
  enum { TYPE_TAG = NT_ELT };

  virtual void debugPrint(ostream &os, int indent) const;

};

class NT_lit : public NTBodyElt {
public:      // data
  LiteralCode * lit;

public:      // funcs
  NT_lit(LiteralCode * _lit) : lit(_lit) {}
  virtual ~NT_lit();

  virtual Kind kind() const { return NT_LIT; }
  enum { TYPE_TAG = NT_LIT };

  virtual void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class GroupElement {
public:      // data

public:      // funcs
  GroupElement() {}
  virtual ~GroupElement();

  enum Kind { GE_FORM, GE_FORMGROUP, GE_FBE, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(GE_form)
  DECL_AST_DOWNCASTS(GE_formGroup)
  DECL_AST_DOWNCASTS(GE_fbe)

  virtual void debugPrint(ostream &os, int indent) const;

};

class GE_form : public GroupElement {
public:      // data
  ASTList <RHSElt > rhs;
  ASTList <FormBodyElt > elts;

public:      // funcs
  GE_form(ASTList <RHSElt > *_rhs, ASTList <FormBodyElt > *_elts) : rhs(_rhs), elts(_elts) {}
  virtual ~GE_form();

  virtual Kind kind() const { return GE_FORM; }
  enum { TYPE_TAG = GE_FORM };

  virtual void debugPrint(ostream &os, int indent) const;

};

class GE_formGroup : public GroupElement {
public:      // data
  ASTList <GroupElements > elts;

public:      // funcs
  GE_formGroup(ASTList <GroupElements > *_elts) : elts(_elts) {}
  virtual ~GE_formGroup();

  virtual Kind kind() const { return GE_FORMGROUP; }
  enum { TYPE_TAG = GE_FORMGROUP };

  virtual void debugPrint(ostream &os, int indent) const;

};

class GE_fbe : public GroupElement {
public:      // data
  FormBodyElement * e;

public:      // funcs
  GE_fbe(FormBodyElement * _e) : e(_e) {}
  virtual ~GE_fbe();

  virtual Kind kind() const { return GE_FBE; }
  enum { TYPE_TAG = GE_FBE };

  virtual void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class FormBodyElement {
public:      // data

public:      // funcs
  FormBodyElement() {}
  virtual ~FormBodyElement();

  enum Kind { FB_ACTION, FB_CONDITION, FB_TREECOMPARE, FB_FUNDECL, FB_FUNDEFN, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(FB_action)
  DECL_AST_DOWNCASTS(FB_condition)
  DECL_AST_DOWNCASTS(FB_treeCompare)
  DECL_AST_DOWNCASTS(FB_funDecl)
  DECL_AST_DOWNCASTS(FB_funDefn)

  virtual void debugPrint(ostream &os, int indent) const;

};

class FB_action : public FormBodyElement {
public:      // data
  string name;
  AttrExpr * expr;

public:      // funcs
  FB_action(string _name, AttrExpr * _expr) : name(_name), expr(_expr) {}
  virtual ~FB_action();

  virtual Kind kind() const { return FB_ACTION; }
  enum { TYPE_TAG = FB_ACTION };

  virtual void debugPrint(ostream &os, int indent) const;

};

class FB_condition : public FormBodyElement {
public:      // data
  AttrExpr * condExpr;

public:      // funcs
  FB_condition(AttrExpr * _condExpr) : condExpr(_condExpr) {}
  virtual ~FB_condition();

  virtual Kind kind() const { return FB_CONDITION; }
  enum { TYPE_TAG = FB_CONDITION };

  virtual void debugPrint(ostream &os, int indent) const;

};

class FB_treeCompare : public FormBodyElement {
public:      // data
  string leftName;
  string rightName;
  AttrExpr * decideExpr;

public:      // funcs
  FB_treeCompare(string _leftName, string _rightName, AttrExpr * _decideExpr) : leftName(_leftName), rightName(_rightName), decideExpr(_decideExpr) {}
  virtual ~FB_treeCompare();

  virtual Kind kind() const { return FB_TREECOMPARE; }
  enum { TYPE_TAG = FB_TREECOMPARE };

  virtual void debugPrint(ostream &os, int indent) const;

};

class FB_funDecl : public FormBodyElement {
public:      // data
  string funDeclBody;

public:      // funcs
  FB_funDecl(string _funDeclBody) : funDeclBody(_funDeclBody) {}
  virtual ~FB_funDecl();

  virtual Kind kind() const { return FB_FUNDECL; }
  enum { TYPE_TAG = FB_FUNDECL };

  virtual void debugPrint(ostream &os, int indent) const;

};

class FB_funDefn : public FormBodyElement {
public:      // data
  string name;
  string funDefnBody;

public:      // funcs
  FB_funDefn(string _name, string _funDefnBody) : name(_name), funDefnBody(_funDefnBody) {}
  virtual ~FB_funDefn();

  virtual Kind kind() const { return FB_FUNDEFN; }
  enum { TYPE_TAG = FB_FUNDEFN };

  virtual void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class RHSElt {
public:      // data

public:      // funcs
  RHSElt() {}
  virtual ~RHSElt();

  enum Kind { RH_NTREF, RH_TERMREF, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(RH_ntref)
  DECL_AST_DOWNCASTS(RH_termref)

  virtual void debugPrint(ostream &os, int indent) const;

};

class RH_ntref : public RHSElt {
public:      // data
  string tag;
  string ntname;

public:      // funcs
  RH_ntref(string _tag, string _ntname) : tag(_tag), ntname(_ntname) {}
  virtual ~RH_ntref();

  virtual Kind kind() const { return RH_NTREF; }
  enum { TYPE_TAG = RH_NTREF };

  virtual void debugPrint(ostream &os, int indent) const;

};

class RH_termref : public RHSElt {
public:      // data
  string tag;
  string tokname;

public:      // funcs
  RH_termref(string _tag, string _tokname) : tag(_tag), tokname(_tokname) {}
  virtual ~RH_termref();

  virtual Kind kind() const { return RH_TERMREF; }
  enum { TYPE_TAG = RH_TERMREF };

  virtual void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class LiteralCode {
public:      // data

public:      // funcs
  LiteralCode() {}
  virtual ~LiteralCode();

  enum Kind { LC_MODIFIER, LC_STANDALONE, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(LC_modifier)
  DECL_AST_DOWNCASTS(LC_standAlone)

  virtual void debugPrint(ostream &os, int indent) const;

};

class LC_modifier : public LiteralCode {
public:      // data
  string codeKindTag;
  string funcToModify;
  string codeBody;

public:      // funcs
  LC_modifier(string _codeKindTag, string _funcToModify, string _codeBody) : codeKindTag(_codeKindTag), funcToModify(_funcToModify), codeBody(_codeBody) {}
  virtual ~LC_modifier();

  virtual Kind kind() const { return LC_MODIFIER; }
  enum { TYPE_TAG = LC_MODIFIER };

  virtual void debugPrint(ostream &os, int indent) const;

};

class LC_standAlone : public LiteralCode {
public:      // data
  string codeKindTag;
  string codeBody;

public:      // funcs
  LC_standAlone(string _codeKindTag, string _codeBody) : codeKindTag(_codeKindTag), codeBody(_codeBody) {}
  virtual ~LC_standAlone();

  virtual Kind kind() const { return LC_STANDALONE; }
  enum { TYPE_TAG = LC_STANDALONE };

  virtual void debugPrint(ostream &os, int indent) const;

};



#endif // GRAMAST_GEN_H
