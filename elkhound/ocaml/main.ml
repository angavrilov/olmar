(* main.ml *)
(* driver for an Elkhound parser written in OCaml *)

open Lexerint      (* tLexerInterface *)
open Lrparse       (* parse *)
open Glr           (* tGLR, makeGLR, glrParse *)
open Useract       (* tSemanticValue *)
open Parsetables   (* tParseTables *)
open Useract       (* tUserActions *)
open Arith         (* arithParseTables, arithUserActions *)
open Een           (* eenParseTables, eenUserActions *)


(* ------------------ lexer ------------------- *)
let useHardcoded:bool = false

class tLexer =
object (self)
  inherit tLexerInterface

  (* hardcoded input *)
  val mutable input: string = "2+3+4";

  method getToken() : unit =
  begin
    try
      (self#setIntSval 0);        (* clear previous *)

      let c:char =
        if (not useHardcoded) then (
          (* read from stdin *)
          (input_char stdin)
        )
        else (
          (* take from hardcoded input *)
          let len:int = (String.length input) in
          if (len > 0) then (
            (* take first char *)
            let res:char = (String.get input 0) in
            input <- (String.sub input 1 (len-1));
            res
          )
          else (
            (* eof *)
            (raise End_of_file)
          )
        )
      in

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


(* --------------------- main -------------------- *)
let main() : unit =
begin
  (* defaults *)
  let useGLR: bool ref = ref true in
  let useArith: bool ref = ref true in
  let justTokens: bool ref = ref false in

  (* process arguments *)
  for i=1 to ((Array.length Sys.argv) - 1) do
    match Sys.argv.(i) with
    | "lr" ->         useGLR := false
    | "glr" ->        useGLR := true
    | "arith" ->      useArith := true
    | "een" ->        useArith := false
    | "tokens" ->     justTokens := true
    | op -> (
        (Printf.printf "unknown option: %s\n" op);
        (flush stdout);
        (failwith "bad command line syntax");
      )
  done;

  (* create lexer *)
  let lex:tLexerInterface = ((new tLexer) :> tLexerInterface) in
  if (!justTokens) then (
    (* just print tokens and bail *)
    (printTokens lex);
    (raise Exit);       (* close enough *)
  );

  (* prime the lexer: get first token *)
  (lex#getToken());

  (* get parse tables and user actions, depending on which
   * grammar the user wants to use *)
  let (tables:tParseTables), (actions:tUserActions) =
    if (!useArith) then (
      (arithParseTables, arithUserActions)
    )
    else (
      (eenParseTables, eenUserActions)
    )
  in

  if (not !useGLR) then (
    (* use LR *)
    let sval:int = (parse lex tables actions) in
    (Printf.printf "LR parse result: %d\n" sval);
  )
  else (
    (* use GLR *)
    let glr:tGLR = (makeGLR tables actions) in
    let treeTop: tSemanticValue ref = ref cNULL_SVAL in
    
    if (glrParse glr lex treeTop) then (
      let sval:int = (Obj.obj !treeTop : int) in
      (Printf.printf "GLR parse results: %d\n" sval);
    )
    else (
      (Printf.printf "GLR parse error\n");
    );

    (* print accounting statistics from glr.ml *)    
    (Printf.printf "stack nodes: num=%d max=%d\n"
                   !numStackNodesAllocd
                   !maxStackNodesAllocd);
    (flush stdout);
  );
end
;;

Printexc.catch main()
;;
