// ast.gen.h
// *** DO NOT EDIT ***
// generated automatically by astgen, from ast.ast
//   on Fri Jul 27 14:21:58 2001

#ifndef AST_GEN_H
#define AST_GEN_H

#include "asthelp.h"        // helpers for generated code

// fwd decls
class ASTSpecFile;
class ToplevelForm;
class UserDecl;
class ASTCtor;
class CtorArg;


  #include "str.h"         // string

class ASTSpecFile {
public:      // data
  ASTList <ToplevelForm > forms;

public:      // funcs
  ASTSpecFile(ASTList <ToplevelForm > *_forms) : forms(_forms) {}
  ~ASTSpecFile() {}


  void debugPrint(ostream &os, int indent) const;

};



class ToplevelForm {
public:      // data

public:      // funcs
  ToplevelForm() {}
  virtual ~ToplevelForm() {}

  enum Kind { TF_VERBATIM, ASTCLASS, NUM_KINDS };
  virtual Kind kind() const = 0;

  DECL_AST_DOWNCAST(TF_verbatim)
  DECL_AST_DOWNCAST(ASTClass)

  virtual void debugPrint(ostream &os, int indent) const;

};

class TF_verbatim : public ToplevelForm {
public:      // data
  string code;

public:      // funcs
  TF_verbatim(string _code) : code(_code) {}
  virtual ~TF_verbatim() {}

  virtual Kind kind() const { return TF_VERBATIM; }
  enum { TYPE_TAG = TF_VERBATIM };

  virtual void debugPrint(ostream &os, int indent) const;

};

class ASTClass : public ToplevelForm {
public:      // data
  string name;
  ASTList <CtorArg > superCtor;
  ASTList <UserDecl > decls;
  ASTList <ASTCtor > ctors;

public:      // funcs
  ASTClass(string _name, ASTList <CtorArg > *_superCtor, ASTList <UserDecl > *_decls, ASTList <ASTCtor > *_ctors) : name(_name), superCtor(_superCtor), decls(_decls), ctors(_ctors) {}
  virtual ~ASTClass() {}

  virtual Kind kind() const { return ASTCLASS; }
  enum { TYPE_TAG = ASTCLASS };

  virtual void debugPrint(ostream &os, int indent) const;

  public:  bool hasChildren() const { return ctors.isNotEmpty(); };
};




  enum AccessCtl { AC_PUBLIC, AC_PRIVATE, AC_PROTECTED };
  string toString(AccessCtl acc);      // defined in ast.cc

class UserDecl {
public:      // data
  AccessCtl access;
  string code;

public:      // funcs
  UserDecl(AccessCtl _access, string _code) : access(_access), code(_code) {}
  ~UserDecl() {}


  void debugPrint(ostream &os, int indent) const;

};



class ASTCtor {
public:      // data
  string name;
  ASTList <CtorArg > args;
  ASTList <UserDecl > decls;

public:      // funcs
  ASTCtor(string _name, ASTList <CtorArg > *_args, ASTList <UserDecl > *_decls) : name(_name), args(_args), decls(_decls) {}
  ~ASTCtor() {}


  void debugPrint(ostream &os, int indent) const;

  public:  string kindName() const;
};



class CtorArg {
public:      // data
  bool owner;
  string type;
  string name;

public:      // funcs
  CtorArg(bool _owner, string _type, string _name) : owner(_owner), type(_type), name(_name) {}
  ~CtorArg() {}


  void debugPrint(ostream &os, int indent) const;

};



#endif // AST_GEN_H
