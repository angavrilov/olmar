// ast.ast.h
// *** DO NOT EDIT ***
// generated automatically by astgen, from ast.ast

#ifndef AST_AST_H
#define AST_AST_H

#include "asthelp.h"        // helpers for generated code

// fwd decls
class ASTSpecFile;
class ToplevelForm;
class TF_verbatim;
class TF_impl_verbatim;
class TF_class;
class ASTClass;
class Annotation;
class UserDecl;
class CustomCode;
class CtorArg;

// *** DO NOT EDIT ***

  #include "str.h"         // string

// *** DO NOT EDIT ***
class ASTSpecFile {
public:      // data
  ASTList <ToplevelForm > forms;

public:      // funcs
  ASTSpecFile(ASTList <ToplevelForm > *_forms) : forms(_forms) {
  }
  ~ASTSpecFile();


  void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class ToplevelForm {
public:      // data

public:      // funcs
  ToplevelForm() {
  }
  virtual ~ToplevelForm();

  enum Kind { TF_VERBATIM, TF_IMPL_VERBATIM, TF_CLASS, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(TF_verbatim)
  DECL_AST_DOWNCASTS(TF_impl_verbatim)
  DECL_AST_DOWNCASTS(TF_class)

  virtual void debugPrint(ostream &os, int indent) const;

};

class TF_verbatim : public ToplevelForm {
public:      // data
  string code;

public:      // funcs
  TF_verbatim(string _code) : ToplevelForm(), code(_code) {
  }
  virtual ~TF_verbatim();

  virtual Kind kind() const { return TF_VERBATIM; }
  enum { TYPE_TAG = TF_VERBATIM };

  virtual void debugPrint(ostream &os, int indent) const;

};

class TF_impl_verbatim : public ToplevelForm {
public:      // data
  string code;

public:      // funcs
  TF_impl_verbatim(string _code) : ToplevelForm(), code(_code) {
  }
  virtual ~TF_impl_verbatim();

  virtual Kind kind() const { return TF_IMPL_VERBATIM; }
  enum { TYPE_TAG = TF_IMPL_VERBATIM };

  virtual void debugPrint(ostream &os, int indent) const;

};

class TF_class : public ToplevelForm {
public:      // data
  ASTClass *super;
  ASTList <ASTClass > ctors;

public:      // funcs
  TF_class(ASTClass *_super, ASTList <ASTClass > *_ctors) : ToplevelForm(), super(_super), ctors(_ctors) {
  }
  virtual ~TF_class();

  virtual Kind kind() const { return TF_CLASS; }
  enum { TYPE_TAG = TF_CLASS };

  virtual void debugPrint(ostream &os, int indent) const;

  public:  bool hasChildren() const { return ctors.isNotEmpty(); };
};



// *** DO NOT EDIT ***
class ASTClass {
public:      // data
  string name;
  ASTList <CtorArg > args;
  ASTList <Annotation > decls;

public:      // funcs
  ASTClass(string _name, ASTList <CtorArg > *_args, ASTList <Annotation > *_decls) : name(_name), args(_args), decls(_decls) {
  }
  ~ASTClass();


  void debugPrint(ostream &os, int indent) const;

  public:  string kindName() const;
};



// *** DO NOT EDIT ***

  // specifies what kind of userdecl this is; pub/priv/prot are uninterpreted
  // class members with the associated access control; ctor and dtor are
  // code to be inserted into the ctor or dtor, respectively
  enum AccessCtl {
    AC_PUBLIC,      // access
    AC_PRIVATE,     //   control
    AC_PROTECTED,   //     keywords
    AC_CTOR,        // insert into ctor
    AC_DTOR,        // insert into dtor
    AC_PUREVIRT,    // declare pure virtual in superclass, and impl in subclass
    NUM_ACCESSCTLS
  };

  // map the enum value to a string like "public"
  string toString(AccessCtl acc);      // defined in ast.cc

// *** DO NOT EDIT ***
class Annotation {
public:      // data

public:      // funcs
  Annotation() {
  }
  virtual ~Annotation();

  enum Kind { USERDECL, CUSTOMCODE, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCASTS(UserDecl)
  DECL_AST_DOWNCASTS(CustomCode)

  virtual void debugPrint(ostream &os, int indent) const;

};

class UserDecl : public Annotation {
public:      // data
  AccessCtl access;
  string code;

public:      // funcs
  UserDecl(AccessCtl _access, string _code) : Annotation(), access(_access), code(_code) {
  }
  virtual ~UserDecl();

  virtual Kind kind() const { return USERDECL; }
  enum { TYPE_TAG = USERDECL };

  virtual void debugPrint(ostream &os, int indent) const;

};

class CustomCode : public Annotation {
public:      // data
  string qualifier;
  string code;

public:      // funcs
  CustomCode(string _qualifier, string _code) : Annotation(), qualifier(_qualifier), code(_code) {
  }
  virtual ~CustomCode();

  virtual Kind kind() const { return CUSTOMCODE; }
  enum { TYPE_TAG = CUSTOMCODE };

  virtual void debugPrint(ostream &os, int indent) const;

};



// *** DO NOT EDIT ***
class CtorArg {
public:      // data
  bool owner;
  string type;
  string name;

public:      // funcs
  CtorArg(bool _owner, string _type, string _name) : owner(_owner), type(_type), name(_name) {
  }
  ~CtorArg();


  void debugPrint(ostream &os, int indent) const;

};



#endif // AST_AST_H
