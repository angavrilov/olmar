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

  (* semantic value of the token; since I can't do arbitrary casts,
   * for now it must be an integer *)
  val mutable sval:int = 0;

  (* lexerint.h has a 'loc' field here... *)

  (* ---- funcs ---- *)
  (* data members aren't public... *)
  method getTokType() : int = tokType
  method getSval() : int = sval

  method getToken() : unit =
  begin
    try
      sval <- 0;    (* clear previous *)

      let c:char = (input_char stdin) in
      if ('0' <= c && c <= '9') then (
        tokType <- 1;
        sval <- (int_of_char c) - (int_of_char '0');
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
      kindDesc ^ "(" ^ (string_of_int sval) ^ ")"
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
                   (lex#getSval())
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


(* -------------------- parser ------------------- *)
let stateStack : int array ref = ref (Array.make 10 0)
let stateStackLen : int ref = ref 0

(* TODO: add sval stack of Obj.t *)

let pushState (v : int) : unit =
begin
  if ((Array.length !stateStack) = !stateStackLen) then (
    (* must make it bigger *)
    let newStack : int array = (Array.make (!stateStackLen * 2) 0) in

    (* copy *)
    (Array.blit
      !stateStack           (* source array *)
      0                     (* source start position *)
      newStack              (* dest array *)
      0                     (* dest start position *)
      !stateStackLen         (* number of elements to copy *)
    );

    (* switch from old to new *)
    stateStack := newStack;
  );

  (* put new element into the stack at the end *)
  (!stateStack).(!stateStackLen) <- v;
  (incr stateStackLen);
end

let topState() : int =
begin
  (!stateStack).(!stateStackLen - 1)
end

let parse (lex:tLexerInterface) : unit =
begin
  (* get first token *)
  (lex#getToken());

  (* initial state *)
  (pushState 0);

  (* loop over all tokens until EOF and stack has just start symbol *)
  while (not ((lex#getTokType()) = 0)) ||
        (!stateStackLen > 2) do
    let tt:int = (lex#getTokType()) in        (* token type *)
    let state:int = (topState()) in           (* current state *)

    (Printf.printf "state=%d tokType=%d sval=%d desc=\"%s\"\n"
                   state
                   tt
                   (lex#getSval())
                   (lex#tokenDesc())
                 );
    (flush stdout);

    (* read from action table *)
    let act:int = actionTable.(state*actionCols + tt) in

    (* shift? *)
    if (0 < act && act <= numStates) then (
      let dest:int = act-1 in                 (* destination state *)
      (pushState dest);

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

      (* pop 'ruleLen' elements *)
      stateStackLen := (!stateStackLen - ruleLen);
      let newTopState:int = (topState()) in

      (* get new state *)
      let dest:int = gotoTable.(newTopState*gotoCols + lhs) in
      (pushState dest);

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
  let i:int ref = ref (pred !stateStackLen) in
  while (!i >= 0) do
    (Printf.printf "  %d\n" (!stateStack).(!i));
    (decr i);
  done;
end


(* --------------------- main -------------------- *)
let main() : unit =
begin
  (print_string "hello\n");
  (flush stdout);
  (*failwith "hi";*)

  let lex:tLexerInterface = (new tLexerInterface) in
  (*(printTokens lex);*)
  
  (parse lex);

end
;;

Printexc.catch main()
;;
