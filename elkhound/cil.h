// cil.h
// C Intermediate Language

#ifndef CIL_H
#define CIL_H

#include "str.h"       // string
#include "objlist.h"   // ObjList
#include "sobjlist.h"  // SObjList
#include "owner.h"     // Owner

// other files
class Type;            // cc_type.h
class FunctionType;    // cc_type.h
class CompoundType;    // cc_type.h
class Env;             // cc_env.h
class Variable;        // cc_env.h
class BBContext;       // stmt2bb.cc
class CCTreeNode;      // cc_tree.h
class SourceLocation;  // fileloc.h

// fwd for this file
class CilExpr;
class CilLval;
class CilFnCall;
class CilCompound;
class CilBB;
class CilBBSwitch;

// ------------- names --------------
typedef string VarName;     // TODO2: find a better place for this to live

typedef string LabelName;
LabelName newTmpLabel();


// ------------ CilThing -------------
// my problem now is I want to switch from storing CCTreeNode*
// to SourceLocation*, but that's syntactically painful; so here
// I name the type that will carry "extra" info for this purpose
typedef SourceLocation const *CilExtraInfo;

// I want to put pointers back into the parse tree
// in all these nodes; perhaps other things may be
// common to Cil things and could go here too
class CilThing {
private:
  // I switched to storing just the loc since, for the moment at
  // least, that's all I need, and the tree nodes get thrown away
  // after each function is processed (again, at least right now..)
  //CCTreeNode *treeNode;       // (serf) ptr to tree node that generated this
  SourceLocation const *_loc;   // (nullable serf) location of leftmost token

public:
  CilThing(CilExtraInfo tn);
  ~CilThing() {}

  string locString() const;
  string locMLString() const;
  string locComment() const;
  SourceLocation const *loc() const;   // can return NULL

  // for passing to constructors in clone()
  CilExtraInfo extra() const { return _loc; }
};


// ====================== Expression Language =====================
// --------------- operators ----------------
// TODO: for now, I don't distinguish e.g. unsigned
// and signed operations
enum BinOp {
  OP_PLUS, OP_MINUS, OP_TIMES, OP_DIVIDE, OP_MOD,
  OP_LSHIFT, OP_RSHIFT,
  OP_LT, OP_GT, OP_LTE, OP_GTE,
  OP_EQUAL, OP_NOTEQUAL,
  OP_BITAND, OP_BITXOR, OP_BITOR,
  //OP_AND, OP_OR,
  NUM_BINOPS
};

struct BinOpInfo {
  char const *text;                 // e.g. "+"
  char const *mlText;               // e.g. "Plus"
  int mlTag;                        // ml's tag value
};
BinOpInfo const &binOp(BinOp);
void validate(BinOp op);            // exception if out of range

inline char const *binOpText(BinOp op)    { return binOp(op).text; }
inline char const *binOpMLText(BinOp op)  { return binOp(op).mlText; }


enum UnaryOp { OP_NEGATE, OP_NOT, OP_BITNOT, NUM_UNOPS };

struct UnaryOpInfo {
  char const *text;                 // e.g. "!"
  char const *mlText;               // e.g. "LNot"
  int mlTag;                        // ml's tag value
};
UnaryOpInfo const &unOp(UnaryOp op);
void validate(UnaryOp op);

inline char const *unOpText(UnaryOp op)    { return unOp(op).text; }
inline char const *unOpMLText(UnaryOp op)  { return unOp(op).mlText; }


// -------------- CilExpr ------------
// an expression denotes a method for computing a value at runtime;
// this value need not have an associated "location" -- i.e. it need
// not be modifiable -- unlike Lvalues, below; CilExprs form trees,
// where parents own children; thus, there can be *no* sharing of
// subtrees between different CilExprs
class CilExpr : public CilThing {
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
  CilExpr(CilExtraInfo info, ETag tag);
  ~CilExpr();
  
  // we need the 'env' argument because we may have to
  // synthesize a new type (e.g. if this is an addr-of
  // expr, we'll need a ptr-to type)
  Type const *getType(Env *env) const;

  CilLval *asLval();
  bool isLval() const { return etag == T_LVAL; }

  // throw an exception if 'tag' is out of range
  static void validate(ETag tag);
  
  static void printAllocStats(bool anyway);

  CilExpr *clone() const;     // deep copy

  string toString() const;    // render as a string
  string toMLString() const;  // and as an ML string
};


// these functions are the interface cc.gr uses; this keeps
// it at arms' length from the data structure implementation
inline Type const *typeOf(CilExpr const *expr, Env *env)
  { return expr->getType(env); }
inline bool isLval(CilExpr const *expr) { return expr->isLval(); }
inline CilLval *asLval(CilExpr *expr) { return expr->asLval(); }

CilExpr *newIntLit(CilExtraInfo tn, int val);
CilExpr *newUnaryExpr(CilExtraInfo tn, UnaryOp op, CilExpr *expr);
CilExpr *newBinExpr(CilExtraInfo tn, BinOp op, CilExpr *e1, CilExpr *e2);
CilExpr *newCastExpr(CilExtraInfo tn, Type const *type, CilExpr *expr);
CilExpr *newAddrOfExpr(CilExtraInfo tn, CilLval *lval);


// ------------------- CilLval -------------------
// an Lval can yield a value, like CilExpr, but it also is
// associated with a storage *location*, so that you can do
// things like take its address and modify its value
class CilLval : public CilExpr {
public:      // types
  enum LTag {
    // these are the ones the from-C translator uses
    T_VARREF, T_DEREF, T_FIELDREF, T_CASTL, T_ARRAYELT,
    
    // these are the ones for outputting Cil
    //T_VAROFS, T_DEREFOFS,

    NUM_LTAGS
  };

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
      CilLval *record;     // (owner) names the record itself (*not* its address)
      Variable *field;     // (serf) field entry in compound type's 'env'
      CompoundType const *recType;  // (serf) type of 'record' (possibly temporary hack)
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
  CilLval(CilExtraInfo tn, LTag tag);       // caller must fill in right fields
  ~CilLval();

  Type const *getType(Env *env) const;

  static void validate(LTag ltag);

  CilLval *clone() const;
  string toString() const;
  string toMLString() const;  
};

CilLval *newVarRef(CilExtraInfo tn, Variable *var);
CilLval *newDeref(CilExtraInfo tn, CilExpr *ptr);
CilLval *newFieldRef(CilExtraInfo tn, CilLval *record, Variable *field,
                     CompoundType const *recType);
CilLval *newCastLval(CilExtraInfo tn, Type const *type, CilLval *lval);
CilLval *newArrayAccess(CilExtraInfo tn, CilExpr *array, CilExpr *index);

// lile 'newVarRef', except it's allowed to replace
// references to constants with the associated literal
CilExpr *newVarRefExpr(CilExtraInfo tn, Variable *var);


// ====================== Instruction Language =====================
// ---------------------- CilInst ----------------
// an instruction is a runtime state transformer: by
// executing an instruction, some side effect occurs;
// here, I've chosen to mix simple imperatives and
// control flow constructs, since my intuition is that
// will be more uniform -- we'll see; CilInsts own all
// the CilExprs *and* CilInsts they point to
class CilInst : public CilThing {
public:      // types
  enum ITag {
    // simple imperatives
    T_ASSIGN, T_CALL,

    NUM_ITAGS
  };

public:      // data
  ITag const itag;          // which kind of instruction

  union {
    // T_ASSIGN
    struct {
      CilLval *lval;        // (owner) value being modified
      CilExpr *expr;        // (owner) value to put in 'lval'
    } assign;

    // T_CALL
    CilFnCall *call;        // (serf) equals 'this'
  };

  // count and high-water allocated nodes
  static int numAllocd;
  static int maxAllocd;

public:      // funcs
  CilInst(CilExtraInfo tn, ITag tag);        // caller fills in fields
  ~CilInst();

  static void validate(ITag tag);
  static void printAllocStats(bool anyway);

  // deep copy
  CilInst *clone() const;

  void printTree(int indent, ostream &os, bool ml) const;
};

CilInst *newAssignInst(CilExtraInfo tn, CilLval *lval, CilExpr *expr);


class CilFnCall : public CilInst {
public:     // data
  CilLval *result;         // (owner, nullable) place to put the result
                           // NULL if fn return value ignored
  CilExpr *func;           // (owner) expr to compute the fn to call
  ObjList<CilExpr> args;   // list of arguments

public:     // funcs
  CilFnCall(CilExtraInfo tn, CilLval *result, CilExpr *expr);
  ~CilFnCall();

  CilFnCall *clone() const;
  void printTree(int indent, ostream &os, bool ml) const;

  void appendArg(CilExpr *arg);
};

CilFnCall *newFnCall(CilExtraInfo tn, CilLval *result, CilExpr *fn);    // args appended later


// ====================== Statement Language =====================
// ---------------- CilStmt ----------------------
// the statement language is a tree of statements and
// instructions; instructions are leaves, and statements
// are both internal nodes and leaves; statements encode
// flow of control
class CilStmt : public CilThing {
public:      // types
  enum STag {
    // control flow constructs ("loop"="while", "jump"="goto")
    T_COMPOUND, T_LOOP, T_IFTHENELSE, T_LABEL, T_JUMP, T_RET,
    T_SWITCH, T_CASE, T_DEFAULT,

    // simple imperative
    T_INST,

    NUM_STAGS
  };

public:      // data
  STag const stag;          // which kind of statement

  union {
    // T_COMPOUND
    CilCompound *comp;      // (serf) equals 'this'

    // T_LOOP
    struct {
      CilExpr *cond;        // (owner) guard expr
      CilStmt *body;        // (owner) loop body
    } loop;

    // T_IFTHENELSE
    struct {
      CilExpr *cond;        // (owner) guard expr
      CilStmt *thenBr;      // (owner) "then" branch
      CilStmt *elseBr;      // (owner) "else" branch
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

    // T_SWITCH
    struct {
      CilExpr *expr;        // (owner) switch expression
      CilStmt *body;        // (owner) instruction block with case/default labels
    } switchStmt;

    // T_CASE
    struct {
      int value;            // case expr value
    } caseStmt;

    // T_DEFAULT
    struct {                // no fields
    } defaultStmt;

    // T_INST
    struct {
      CilInst *inst;        // simple instruction
    } inst;
  };

  // count and high-water allocated nodes
  static int numAllocd;
  static int maxAllocd;

public:      // funcs
  CilStmt(CilExtraInfo tn, STag stag);        // caller fills in fields
  ~CilStmt();

  static void validate(STag stag);
  static void printAllocStats(bool anyway=true);

  // deep copy
  CilStmt *clone() const;

  void printTree(int indent, ostream &os, bool ml,
                 char const *mlLineEnd = "\n") const;
  // translaton
  CilBB * /*owner*/ translateToBB(BBContext &ctxt,
                                  CilBB * /*owner*/ next) const;
};

CilStmt *newWhileLoop(CilExtraInfo tn, CilExpr *expr, CilStmt *body);
CilStmt *newIfThenElse(CilExtraInfo tn, CilExpr *cond, CilStmt *thenBranch, CilStmt *elseBranch);
CilStmt *newLabel(CilExtraInfo tn, LabelName label);
CilStmt *newGoto(CilExtraInfo tn, LabelName label);
CilStmt *newReturn(CilExtraInfo tn, CilExpr *expr /*nullable*/);
CilStmt *newSwitch(CilExtraInfo tn, CilExpr *expr, CilStmt *body);
CilStmt *newCase(CilExtraInfo tn, int val);
CilStmt *newDefault(CilExtraInfo tn);
CilStmt *newInst(CilExtraInfo tn, CilInst *inst);

// since it's common, here's an assign that wraps
// up its arguments into a CilStmt
CilStmt *newAssign(CilExtraInfo tn, CilLval *lval, CilExpr *expr);


// sequential list of statements
class CilStatements {
public:    // data
  ObjList<CilStmt> stmts;    // list of statements

public:    // funcs
  CilStatements();
  ~CilStatements();

  void append(CilStmt *inst);
  void printTreeNoBraces(int indent, ostream &os) const;
  bool isEmpty() const { return stmts.isEmpty(); }
};


// CilStmt must be the first base class, because I cast
// from CilInst to CilCompound
class CilCompound : public CilStmt, public CilStatements {
public:    // funcs
  CilCompound(CilExtraInfo tn);
  ~CilCompound();

  CilCompound *clone() const;
  void printTree(int indent, ostream &os, bool ml,
                 char const *mlLineEnd = "\n") const;
  CilBB * /*owner*/ translateToBB(BBContext &ctxt, 
                                  CilBB * /*owner*/ next) const;
};

CilCompound *newCompound(CilExtraInfo tn);


// ====================== Basic Block Language =====================
// a basic block is a sequence of instructions (which lack
// any control flow except for simple sequencing) followed
// by one or more outgoing control flow edges; when there
// are multiple outgoing edges, the edge to choose is controlled
// by associated guard expressions
class CilBB {
public:     // types
  enum BTag {
    T_RETURN, T_IF, T_SWITCH, T_JUMP,
    NUM_BTAGS
  };

private:    // data
  static int nextId;            // trivial id assignment for now

public:     // data
  int id;                       // breadth-first identifier (?)
  SObjList<CilInst> insts;      // list of instructions (owned by CilStmt tree)
  BTag const btag;              // tag for successor information

  union {
    // T_RETURN
    struct {
      CilExpr *expr;            // (nullable serf) expression to return
    } ret;

    // T_IF
    struct {
      CilExpr *expr;            // (serf) test expression
      CilBB *thenBB;            // (owner) BB for when expr is true
      CilBB *elseBB;            // (owner) BB for when expr is false
      bool loopHint;            // true when this is part of a while loop
    } ifbb;

    // T_SWITCH
    CilBBSwitch *switchbb;      // (serf) points to 'this'

    // T_JUMP
    struct {
      CilBB *nextBB;            // (nullable serf) BB to go to next; NULL for fix-up
      VarName *targetLabel;     // (nullable owner) for jumps to fix-up
    } jump;
  };

  static int numAllocd;
  static int maxAllocd;

public:     // funcs
  CilBB(BTag btag);             // fill in fields after ctor
  ~CilBB();

  void printTree(int indent, ostream &os) const;

  static void validate(BTag btag);
  static void printAllocStats(bool anyway=true);
};

CilBB *newJumpBB(CilBB * /*serf*/ target);


// a single case of a switch block
class BBSwitchCase {
public:
  int value;                      // case <value>
  Owner<CilBB> code;              // code to execute for this case

public:
  BBSwitchCase(int v, CilBB * /*owner*/ c)
    : value(v), code(c) {}
  ~BBSwitchCase();
};


// basic block with more than two outgoing edges
class CilBBSwitch : public CilBB {
public:
  CilExpr *expr;                  // (serf) expression to switch upon
  ObjList<BBSwitchCase> exits;    // exits associated with values
  Owner<CilBB> defaultCase;       // what to do when no case matches

public:
  CilBBSwitch(CilExpr * /*serf*/ expr);
  ~CilBBSwitch();

  void printTree(int indent, ostream &os) const;
};



// ====================== Program Language =====================
// ---------------------- CilFnDefn -----------------
// a function definition -- name, type, and code
class CilFnDefn : public CilThing {
public:
  Variable *var;               // (serf) name, type
  CilCompound bodyStmt;        // fn body code as statements
  SObjList<Variable> locals;   // local variables

  ObjList<CilBB> bodyBB;       // body code as basic blocks; order insignificant
  CilBB *startBB;              // (serf) starting basic block

public:
  CilFnDefn(CilExtraInfo tn, Variable *v)
    : CilThing(tn), var(v), bodyStmt(tn) {}
  ~CilFnDefn();

  // stmts==true: print statements
  // stmts==false: print basic blocks
  void printTree(int indent, ostream &os, bool stmts, bool ml) const;
};


// ------------------ CilProgram ------------------
// an entire Cil program
class CilProgram {
public:
  SObjList<Variable> globals;  // global variables
  ObjList<CilFnDefn> funcs;    // function definitions

public:
  CilProgram();
  ~CilProgram();

  void printTree(int indent, ostream &os, bool ml) const;
  void empty();
};


// ------------------ CilContext --------------
// context to carry during translation from
// C/C++ to CilStmt
class CilContext {
public:
  // program we're translating
  CilProgram *prog;           // (serf)

  // function we're translating, if any
  CilFnDefn *fn;              // (nullable serf)

  // statements to execute before the current expression's
  // value can be returned; when we translate an expression
  // with a side effect, it puts its side effects here
  CilStatements *stmts;       // (nullable serf)

  // function call being constructed
  CilFnCall *call;            // (nullable serf)

  // true if we're only doing a trial analysis, and
  // therefore should avoid doing things that have
  // side effects
  bool isTrial;

public:
  CilContext(CilProgram &p)
    : prog(&p), fn(NULL), stmts(NULL), call(NULL), isTrial(false) {}
    
  // this is used in places where, because of C syntax, I can be sure
  // there are no global declarations, so 'prog' is not used (and I
  // don't have easy access to a proper context -- no point in doing
  // this if I don't have to)
  CilContext(CilFnDefn &f, int)     // the 'int' is to avoid implicit calls
    : prog(NULL), fn(&f), stmts(&f.bodyStmt), call(NULL), isTrial(false) {}

  CilContext(CilContext const &ctxt, CilFnDefn &f)
    : prog(ctxt.prog), fn(&f), stmts(&f.bodyStmt), call(NULL), isTrial(ctxt.isTrial) {}
  CilContext(CilContext const &ctxt, CilStatements &s)
    : prog(ctxt.prog), fn(ctxt.fn), stmts(&s), call(NULL), isTrial(ctxt.isTrial) {}
  CilContext(CilContext const &ctxt, CilFnCall &c)
    : prog(ctxt.prog), fn(ctxt.fn), stmts(ctxt.stmts), call(&c), isTrial(ctxt.isTrial) {}

  // constness here is just a syntactic tweak so that
  // egcs won't compilain when I construct CilContexts
  // in-place ..
  void append(CilStmt * /*owner*/ s) const;
  void addVarDecl(Variable *var) const;
};


#endif // CIL_H
