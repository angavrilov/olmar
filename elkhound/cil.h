// cil.h
// C Intermediate Language

// blah ..
                     
typedef string VarName;
VarName newTmpVar();

typedef string LabelName
LabelName newTmpLabel();

class CilLval {};

class CilExpr {};
Type *typeOf(CilExpr *expr);

CilLval asLval(CilExpr *expr);
bool isLval(CilExpr *expr);

CilExpr *newIntLit(int val);

enum BinOp { OP_PLUS, OP_MINUS, OP_TIMES, OP_DIVIDE,
             OP_MOD, OP_LSHIFT, OP_RSHIFT,
             OP_LT, OP_GT, OP_LTE, OP_GTE,
             OP_EQUAL, OP_NOTEQUAL,
             OP_BITAND, OP_BITXOR, OP_BITOR,
             OP_AND, OP_OR };
CilExpr *newBinExpr(BinOp op, CilExpr *e1, CilExpr *e2);

CilExpr *newVarRef(VarName var);
CilExpr *newAddrOfExpr(CilLval *lval);

enum UnaryOp { OP_NEGATE, OP_NOT, OP_BITNOT };
CilExpr *newUnaryExpr(UnaryOp op, CilExpr *expr);

CilExpr *newCastExpr(Type *type, CilExpr *expr);
CilLval *newCastLval(Type *type, CilExpr *expr);


class CilFnCall : public CilExpr {
public:
  void appendArg(CilExpr *arg);
};

class CilInst {};
CilInst *newVarDecl(VarName var, Type *type);
CilInst *newAssign(CilLval *lval, CilExpr *expr);
CilInst *newLabel(LabelName label);
CilInst *newGoto(LabelName label);
CilInst *newWhileLoop(CilExpr *expr, CilInst *body);
CilInst *newReturn(CilExpr *expr /*nullable*/);

class CilInstructions {
public:
  void append(CilInst *inst);
};

class CilCompound : public CilInst, public CilInstructions {
public:                
  // deep copy
  CilCompound *clone() const;

};
CilCompound *newCilCompound();


