// stmt2bb.cc
// code for stmt2bb.h

#include "stmt2bb.h"      // this module
#include "trace.h"        // tracingSys
#include "exc.h"          // xBase
#include "strsobjdict.h"  // StringSObjDict


// translation context for Stmt -> BB
class BBContext {
public:
  CilFnDefn &fn;                   // function being translated
  StringSObjDict<CilBB> labels;    // map: label -> basic block
  SObjList<CilBB> gotos;           // gotos that need to be fixed up

public:
  BBContext(CilFnDefn &f)
    : fn(f) {}

  //BBContext(BBContext &c, CilBB *newNext)
  //  : f(c.fn), next(newNext) {}

  // add 'bb' to fn's basic-block list
  void appendBB(CilBB *bb);
  
  // delete a basic block that's dead
  // (for now, don't delete, because there could be pointers
  // to nodes due to 'gotos')
  void deadCodeElim(CilBB *bb)
    { appendBB(bb); }
};

void BBContext::appendBB(CilBB *bb)
{
  // turns out prepend prints out nicer
  fn.bodyBB.prepend(bb);
}



CilBB * /*owner*/ CilStmt::
  translateToBB(BBContext &ctxt, CilBB * /*owner*/ _next) const
{
  Owner<CilBB> next; next = _next;

  switch (stag) {
    default:
      xfailure("bad tag");

    case T_COMPOUND:
      return comp->translateToBB(ctxt, next.xfr());

    case T_LOOP: {
      // (note that since I duplicated code to get 'expr' side-effect
      // free, yet I could have dealt with it here, there is some
      // inefficiency in the composition of the translations)

      // make a basic block to hold the condition evaluation
      Owner<CilBB> cond; cond = new CilBB(CilBB::T_IF);
      cond->ifbb.expr = loop.cond;
      cond->ifbb.loopHint = true;

      // make another basic block for the end of the loop
      Owner<CilBB> bodyEnd; bodyEnd = new CilBB(CilBB::T_JUMP);
      bodyEnd->jump.nextBB = cond;      // loops back to condition

      // translate the loop body
      Owner<CilBB> body;
      body = loop.body->translateToBB(ctxt, bodyEnd.xfr());

      // connect the 'then' branch of conditional to the body
      cond->ifbb.thenBB = body.xfr();

      // connect the 'else' branch of conditional to whatever
      // was scheduled to follow the loop as a whole
      cond->ifbb.elseBB = next.xfr();

      // if someone wants to jump to me, jump to the conditional
      // (need to break the basic block because of the jump
      // target for the while-continue)
      CilBB *condSerf = cond;
      ctxt.appendBB(cond.xfr());
      return newJumpBB(condSerf);
    }

    case T_IFTHENELSE: {
      // make a basic block to hold the condition evaluation
      Owner<CilBB> cond; cond = new CilBB(CilBB::T_IF);
      cond->ifbb.expr = ifthenelse.cond;
      cond->ifbb.loopHint = false;

      // translate the 'then' branch, and connect the conditional
      // to it; after the 'then' branch, we go wherever we go
      // after the 'if' as a whole
      cond->ifbb.thenBB =
        ifthenelse.thenBr->translateToBB(ctxt, newJumpBB(next));

      // similarly for the 'else' branch
      cond->ifbb.elseBB =
        ifthenelse.elseBr->translateToBB(ctxt, newJumpBB(next));

      // since neither branch owned the 'next' target, add it
      // to the function-wide list
      ctxt.appendBB(next.xfr());

      // if someone wants to jump to me, jump to the conditional
      return cond.xfr();
    }

    case T_LABEL: {
      string name = *(label.name);

      // whatever is 'next' gets assigned this label
      if (ctxt.labels.isMapped(name)) {
        xfailure(stringc << "duplicate label: " << name);
      }
      CilBB *nextSerf = next;
      ctxt.labels.add(name, nextSerf);

      // add this block to the global list
      ctxt.appendBB(next.xfr());

      // start a new 'previous' block, ending in a jump
      // to this one
      return newJumpBB(nextSerf);
    }

    case T_JUMP: {
      string name = *(jump.dest);

      // the 'next' basic block is excusively owned,
      // but we're not going to go to it; thus, we
      // can throw it away entirely (it's dead code)
      ctxt.deadCodeElim(next.xfr());

      if (ctxt.labels.isMapped(name)) {
        // start a new 'jump' basic block with the known target
        return newJumpBB(ctxt.labels.queryf(name));
      }
      else {
        // start a new block, but leave the target empty; we'll
        // get that on a post-process
        Owner<CilBB> jmp; jmp = newJumpBB(NULL);
        jmp->jump.targetLabel = new VarName(name);

        // add it to the post-process list
        ctxt.gotos.append(jmp);

        // and this is where prior code goes next
        return jmp.xfr();
      }
    }

    case T_RET: {
      // won't do anything with next
      ctxt.appendBB(next.xfr());

      Owner<CilBB> r; r = new CilBB(CilBB::T_RETURN);
      r->ret.expr = ret.expr;
      return r.xfr();
    }

    case T_SWITCH: {
      #if 0
      // make a switch bb
      Owner<CilBBSwitch> sw; sw = new CilBBSwitch(switchStmt.expr);
      #endif

      xfailure("unimplemented");
    }

    case T_CASE:
    case T_DEFAULT:
      xfailure("unimplemented");

    case T_INST:
      // simply prepend this instruction onto wherever
      // we're going next
      next->insts.prepend(inst.inst);
      return next.xfr();
  }
}


CilBB *CilCompound::
  translateToBB(BBContext &ctxt, CilBB * /*owner*/ next) const
{
  // translate in reverse order
  int len = stmts.count();
  for (int i=len-1; i>=0; i--) {
    // translate; after this statement we go wherever 'ctxt' was
    // going; for subsequent statements, we go to the last translated
    // statement
    next = stmts.nthC(i)->translateToBB(ctxt, next);
  }

  // callers wanting to jump to our first instruction
  // should jump to what we've ultimately marked as 'next'
  // (which is really the first instruction)
  return next;
}


void translateStmtToBB(CilFnDefn &fn)
{
  // dummy final node
  Owner<CilBB> final; final = new CilBB(CilBB::T_RETURN);
  final->ret.expr = NULL;

  // build a translation context
  BBContext ctxt(fn);

  // recursive translation
  Owner<CilBB> start;
  start = fn.bodyStmt.translateToBB(ctxt, final.xfr());
  fn.startBB = start;
  fn.bodyBB.prepend(start.xfr());
  
  // fix up the gotos
  SMUTATE_EACH_OBJLIST(CilBB, ctxt.gotos, iter) {
    CilBB *jmp = iter.data();
    string label = *( jmp->jump.targetLabel );
    if (ctxt.labels.isMapped(label)) {
      jmp->jump.nextBB = ctxt.labels.queryf(label);
      delete jmp->jump.targetLabel;
      jmp->jump.targetLabel = NULL;
    }
    else {
      xfailure(stringc << "undefined label: " << label);
    }
  }
}


void doTranslationStuff(CilFnDefn &fn)
{
  if (tracingSys("cil-bb")) {
    try {
      translateStmtToBB(fn);
      fn.printTree(0 /*indent*/, cout, false /*stmts*/);
    }
    catch (xBase &x) {
      cout << "translation error: " << x << endl;
    }
  }
}

