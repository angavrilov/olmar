(* main.ml *)
(* porting Elkhound to OCaml *)




(* ------------------ lexer ------------------- *)
class tLexerInterface =
object (self)
  (* ---- data ---- *)
  (* the following fields are used to describe the *current* token;
   * they are changed by getToken() each time it is called *)

  (* token classification, used by parser to make parsing decisions;
   * the value 0 means end of file *)
  val mutable tokType:int = 0;

  (* semantic value of the token *)
  val mutable sval:Obj.t = (Obj.repr 0);

  (* lexerint.h has a 'loc' field here... *)

  (* ---- funcs ---- *)
  (* data members aren't public... *)
  method getTokType() : int = tokType
  method getSval() : Obj.t = sval

  method setIntSval (i:int) : unit =
  begin
    sval <- (Obj.repr i)
  end
  method getIntSval() : int =
  begin
    (Obj.obj sval : int)
  end

  method getToken() : unit =
  begin
    try
      (self#setIntSval 0);        (* clear previous *)

      let c:char = (input_char stdin) in
      if ('0' <= c && c <= '9') then (
        tokType <- 1;
        (self#setIntSval ((int_of_char c) - (int_of_char '0')));
      )
      else if (c = '+') then tokType <- 2
      else if (c = '-') then tokType <- 3
      else if (c = '*') then tokType <- 4
      else if (c = '/') then tokType <- 5
      else if (c = '(') then tokType <- 6
      else if (c = ')') then tokType <- 7
      else (
        (* skip it *)
        self#getToken()
      )
    with End_of_file -> (
      tokType <- 0
    )
  end
  
  method tokenDesc() : string =
  begin
    let kindDesc:string = (self#tokenKindDesc tokType) in
    if (tokType = 1) then (
      kindDesc ^ "(" ^ (string_of_int (self#getIntSval())) ^ ")"
    )
    else
      kindDesc
  end

  method tokenKindDesc (kind:int) : string =
  begin
    match tokType with
    | 0 -> "EOF"
    | 1 -> "Number"
    | 2 -> "+"
    | 3 -> "-"
    | 4 -> "*"
    | 5 -> "/"
    | 6 -> "("
    | 7 -> ")"
    | _ -> (failwith "bad token kind")
  end
end


let printTokens (lex:tLexerInterface) : unit =
begin
  (lex#getToken ());
  while (not ((lex#getTokType()) = 0)) do
    (Printf.printf "tokType=%d sval=%d desc=\"%s\"\n"
                   (lex#getTokType())
                   (lex#getIntSval())
                   (lex#tokenDesc())
                 );
    (flush stdout);
    (lex#getToken ());
  done
end


(* ------------------- parse tables (from arith.gr.gen.cc) -------------------- *)
let numTerms = 8
let numNonterms = 4
let numStates = 16
let numProds = 8
let actionCols = 8
let actionRows = 16
let gotoCols = 4
let gotoRows = 16

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


(* ---------------- reduction actions -------------- *)
(* this is how ocamlyacc does it, so I assume it's fastest way *)
let actionArray : (Obj.t array -> Obj.t) array = [|
  (fun svals ->
    let top = (Obj.obj svals.(0) : int) in 
    (Obj.repr (
      top
    ))
  );
  (fun svals ->
    let e1 = (Obj.obj svals.(0) : int) in
    let e2 = (Obj.obj svals.(2) : int) in
    (Obj.repr (
      e1 + e2
    ))
  );
  (fun svals ->
    let e1 = (Obj.obj svals.(0) : int) in
    let e2 = (Obj.obj svals.(2) : int) in
    (Obj.repr (
      e1 - e2
    ))
  );
  (fun svals ->
    let e1 = (Obj.obj svals.(0) : int) in
    let e2 = (Obj.obj svals.(2) : int) in
    (Obj.repr (
      e1 * e2
    ))
  );
  (fun svals ->
    let e1 = (Obj.obj svals.(0) : int) in
    let e2 = (Obj.obj svals.(2) : int) in
    (Obj.repr (
      e1 / e2
    ))
  );
  (fun svals ->
    let n = (Obj.obj svals.(0) : int) in
    (Obj.repr (
      n
    ))
  );
  (fun svals ->
    let p = (Obj.obj svals.(0) : int) in
    (Obj.repr (
      p
    ))
  );
  (fun svals ->
    let e = (Obj.obj svals.(1) : int) in
    (Obj.repr (
      e
    ))
  )
|]

let reductionAction (productionId:int) (svals:Obj.t array) : Obj.t =
begin
  (actionArray.(productionId) svals)
end


(* -------------------- parser ------------------- *)
(* NOTE: in some cases, more detailed comments can be found in
 * elkhound/glr.h, as these data structures mirror the ones
 * defined there *)
type tStateId = int

(* link from one stack node to another *)
type tSiblingLink = {
  (* stack node we're pointing at *)
  sib: tStackNode;

  (* semantic value on this link *)
  sval: Obj.t;

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

  (* when this path is in the queue, this link points to the
   * next path in dequeueing order *)
  mutable next: tPath;
}

(* priority queue of reduction paths *)
type tReductionPathQueue = {
  (* head of the list, first to dequeue *)
  mutable top: tPath;
















let stateStack : tStateId array ref = ref (Array.make 10 0)
let svalStack : Obj.t array ref = ref (Array.make 10 (Obj.repr 0))
let stackLen : int ref = ref 0

let pushStateSval (state : tStateId) (sval : Obj.t) : unit =
begin
  if ((Array.length !stateStack) = !stackLen) then (
    (* must make it bigger *)
    let newStateStack : tStateId array = (Array.make (!stackLen * 2) 0) in
    let newSvalStack : Obj.t array = (Array.make (!stackLen * 2) (Obj.repr 0)) in

    (* copy *)
    (Array.blit
      !stateStack           (* source array *)
      0                     (* source start position *)
      newStateStack         (* dest array *)
      0                     (* dest start position *)
      !stackLen             (* number of elements to copy *)
    );
    (Array.blit
      !svalStack            (* source array *)
      0                     (* source start position *)
      newSvalStack          (* dest array *)
      0                     (* dest start position *)
      !stackLen             (* number of elements to copy *)
    );

    (* switch from old to new *)
    stateStack := newStateStack;
    svalStack := newSvalStack;
  );

  (* put new element into the stack at the end *)
  (!stateStack).(!stackLen) <- state;
  (!svalStack).(!stackLen) <- sval;
  (incr stackLen);
end

let topState() : tStateId =
begin
  (!stateStack).(!stackLen - 1)
end

let parse (lex:tLexerInterface) : int =
begin
  (* get first token *)
  (lex#getToken());

  (* initial state *)
  (pushStateSval 0 (Obj.repr 0));

  (* loop over all tokens until EOF and stack has just start symbol *)
  while (not ((lex#getTokType()) = 0)) ||
        (!stackLen > 2) do
    let tt:int = (lex#getTokType()) in        (* token type *)
    let state:tStateId = (topState()) in      (* current state *)

    (Printf.printf "state=%d tokType=%d sval=%d desc=\"%s\"\n"
                   state
                   tt
                   (lex#getIntSval())
                   (lex#tokenDesc())
                 );
    (flush stdout);

    (* read from action table *)
    let act:int = actionTable.(state*actionCols + tt) in

    (* shift? *)
    if (0 < act && act <= numStates) then (
      let dest:tStateId = act-1 in            (* destination state *)
      (pushStateSval dest (lex#getSval()));

      (* next token *)
      (lex#getToken());

      (Printf.printf "shift to state %d\n" dest);
      (flush stdout);
    )

    (* reduce? *)
    else if (act < 0) then (
      let rule:int = -(act+1) in              (* reduction rule *)
      let ruleLen:int = prodInfo_rhsLen.(rule) in
      let lhs:int = prodInfo_lhsIndex.(rule) in

      (* make an array of semantic values for the action rule; this does
       * an extra copy if we're already using a linear stack, but will
       * be needed for GLR so I'll do it this way *)
      let svalArray : Obj.t array = (Array.make ruleLen (Obj.repr 0)) in
      (Array.blit
        !svalStack            (* source array *)
        (!stackLen - ruleLen) (* source start position *)
        svalArray             (* dest array *)
        0                     (* dest start position *)
        ruleLen               (* number of elements to copy *)
      );

      (* invoke user's reduction action *)
      let sval:Obj.t = (reductionAction rule svalArray) in

      (* pop 'ruleLen' elements *)
      stackLen := (!stackLen - ruleLen);
      let newTopState:int = (topState()) in

      (* get new state *)
      let dest:tStateId = gotoTable.(newTopState*gotoCols + lhs) in
      (pushStateSval dest sval);

      (Printf.printf "reduce by rule %d (len=%d, lhs=%d), goto state %d\n"
                     rule ruleLen lhs dest);
      (flush stdout);
    )

    (* error? *)
    else if (act = 0) then (
      (Printf.printf "parse error in state %d\n" state);
      (flush stdout);
      (failwith "parse error");
    )

    (* bad code? *)
    else (
      (failwith "bad action code");
    );
  done;

  (* print final parse stack *)
  (Printf.printf "final parse stack (up is top):\n");
  let i:int ref = ref (pred !stackLen) in
  while (!i >= 0) do
    (Printf.printf "  %d\n" (!stateStack).(!i));
    (decr i);
  done;
  (flush stdout);

  (* return value: sval of top element *)
  let topSval:Obj.t = (!svalStack).(!stackLen - 1) in

  (* assume is int for now *)
  (Obj.obj topSval : int)
end


(* --------------------- main -------------------- *)
let main() : unit =
begin
  (print_string "hello\n");
  (flush stdout);
  (*failwith "hi";*)

  let lex:tLexerInterface = (new tLexerInterface) in
  (*(printTokens lex);*)
  
  let sval:int = (parse lex) in
  (Printf.printf "parse result: %d\n" sval);

end
;;

Printexc.catch main()
;;
