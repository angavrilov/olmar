// ast.hand.h
// generated (by hand) from ast.ast

  #include "astlist.h"     // ASTList

// fwd decls
class ASTSpecFile;
class ToplevelForm;
class TF_verbatim;
class ASTClass;
class UserDecl;
class ASTCtor;
class CtorArg;

class ASTSpecFile {
public:
  ASTList<ToplevelForm> forms;
  
public:
  ASTSpecFile() {}
  ~ASTSpecFile() {}
};

class ToplevelForm {
public:
  ToplevelForm() {}
  virtual ~ToplevelForm() {}

  enum Kind { TF_VERBATIM, ASTCLASS, NUM_KINDS };
  virtual Kind kind() const = 0;
};

class TF_verbatim : public ToplevelForm {
public:
  string code;

public:
  TF_verbatim(string _code)
    : code(_code)
  {}
  virtual ~TF_verbatim() {}
  
  virtual Kind kind() const { return TF_VERBATIM; }
  enum { TYPE_TAG = TF_VERBATIM };
};

class ASTClass : public ToplevelForm {
public:
  string name;
  ASTList<UserDcl> decls;
  ASTList<ASTCtor> ctors;

public:
  ASTClass(string _name) :
    name(_name)
  {}
  ~ASTClass() {}
  
  virtual Kind kind() const { return ASTCLASS; }
  enum { TYPE_TAG = ASTCLASS };
};

class UserDecl {
public:
  string access;
  string code;

public:
  UserDecl(string _access, string _code) 
    : access(_access), code(_code)
  {}
  ~UserDecl() {}
};

class ASTCtor {
public:
  string name;
  ASTList<CtorArg> args;
  ASTList<UserDecl> decls;

public:
  ASTCtor(string _name)
    : name(_name)
  {}
  ~ASTCtor() {}
};

class CtorArg {
public:
  bool owner;
  string type;
  string name;
 
public:
  CtorArg(bool _owner, string _type, string _name)
    : owner(owner),
      type(_type),
      name(_name)
  {}
  ~CtorArg() {}
};








