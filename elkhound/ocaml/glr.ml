(* glr.ml *)
(* GLR parser *)
(* based on elkhound/glr.h and elkhound/glr.cc *)

open Arraystack      (* tArrayStack *)
open Objpool         (* tObjPool *)
open Useract         (* tSemanticValue *)
open Lexerint        (* tLexerInterface *)
open Parsetables     (* action/goto/etc. *)


(* NOTE: in some cases, more detailed comments can be found in
 * elkhound/glr.h, as these data structures mirror the ones
 * defined there *)


(* identifier for a state in the finite automaton *)
type tStateId = int
let cSTATE_INVALID = -1

(* identifier for a symbol *)
type tSymbolId = int


(* link from one stack node to another *)
type tSiblingLink = {
  (* stack node we're pointing at *)
  mutable sib: tStackNode option (*ouch!*);

  (* semantic value on this link *)
  mutable sval: tSemanticValue;

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

  (* for access to parser context in a few unusual situations *)
  mutable glr: tGLR;

  (* position of token that was active when this node was created
   * (or pulled from pool); used in yield-then-merge calculations *)
  mutable column: int;
}

(* this is a path that has been queued for reduction;
 * all fields mutable to support pooling *)
and tPath = {
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
and tReductionPathQueue = {
  (* head of the list, first to dequeue *)
  mutable top: tPath;

  (* pool of path objects *)
  pathPool: tPath tObjectPool;

  (* TODO: when tables are encapsulated, we need a pointer to them here *)
}


(* GLR parser object *)
and tGLR = {
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



(* what follows is based on elkhound/glr.cc *)


(* -------------------- misc constants --------------------- *)
(* maximum RHS length *)
let cMAX_RHSLEN = 30

let cTYPICAL_MAX_REDUCTION_PATHS = 5

let cINITIAL_RHSLEN_SIZE = 10


(* ------------------ generic utilities --------------------- *)
let isEmpty (lst: 'a list) : bool =
begin
  match lst with
  | _ :: _ -> false
  | _ -> true
end

let isNotEmpty (lst: 'a list) : bool =
begin
  (not (isEmpty lst))
end

let isNone (o: 'a option) : bool =
begin
  match o with
  | None -> true
  | _ -> false
end

let isSome (o: 'a option) : bool =
begin
  (not (isNone o))
end

let getSome (o: 'a option) : 'a =
begin
  match o with
  | None -> (failwith "getSome applied to None")
  | Some(v) -> v
end

(* true if 'o' is a Some, and it equals (==) 'v' *)
let someEquals (o: 'a option) (v: 'a) : bool =
begin
  match o with
  | None -> false
  | Some(v2) -> v2 == v
end

let xassertdb (b: bool) : unit =
begin
  if (not b) then (
    (failwith "assertion failure")
  );
end

let xassert (b: bool) : unit =
begin
  if (not b) then (
    (failwith "assertion failure")
  );
end


(* ----------------- front ends to user code --------------- *)
let symbolDescription (sym: tSymbolId) (*user*) (sval: tSemanticValue) : string =
begin
  "TODO"
end

let duplicateSemanticValue (glr: tGLR) (sym: tSymbolId) (sval: tSemanticValue)
  : tSemanticValue =
begin
  (* TODO *)
  sval
end

let deallocateSemanticValue (sym: tSymbolId) (*user*) (sval: tSemanticValue) : unit =
begin
  (* TODO *)
end


(* --------------------- SiblingLink ----------------------- *)
let makeSiblingLink (s: tStackNode option) (sv: tSemanticValue) : tSiblingLink =
begin
  { sib=s; sval=sv; }
end


(* --------------------- StackNode -------------------------- *)
let emptyStackNode(g : tGLR) : tStackNode =
begin
  { state = cSTATE_INVALID;
    leftSiblings = [];
    firstSib = (makeSiblingLink None (Obj.repr 0));
    referenceCount = 0;
    determinDepth = 0;
    glr = g;
    column = 0 }
end

let getNodeSymbol (ths: tStackNode) : tSymbolId =
begin
  stateSymbol.(ths.state)
end

let deallocSemanticValues (ths: tStackNode) : unit =
begin
  if (isSome ths.firstSib.sib) then (
    (deallocateSemanticValue (getNodeSymbol ths) ths.firstSib.sval)
  );

  (List.iter
    (fun s -> (deallocateSemanticValue (getNodeSymbol ths) s.sval))
    ths.leftSiblings);
  ths.leftSiblings <- [];
end

let initStackNode (ths: tStackNode) (st: tStateId) : unit =
begin
  ths.state <- st;
  (xassertdb (isEmpty ths.leftSiblings));
  (xassertdb (isNone ths.firstSib.sib));
  ths.referenceCount <- 0;
  ths.determinDepth <- 1;
end

let deinitStackNode (ths: tStackNode) : unit =
begin
  (deallocSemanticValues ths);
  ths.firstSib.sib <- None;
end

let incRefCt (ths: tStackNode) : unit =
begin
  ths.referenceCount <- ths.referenceCount + 1;
end

let decRefCt (ths: tStackNode) : unit =
begin
  (xassert (ths.referenceCount > 0));

  ths.referenceCount <- ths.referenceCount - 1;
  if (ths.referenceCount = 0) then (
    (ths.glr.stackNodePool#dealloc ths)
  )
end

let hasZeroSiblings (ths: tStackNode) : bool =
begin
  (isNone ths.firstSib.sib)
end

let hasOneSibling (ths: tStackNode) : bool =
begin
  (isSome ths.firstSib.sib) && (isEmpty ths.leftSiblings)
end

let hasMultipleSiblings (ths: tStackNode) : bool =
begin
  (isNotEmpty ths.leftSiblings)
end

let addFirstSiblingLink_noRefCt (ths: tStackNode) (leftSib: tStackNode)
                                (sval: tSemanticValue) : unit =
begin
  (xassertdb (hasZeroSiblings ths));

  ths.determinDepth <- leftSib.determinDepth + 1;

  (xassertdb (isNone ths.firstSib.sib));
  ths.firstSib.sib <- (Some leftSib);     (* update w/o refct *)

  ths.firstSib.sval <- sval;
end

let addAdditionalSiblingLink (ths: tStackNode) (leftSib: tStackNode)
                             (sval: tSemanticValue) : tSiblingLink =
begin
  (* now there is a second outgoing pointer *)
  ths.determinDepth <- 0;

  let link:tSiblingLink = (makeSiblingLink (Some leftSib) sval) in
  ths.leftSiblings <- link :: ths.leftSiblings;
  link
end

let addSiblingLink (ths: tStackNode) (leftSib: tStackNode)
                   (sval: tSemanticValue) : tSiblingLink =
begin
  if (isNone ths.firstSib.sib) then (
    (addFirstSiblingLink_noRefCt ths leftSib sval);

    (* manually inc refct *)
    (incRefCt leftSib);

    (* pointer to firstSib.. *)
    ths.firstSib
  )
  else (
    (addAdditionalSiblingLink ths leftSib sval)
  )
end

let getUniqueLink (ths: tStackNode) : tStackNode =
begin
  (xassert (hasOneSibling ths));
  (getSome ths.firstSib.sib)
end

let getLinkTo (ths: tStackNode) (another: tStackNode) : tStackNode option =
begin
  (* first? *)
  if (someEquals ths.firstSib.sib another) then (
    ths.firstSib.sib
  )
   
  else (
    (* rest? *)
    try
      let link:tSiblingLink = (List.find
        (fun candidate -> (someEquals candidate.sib another))
        ths.leftSiblings) in
      link.sib
    with Not_found ->
      None
  )
end

(* printAllocStats goes here *)

let computeDeterminDepth (ths: tStackNode) : int =
begin
  if (hasZeroSiblings ths) then (
    1
  )
  else if (hasOneSibling ths) then (
    (* sibling's plus one *)
    (getSome ths.firstSib.sib).determinDepth + 1
  )
  else (
    (xassert (hasMultipleSiblings ths));
    0
  )
end

let checkLocalInvariants (ths: tStackNode) : unit =
begin
  (xassertdb ((computeDeterminDepth ths) = ths.determinDepth));
end


(* ----------------------- stack node list ops ---------------------- *)
let decParserList (lst: tStackNode tArrayStack) : unit =
begin
  (lst#iter (fun n -> (decRefCt n)))
end

let incParserList (lst: tStackNode tArrayStack) : unit =
begin
  (lst#iter (fun n -> (incRefCt n)))
end

let parserListContains (lst: tStackNode tArrayStack) (n: tStackNode) : bool =
begin
  (lst#contains (fun n2 -> n2 == n))
end



















(* EOF *)
