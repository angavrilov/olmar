// stmt2bb.cc
// code for stmt2bb.h

#include "stmt2bb.h"      // this module
#include "trace.h"        // tracingSys
#include "exc.h"          // xBase
#include "strsobjdict.h"  // StringSObjDict


// parts of translation context for Stmt -> BB
// that don't change if we enter branch target scopes
class BBTopContext {
public:
  CilFnDefn &fn;                   // function being translated
  StringSObjDict<CilBB> labels;    // map: label -> basic block
  SObjList<CilBB> gotos;           // gotos that need to be fixed up

public:
  BBTopContext(CilFnDefn &f)
    : fn(f) {}
};
                                               

// entire context, including things that change
class BBContext {
public:
  BBTopContext &top;        // invariant context
  
  CilBB *breakTarget;       // (nullable serf) where to go on 'nreak'
  CilBB *continueTarget;    // (nullable serf) .............. 'continue'

public:
  BBContext(BBTopContext &t)
    : top(t), breakTarget(NULL), continueTarget(NULL) {}
  BBContext(BBContext &c);

  // add 'bb' to fn's basic-block list
  void appendBB(CilBB *bb);

  // delete a basic block that's dead
  // (for now, don't delete, because there could be pointers
  // to nodes due to 'gotos')
  void deadCodeElim(CilBB *bb)
    { appendBB(bb); }
};


BBContext::BBContext(BBContext &obj)
  : DMEMB(top),
    DMEMB(breakTarget),
    DMEMB(continueTarget)
{}


void BBContext::appendBB(CilBB *bb)
{
  // turns out prepend prints out nicer
  top.fn.bodyBB.prepend(bb);
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
      CilBB *cond = new CilBB(CilBB::T_IF);
      cond->ifbb.expr = loop.cond;
      cond->ifbb.loopHint = true;
      ctxt.appendBB(cond);

      // make another basic block for the end of the loop
      Owner<CilBB> body; body = new CilBB(CilBB::T_JUMP);
      body->jump.nextBB = cond;      // loops back to condition

      // make a new context so we can say where the 'break'
      // and 'continue' targets are
      BBContext bodyCtxt(ctxt);
      bodyCtxt.breakTarget = next;
      bodyCtxt.continueTarget = cond;

      // translate the loop body
      body = loop.body->translateToBB(ctxt, body.xfr());

      // connect the 'then' branch of conditional to the body
      cond->ifbb.thenBB = body.xfr();

      // connect the 'else' branch of conditional to whatever
      // was scheduled to follow the loop as a whole
      cond->ifbb.elseBB = next.xfr();

      // if someone wants to jump to me, jump to the conditional
      // (need to break the basic block because of the jump
      // target for the while-continue)
      return newJumpBB(cond);
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
      if (ctxt.top.labels.isMapped(name)) {
        xfailure(stringc << "duplicate label: " << name);
      }
      CilBB *nextSerf = next;
      ctxt.top.labels.add(name, nextSerf);

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

      if (ctxt.top.labels.isMapped(name)) {
        // start a new 'jump' basic block with the known target
        return newJumpBB(ctxt.top.labels.queryf(name));
      }
      else {
        // start a new block, but leave the target empty; we'll
        // get that on a post-process
        Owner<CilBB> jmp; jmp = newJumpBB(NULL);
        jmp->jump.targetLabel = new VarName(name);

        // add it to the post-process list
        ctxt.top.gotos.append(jmp);

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
  BBTopContext topCtxt(fn);
  BBContext ctxt(topCtxt);

  // recursive translation
  Owner<CilBB> start;
  start = fn.bodyStmt.translateToBB(ctxt, final.xfr());
  fn.startBB = start;
  fn.bodyBB.prepend(start.xfr());

  // fix up the gotos
  SMUTATE_EACH_OBJLIST(CilBB, topCtxt.gotos, iter) {
    CilBB *jmp = iter.data();
    string label = *( jmp->jump.targetLabel );
    if (topCtxt.labels.isMapped(label)) {
      jmp->jump.nextBB = topCtxt.labels.queryf(label);
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

