// cil.h
// C Intermediate Language

#ifndef CIL_H
#define CIL_H

#include "str.h"       // string
#include "objlist.h"   // ObjList

// other files
class Type;            // cc_type.h
class FunctionType;    // cc_type.h
class Env;             // cc_env.h
class Variable;        // cc_env.h

// fwd for this file
class CilExpr;
class CilLval;
class CilFnCall;
class CilCompound;

// ------------- names --------------
typedef string VarName;     // TODO2: find a better place for this to live

typedef string LabelName;
LabelName newTmpLabel();


// --------------- operators ----------------
// TODO: for now, I don't distinguish e.g. unsigned
// and signed operations
enum BinOp { 
  OP_PLUS, OP_MINUS, OP_TIMES, OP_DIVIDE, OP_MOD,
  OP_LSHIFT, OP_RSHIFT,
  OP_LT, OP_GT, OP_LTE, OP_GTE,
  OP_EQUAL, OP_NOTEQUAL,
  OP_BITAND, OP_BITXOR, OP_BITOR,
  OP_AND, OP_OR,
  NUM_BINOPS
};
char const *binOpText(BinOp op);    // e.g. "+"
void validate(BinOp op);            // exception if out of range


enum UnaryOp { OP_NEGATE, OP_NOT, OP_BITNOT, NUM_UNOPS };
char const *unOpText(UnaryOp op);   // e.g. "!"
void validate(UnaryOp op);


// -------------- CilExpr ------------
// an expression denotes a method for computing a value at runtime;
// this value need not have an associated "location" -- i.e. it need
// not be modifiable -- unlike Lvalues, below; CilExprs form trees,
// where parents own children; thus, there can be *no* sharing of
// subtrees between different CilExprs
class CilExpr {
public:      // types
  enum ETag { T_LITERAL, T_LVAL, T_UNOP, T_BINOP,
              T_CASTE, T_ADDROF, NUM_ETAGS };

public:      // data
  ETag const etag;            // which kind of expression

  union {
    // T_LITERAL
    struct {
      int value;
    } lit;

    // T_LVAL
    CilLval *lval;            // (serf) actually equal to 'this'

    // T_UNOP
    struct {
      UnaryOp op;
      CilExpr *exp;           // (owner)
    } unop;

    // T_BINOP
    struct {
      BinOp op;
      CilExpr *left, *right;  // (owner)
    } binop;

    // T_CASTE   ("E" means cast Expression)
    struct {
      Type const *type;       // (serf) type being cast to
      CilExpr *exp;           // (owner) source expression
    } caste;

    // T_ADDROF
    struct {
      CilLval *lval;          // (owner) thing whose address is being taken
    } addrof;
  };

  // count and high-water allocated nodes
  static int numAllocd;
  static int maxAllocd;

public:      // funcs
  // the ctor just accepts a tag; caller must fill in the
  // other fields
  CilExpr(ETag tag);
  ~CilExpr();
  
  // we need the 'env' argument because we may have to
  // synthesize a new type (e.g. if this is an addr-of
  // expr, we'll need a ptr-to type)
  Type const *getType(Env *env) const;

  CilLval *asLval();
  bool isLval() const { return etag == T_LVAL; }

  // throw an exception if 'tag' is out of range
  static void validate(ETag tag);
  
  static void printAllocStats();

  CilExpr *clone() const;     // deep copy

  string toString() const;    // render as a string
};


// these functions are the interface cc.gr uses; this keeps
// it at arms' length from the data structure implementation
inline Type const *typeOf(CilExpr const *expr, Env *env)
  { return expr->getType(env); }
inline CilLval *asLval(CilExpr *expr) { return expr->asLval(); }
inline bool isLval(CilExpr const *expr) { return expr->isLval(); }

CilExpr *newIntLit(int val);
CilExpr *newUnaryExpr(UnaryOp op, CilExpr *expr);
CilExpr *newBinExpr(BinOp op, CilExpr *e1, CilExpr *e2);
CilExpr *newCastExpr(Type const *type, CilExpr *expr);
CilExpr *newAddrOfExpr(CilLval *lval);


// ------------------- CilLval -------------------
// an Lval can yield a value, like CilExpr, but it also is
// associated with a storage *location*, so that you can do
// things like take its address and modify its value
class CilLval : public CilExpr {
public:      // types
  enum LTag { T_VARREF, T_DEREF, T_FIELDREF, T_CASTL, T_ARRAYELT, NUM_LTAGS };

public:      // data
  LTag const ltag;         // tag ("L" for lval, so can still access CilExpr::tag)

  union {
    // T_VARREF
    struct {
      Variable *var;       // (serf) variable's entry in an environment
    } varref;

    // T_DEREF
    struct {
      CilExpr *addr;       // (owner) expr to compute addr to deref
    } deref;

    // T_FIELDREF
    struct {
      CilExpr *record;     // (owner) names the record itself (*not* its address)
      Variable *field;     // (serf) field entry in compound type's 'env'
    } fieldref;

    // T_CASTL
    struct {
      Type const *type;    // (serf) type being cast to
      CilLval *lval;       // (owner) lval being cast
    } castl;

    // T_ARRAYELT
    struct {
      CilExpr *array;      // (owner) names the array, not its address (for now, I'm *not* adopting C's idea that an array name is the same as its address (more later on this, maybe))
      CilExpr *index;      // (owner) integer index calculation
    } arrayelt;
  };

public:      // funcs
  CilLval(LTag tag);       // caller must fill in right fields
  ~CilLval();

  Type const *getType(Env *env) const;
  
  static void validate(LTag ltag);

  CilLval *clone() const;
  string toString() const;  
};

CilLval *newVarRef(Variable *var);
CilLval *newDeref(CilExpr *ptr);
CilLval *newFieldRef(CilExpr *record, Variable *field);
CilLval *newCastLval(Type const *type, CilLval *lval);
CilLval *newArrayAccess(CilExpr *array, CilExpr *index);


// ---------------------- CilInst ----------------
// an instruction is a runtime state transformer: by
// executing an instruction, some side effect occurs;
// here, I've chosen to mix simple imperatives and
// control flow constructs, since my intuition is that
// will be more uniform -- we'll see; CilInsts own all
// the CilExprs *and* CilInsts they point to
class CilInst {
public:      // types
  enum ITag {
    // declarations; another candidate for pulling out into
    // a separate thing
    T_VARDECL, T_FUNDECL,

    // simple imperatives
    T_ASSIGN, T_CALL,

    // control flow constructs ("loop"="while", "jump"="goto")
    T_COMPOUND, T_LOOP, T_IFTHENELSE, T_LABEL, T_JUMP, T_RET,

    NUM_ITAGS
  };

public:      // data
  ITag const itag;          // which kind of instruction

  union {
    // T_VARDECL: at the moment, we have exactly one variable
    // declaration in the Cil code for every declaration in the
    // original C code, and for every temporary introduced by
    // the translator
    struct {
      Variable *var;        // (serf) variable being declared
    } vardecl;

    // T_FUNDECL: the whole program is a CilCompound sequence
    // of these things ..
    struct {
      Variable *func;       // (serf) record of this fn in an environment
      CilInst *body;        // (owner) function body
    } fundecl;

    // T_ASSIGN
    struct {
      CilLval *lval;        // (owner) value being modified
      CilExpr *expr;        // (owner) value to put in 'lval'
    } assign;

    // T_CALL
    CilFnCall *call;        // (serf) equals 'this'

    // T_COMPOUND
    CilCompound *comp;      // (serf) equals 'this'

    // T_LOOP
    struct {
      CilExpr *cond;        // (owner) guard expr
      CilInst *body;        // (owner) loop body
    } loop;

    // T_IFTHENELSE
    struct {
      CilExpr *cond;        // (owner) guard expr
      CilInst *thenBr;      // (owner) "then" branch
      CilInst *elseBr;      // (owner) "else" branch
    } ifthenelse;

    // T_LABEL
    struct {
      LabelName *name;      // (owner) name of label
    } label;

    // T_JUMP
    struct {
      LabelName *dest;      // (owner) name of destination label
    } jump;

    // T_RET
    struct {
      CilExpr *expr;        // (owner, nullable) expr to return
    } ret;
  };

  // count and high-water allocated nodes
  static int numAllocd;
  static int maxAllocd;

public:      // funcs
  CilInst(ITag tag);        // caller fills in fields
  ~CilInst();

  static void validate(ITag tag);
  static void printAllocStats();

  // deep copy
  CilInst *clone() const;

  void printTree(int indent, ostream &os) const;
};

CilInst *newVarDecl(Variable *var);
CilInst *newFunDecl(Variable *func, CilInst *body);
CilInst *newAssign(CilLval *lval, CilExpr *expr);
CilInst *newWhileLoop(CilExpr *expr, CilInst *body);
CilInst *newIfThenElse(CilExpr *cond, CilInst *thenBranch, CilInst *elseBranch);
CilInst *newLabel(LabelName label);
CilInst *newGoto(LabelName label);
CilInst *newReturn(CilExpr *expr /*nullable*/);


class CilFnCall : public CilInst {
public:     // data
  CilLval *result;         // (owner) place to put the result
  CilExpr *func;           // (owner) expr to compute the fn to call
  ObjList<CilExpr> args;   // list of arguments

public:     // funcs
  CilFnCall(CilLval *result, CilExpr *expr);
  ~CilFnCall();

  CilFnCall *clone() const;
  void printTree(int indent, ostream &os) const;

  void appendArg(CilExpr *arg);
};

CilFnCall *newFnCall(CilLval *result, CilExpr *fn);    // args appended later


// I am considering replacing this with simple a rule like
// "i1 ; i2" and build all lists out of it ....
class CilInstructions {
public:    // data
  ObjList<CilInst> insts;    // list of instructions

public:    // funcs
  CilInstructions();
  ~CilInstructions();

  void append(CilInst *inst);
  void printTreeNoBraces(int indent, ostream &os) const;
};


// CilInst must be the first base class, because I cast
// from CilInst to CilCompound
class CilCompound : public CilInst, public CilInstructions {
public:    // funcs
  CilCompound();
  ~CilCompound();

  CilCompound *clone() const;
  void printTree(int indent, ostream &os) const;
};

CilCompound *newCompound();

#endif // CIL_H
