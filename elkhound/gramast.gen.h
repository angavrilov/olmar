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
class FormBodyElt;
class FB_action;
class FB_condition;
class FB_treeCompare;
class FB_funDecl;
class FB_funDefn;
class FB_dataDecl;
class RHS;
class RHSElt;
class RH_name;
class RH_taggedName;
class RH_string;
class RH_taggedString;
class ExprAST;
class E_attrRef;
class E_treeAttrRef;
class E_intLit;
class E_funCall;
class E_unary;
class E_binary;
class E_cond;
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
  LocString name;
  ASTList <LocString > baseClasses;
  ASTList <NTBodyElt > elts;

public:      // funcs
  TF_nonterminal(LocString *_name, ASTList <LocString > *_baseClasses, ASTList <NTBodyElt > *_elts) : name(_name), baseClasses(_baseClasses), elts(_elts) {}
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
  LocString baseClassName;

public:      // funcs
  TF_treeNodeBase(LocString *_baseClassName) : baseClassName(_baseClassName) {}
  virtual ~TF_treeNodeBase();

  virtual Kind kind() const { return TF_TREENODEBASE; }
  enum { TYPE_TAG = TF_TREENODEBASE };

  virtual void debugPrint(ostream &os, int indent) const;

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
  LocString name;

public:      // funcs
  NT_attr(LocString *_name) : name(_name) {}
  virtual ~NT_attr();

  virtual Kind kind() const { return NT_ATTR; }
  enum { TYPE_TAG = NT_ATTR };

  virtual void debugPrint(ostream &os, int indent) const;

};

class NT_decl : public NTBodyElt {
public:      // data
  LocString declBody;

public:      // funcs
  NT_decl(LocString *_declBody) : declBody(_declBody) {}
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
  ASTList <RHS > rhsides;
  ASTList <FormBodyElt > elts;

public:      // funcs
  GE_form(ASTList <RHS > *_rhsides, ASTList <FormBodyElt > *_elts) : rhsides(_rhsides), elts(_elts) {}
  virtual ~GE_form();

  virtual Kind kind() const { return GE_FORM; }
  enum { TYPE_TAG = GE_FORM };

  virtual void debugPrint(ostream &os, int indent) const;

};

class GE_formGroup : public GroupElement {
public:      // data
  ASTList <GroupElement > elts;

public:      // funcs
  GE_formGroup(ASTList <GroupElement > *_elts) : elts(_elts) {}
  virtual ~GE_formGroup();

  virtual Kind kind() const { return GE_FORMGROUP; }
  enum { TYPE_TAG = GE_FORMGROUP };

  virtual void debugPrint(ostream &os, int indent) const;

};

class GE_fbe : public GroupElement {
public:      // data
  FormBodyElt * e;

public:      // funcs
  GE_fbe(FormBodyElt * _e) : e(_e) {}
  virtual ~GE_fbe();

  virtual Kind kind() const { return GE_FBE; }
  enum { TYPE_TAG = GE_FBE };

  virtual void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class FormBodyElt {
public:      // data

public:      // funcs
  FormBodyElt() {}
  virtual ~FormBodyElt();

  enum Kind { FB_ACTION, FB_CONDITION, FB_TREECOMPARE, FB_FUNDECL, FB_FUNDEFN, FB_DATADECL, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(FB_action)
  DECL_AST_DOWNCASTS(FB_condition)
  DECL_AST_DOWNCASTS(FB_treeCompare)
  DECL_AST_DOWNCASTS(FB_funDecl)
  DECL_AST_DOWNCASTS(FB_funDefn)
  DECL_AST_DOWNCASTS(FB_dataDecl)

  virtual void debugPrint(ostream &os, int indent) const;

};

class FB_action : public FormBodyElt {
public:      // data
  LocString name;
  ExprAST *expr;

public:      // funcs
  FB_action(LocString *_name, ExprAST *_expr) : name(_name), expr(_expr) {}
  virtual ~FB_action();

  virtual Kind kind() const { return FB_ACTION; }
  enum { TYPE_TAG = FB_ACTION };

  virtual void debugPrint(ostream &os, int indent) const;

};

class FB_condition : public FormBodyElt {
public:      // data
  ExprAST * condExpr;

public:      // funcs
  FB_condition(ExprAST * _condExpr) : condExpr(_condExpr) {}
  virtual ~FB_condition();

  virtual Kind kind() const { return FB_CONDITION; }
  enum { TYPE_TAG = FB_CONDITION };

  virtual void debugPrint(ostream &os, int indent) const;

};

class FB_treeCompare : public FormBodyElt {
public:      // data
  LocString leftName;
  LocString rightName;
  ExprAST * decideExpr;

public:      // funcs
  FB_treeCompare(LocString *_leftName, LocString *_rightName, ExprAST * _decideExpr) : leftName(_leftName), rightName(_rightName), decideExpr(_decideExpr) {}
  virtual ~FB_treeCompare();

  virtual Kind kind() const { return FB_TREECOMPARE; }
  enum { TYPE_TAG = FB_TREECOMPARE };

  virtual void debugPrint(ostream &os, int indent) const;

};

class FB_funDecl : public FormBodyElt {
public:      // data
  LocString declBody;

public:      // funcs
  FB_funDecl(LocString *_declBody) : declBody(_declBody) {}
  virtual ~FB_funDecl();

  virtual Kind kind() const { return FB_FUNDECL; }
  enum { TYPE_TAG = FB_FUNDECL };

  virtual void debugPrint(ostream &os, int indent) const;

};

class FB_funDefn : public FormBodyElt {
public:      // data
  LocString name;
  LocString defnBody;

public:      // funcs
  FB_funDefn(LocString *_name, LocString *_defnBody) : name(_name), defnBody(_defnBody) {}
  virtual ~FB_funDefn();

  virtual Kind kind() const { return FB_FUNDEFN; }
  enum { TYPE_TAG = FB_FUNDEFN };

  virtual void debugPrint(ostream &os, int indent) const;

};

class FB_dataDecl : public FormBodyElt {
public:      // data
  LocString declBody;

public:      // funcs
  FB_dataDecl(LocString *_declBody) : declBody(_declBody) {}
  virtual ~FB_dataDecl();

  virtual Kind kind() const { return FB_DATADECL; }
  enum { TYPE_TAG = FB_DATADECL };

  virtual void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class RHS {
public:      // data
  ASTList <RHSElt > rhs;

public:      // funcs
  RHS(ASTList <RHSElt > *_rhs) : rhs(_rhs) {}
  ~RHS();


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
  RH_name(LocString *_name) : name(_name) {}
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
  RH_taggedName(LocString *_tag, LocString *_name) : tag(_tag), name(_name) {}
  virtual ~RH_taggedName();

  virtual Kind kind() const { return RH_TAGGEDNAME; }
  enum { TYPE_TAG = RH_TAGGEDNAME };

  virtual void debugPrint(ostream &os, int indent) const;

};

class RH_string : public RHSElt {
public:      // data
  LocString str;

public:      // funcs
  RH_string(LocString *_str) : str(_str) {}
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
  RH_taggedString(LocString *_tag, LocString *_str) : tag(_tag), str(_str) {}
  virtual ~RH_taggedString();

  virtual Kind kind() const { return RH_TAGGEDSTRING; }
  enum { TYPE_TAG = RH_TAGGEDSTRING };

  virtual void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class ExprAST {
public:      // data

public:      // funcs
  ExprAST() {}
  virtual ~ExprAST();

  enum Kind { E_ATTRREF, E_TREEATTRREF, E_INTLIT, E_FUNCALL, E_UNARY, E_BINARY, E_COND, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(E_attrRef)
  DECL_AST_DOWNCASTS(E_treeAttrRef)
  DECL_AST_DOWNCASTS(E_intLit)
  DECL_AST_DOWNCASTS(E_funCall)
  DECL_AST_DOWNCASTS(E_unary)
  DECL_AST_DOWNCASTS(E_binary)
  DECL_AST_DOWNCASTS(E_cond)

  virtual void debugPrint(ostream &os, int indent) const;

};

class E_attrRef : public ExprAST {
public:      // data
  LocString tag;
  LocString attr;

public:      // funcs
  E_attrRef(LocString *_tag, LocString *_attr) : tag(_tag), attr(_attr) {}
  virtual ~E_attrRef();

  virtual Kind kind() const { return E_ATTRREF; }
  enum { TYPE_TAG = E_ATTRREF };

  virtual void debugPrint(ostream &os, int indent) const;

};

class E_treeAttrRef : public ExprAST {
public:      // data
  LocString tree;
  LocString tag;
  LocString attr;

public:      // funcs
  E_treeAttrRef(LocString *_tree, LocString *_tag, LocString *_attr) : tree(_tree), tag(_tag), attr(_attr) {}
  virtual ~E_treeAttrRef();

  virtual Kind kind() const { return E_TREEATTRREF; }
  enum { TYPE_TAG = E_TREEATTRREF };

  virtual void debugPrint(ostream &os, int indent) const;

};

class E_intLit : public ExprAST {
public:      // data
  int val;

public:      // funcs
  E_intLit(int _val) : val(_val) {}
  virtual ~E_intLit();

  virtual Kind kind() const { return E_INTLIT; }
  enum { TYPE_TAG = E_INTLIT };

  virtual void debugPrint(ostream &os, int indent) const;

};

class E_funCall : public ExprAST {
public:      // data
  LocString funcName;
  ASTList <ExprAST > args;

public:      // funcs
  E_funCall(LocString *_funcName, ASTList <ExprAST > *_args) : funcName(_funcName), args(_args) {}
  virtual ~E_funCall();

  virtual Kind kind() const { return E_FUNCALL; }
  enum { TYPE_TAG = E_FUNCALL };

  virtual void debugPrint(ostream &os, int indent) const;

};

class E_unary : public ExprAST {
public:      // data
  int op;
  ExprAST * exp;

public:      // funcs
  E_unary(int _op, ExprAST * _exp) : op(_op), exp(_exp) {}
  virtual ~E_unary();

  virtual Kind kind() const { return E_UNARY; }
  enum { TYPE_TAG = E_UNARY };

  virtual void debugPrint(ostream &os, int indent) const;

};

class E_binary : public ExprAST {
public:      // data
  int op;
  ExprAST * left;
  ExprAST * right;

public:      // funcs
  E_binary(int _op, ExprAST * _left, ExprAST * _right) : op(_op), left(_left), right(_right) {}
  virtual ~E_binary();

  virtual Kind kind() const { return E_BINARY; }
  enum { TYPE_TAG = E_BINARY };

  virtual void debugPrint(ostream &os, int indent) const;

};

class E_cond : public ExprAST {
public:      // data
  ExprAST *test;
  ExprAST *thenExp;
  ExprAST *elseExp;

public:      // funcs
  E_cond(ExprAST *_test, ExprAST *_thenExp, ExprAST *_elseExp) : test(_test), thenExp(_thenExp), elseExp(_elseExp) {}
  virtual ~E_cond();

  virtual Kind kind() const { return E_COND; }
  enum { TYPE_TAG = E_COND };

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
  LocString codeKindTag;
  LocString funcToModify;
  LocString codeBody;

public:      // funcs
  LC_modifier(LocString *_codeKindTag, LocString *_funcToModify, LocString *_codeBody) : codeKindTag(_codeKindTag), funcToModify(_funcToModify), codeBody(_codeBody) {}
  virtual ~LC_modifier();

  virtual Kind kind() const { return LC_MODIFIER; }
  enum { TYPE_TAG = LC_MODIFIER };

  virtual void debugPrint(ostream &os, int indent) const;

};

class LC_standAlone : public LiteralCode {
public:      // data
  LocString codeKindTag;
  LocString codeBody;

public:      // funcs
  LC_standAlone(LocString *_codeKindTag, LocString *_codeBody) : codeKindTag(_codeKindTag), codeBody(_codeBody) {}
  virtual ~LC_standAlone();

  virtual Kind kind() const { return LC_STANDALONE; }
  enum { TYPE_TAG = LC_STANDALONE };

  virtual void debugPrint(ostream &os, int indent) const;

};



#endif // GRAMAST_GEN_H
