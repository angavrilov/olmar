(* parsetables.ml *)
(* representation of parsing tables *)
(* based on elkhound/parsetables.h *)


(* for action entries; some places may still be called int *)
type tActionEntry = int

(* identifier for a state in the finite automaton *)
type tStateId = int
let cSTATE_INVALID = -1

(* entry in the goto table *)
type tGotoEntry = int


(* for now, some actual parsing tables *)

(* ------------------- parse tables (from arith.gr.gen.cc) -------------------- *)
let numTerms = 8
let numNonterms = 4
let numStates = 16
let numProds = 8
let actionCols = 8
let actionRows = 16
let gotoCols = 4
let gotoRows = 16
(*ambigTableSize*)
(*startState*)
let finalProductionIndex = 0
(*bigProductionListSize*)
(*errorBitsRowSize*)
(*uniqueErrorRows*)

let actionTable = [|    (* 128 elements *)
  (* 0*) 0; 3; 0; 0; 0; 0; 8; 0;
  (* 1*) 0; 0; 0; 0; 0; 0; 0; 0;
  (* 2*) -6; 0; -6; -6; -6; -6; 0; -6;
  (* 3*) 0; 3; 0; 0; 0; 0; 8; 0;
  (* 4*) 0; 3; 0; 0; 0; 0; 8; 0;
  (* 5*) 0; 3; 0; 0; 0; 0; 8; 0;
  (* 6*) 0; 3; 0; 0; 0; 0; 8; 0;
  (* 7*) 0; 3; 0; 0; 0; 0; 8; 0;
  (* 8*) -8; 0; -8; -8; -8; -8; 0; -8;
  (* 9*) 2; 0; 4; 5; 6; 7; 0; 0;
  (*10*) 0; 0; 4; 5; 6; 7; 0; 9;
  (*11*) -2; 0; -2; -2; 6; 7; 0; -2;
  (*12*) -3; 0; -3; -3; 6; 7; 0; -3;
  (*13*) -4; 0; -4; -4; -4; -4; 0; -4;
  (*14*) -5; 0; -5; -5; -5; -5; 0; -5;
  (*15*) -7; 0; -7; -7; -7; -7; 0; -7
|]

let gotoTable = [|     (* 64 elements *)
  (* 0*) 65535; 65535; 9; 15;
  (* 1*) 65535; 65535; 65535; 65535;
  (* 2*) 65535; 65535; 65535; 65535;
  (* 3*) 65535; 65535; 11; 15;
  (* 4*) 65535; 65535; 12; 15;
  (* 5*) 65535; 65535; 13; 15;
  (* 6*) 65535; 65535; 14; 15;
  (* 7*) 65535; 65535; 10; 15;
  (* 8*) 65535; 65535; 65535; 65535;
  (* 9*) 65535; 65535; 65535; 65535;
  (*10*) 65535; 65535; 65535; 65535;
  (*11*) 65535; 65535; 65535; 65535;
  (*12*) 65535; 65535; 65535; 65535;
  (*13*) 65535; 65535; 65535; 65535;
  (*14*) 65535; 65535; 65535; 65535;
  (*15*) 65535; 65535; 65535; 65535
|]

let prodInfo_rhsLen = [|       (* 8 elements *)
  (*0*) 2; 3; 3; 3; 3; 1; 1; 3
|]
let prodInfo_lhsIndex = [|     (* 8 elements *)
  (*0*) 1; 2; 2; 2; 2; 2; 2; 3
|]

let stateSymbol = [|           (* 16 elements *)
  (*0*) 0; 1; 2; 3; 4; 5; 6; 7; 8; -3; -3; -3; -3; -3; -3; -4
|]

let nontermOrder = [|          (* 4 elements *)
  (*0*) 3; 2; 1; 0
|]


(* ---------- nascent form of ParseTables interface ------------ *)
let getActionEntry (*tables*) (state: int) (tok: int) : tActionEntry =
begin
  actionTable.(state*actionCols + tok)
end

let isShiftAction (*tables*) (code: tActionEntry) : bool =
begin
  code > 0 && code <= numStates
end

let decodeShift (code: tActionEntry) (shiftedTerminal: int) : tStateId =
begin
  code-1
end

let isReduceAction (code: tActionEntry) : bool =
begin
  code < 0
end

let decodeReduce (code: tActionEntry) (inState: tStateId) : int =
begin
  -(code+1)
end

let isErrorAction (*tables*) (code: tActionEntry) : bool =
begin
  code = 0
end


let getGotoEntry (stateId: tStateId) (nontermId: int) : tGotoEntry =
begin
  gotoTable.(stateId*gotoCols + nontermId)
end

let decodeGoto (code: tGotoEntry) (shiftNonterminal: int) : tStateId =
begin
  code
end


let getProdInfo_rhsLen (rule: int) : int =
begin
  prodInfo_rhsLen.(rule)
end

let getProdInfo_lhsIndex (rule: int) : int =
begin
  prodInfo_lhsIndex.(rule)
end


let getNontermOrdinal (idx: int) : int =
begin
  nontermOrder.(idx)
end


(* EOF *)
