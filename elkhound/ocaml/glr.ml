(* glr.ml *)
(* GLR parser *)

open Arraystack      (* tArrayStack *)
open Objpool         (* tObjPool *)


(* NOTE: in some cases, more detailed comments can be found in
 * elkhound/glr.h, as these data structures mirror the ones
 * defined there *)
 
 
(* identifier for a state in the finite automaton *)
type tStateId = int


(* link from one stack node to another *)
type tSiblingLink = {
  (* stack node we're pointing at *)
  sib: tStackNode;

  (* semantic value on this link *)
  sval: tSemanticValue;

  (* TODO: source location *)

  (* possible TODO: yield count *)
}

(* node in the GLR graph-structured stack; all fields are
 * mutable because these are stored in a pool for explicit re-use *)
and tStackNode = {
  (* LR parser state when this node is at the top *)
  mutable state: tStateId;

  (* pointers to adjacent (to the left) stack nodes *)
  (* possible TODO: put links into a pool so I can deallocate them *)
  mutable leftSiblings: tSiblingLink list;

  (* logically first sibling in the sibling list; separated out
   * from 'leftSiblings' for performance reasons *)
  mutable firstSib: tSiblingLink;
  
  (* number of sibling links pointing at this node, plus the
   * number of worklists this node appears in *)
  mutable referenceCount: int;
  
  (* number of links we can follow to the left before hitting a node
   * that has more than one sibling *)
  mutable determinDepth: int;

  (* TODO: add the 'glr' context pointer when I encapsulate parsing
   * state for re-entrancy among other things *)

  (* position of token that was active when this node was created
   * (or pulled from pool); used in yield-then-merge calculations *)
  mutable column: int;
}

(* this is a path that has been queued for reduction;
 * all fields mutable to support pooling *)
type tPath = {
  (* rightmost state's id *)
  mutable startStateId: tStateId;

  (* production we're going to reduce with *)
  mutable prodIndex: int;

  (* column from leftmost stack node *)
  mutable startColumn: int;

  (* the leftmost stack node itself *)
  mutable leftEdgeNode: tStackNode;

  (* array of sibling links, i.e. the path; 0th element is
   * leftmost link *)
  sibLinks: tSiblingLink array ref;

  (* corresponding array of symbol ids to interpret svals *)
  symbols: tSymbolId array ref;

  (* next path in dequeueing order *)
  mutable next: tPath;
}

(* priority queue of reduction paths *)
type tReductionPathQueue = {
  (* head of the list, first to dequeue *)
  mutable top: tPath;
  
  (* pool of path objects *)
  pathPool: tPath tObjectPool;
  
  (* TODO: when tables are encapsulated, we need a pointer to them here *)
}

          
(* GLR parser object *)
type tGLR = {
  (* for debugging, so I can ask for token descriptions in places *)
  mutable lexerPtr: tLexerInterface;

  (* set of topmost parser nodes *)
  topmostParsers: tStackNode tArrayStack;

  (* treat this as a local variable of rwlProcessWorklist, included
   * here just to avoid unnecessary repeated allocation *)
  toPass: tSemanticValue array;

  (* swapped with 'topmostParsers' periodically, for performance reasons *)
  prevTopmost: tStackNode tArrayStack;

  (* node allocation pool; shared with innerGlrParse *)
  stackNodePool: tStackNode tObjectPool;

  (* reduction queue and pool *)
  pathQueue: tReductionPathQueue;
  
  (* when true, print some diagnosis of failed parses *)
  noisyFailedParse: bool;
  
  (* current token number *)
  globalNodeColumn: int;
  
  (* parser action statistics *)
  detShift: int;
  detReduce: int;
  nondetShift: int;
  nondetReduce: int;
}




