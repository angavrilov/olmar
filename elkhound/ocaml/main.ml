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
open Ptreeact      (* tParseTreeLexer, makeParseTreeActions *)
open Ptreenode     (* tPTreeNode, printTree *)
open Lexer         (* tToken, readToken *)


(* ------------------ lexer ------------------- *)
let useHardcoded:bool = false        (* set to true for testing with ocamldebug *)

class tLexer =
object (self)
  inherit tLexerInterface

  (* stdin *)
  val lexbuf:Lexing.lexbuf =
    if (useHardcoded) then (
      (Lexing.from_string "2+3")     (* hardcoded input *)
    )
    else (
      (Lexing.from_channel stdin)
    );

  method getToken() : unit =
  begin
    (* read from stdin *)
    let t:tToken = (readToken lexbuf) in
    
    (* break the tToken apart into a kind and an sval; perhaps
     * this belongs in lexer.mll too? *)
    match t with
    | INT(i) -> (
        tokType <- 1;
        (self#setIntSval i);
      )
    | _ -> (
        tokType <- (tokenKind t);
        (self#setIntSval 0);           (* clear previous *)
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
    (Lexer.tokenKindDesc kind)
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
  (* random tests *)
  (if false then (
    (Printf.printf "Sys.max_array_length: %d\n" Sys.max_array_length);
    (Printf.printf "Sys.max_string_length: %d\n" Sys.max_string_length);
  ));

  (* defaults *)
  let useGLR: bool ref = ref true in
  let useArith: bool ref = ref true in
  let justTokens: bool ref = ref false in
  let usePTree: bool ref = ref false in

  (* process arguments *)
  for i=1 to ((Array.length Sys.argv) - 1) do
    match Sys.argv.(i) with
    | "lr" ->         useGLR := false
    | "glr" ->        useGLR := true
    | "arith" ->      useArith := true
    | "een" ->        useArith := false
    | "tokens" ->     justTokens := true
    | "ptree" ->      usePTree := true
    | op -> (
        (Printf.printf "unknown option: %s\n" op);
        (flush stdout);
        (failwith "bad command line syntax");
      )
  done;

  (* create the lexer *)
  let lex:tLexerInterface = ((new tLexer) :> tLexerInterface) in
  if (!justTokens) then (
    (* just print all the tokens and bail *)
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

  (* substitute tree-building actions? *)
  let (lex':tLexerInterface), (actions':tUserActions) =
    if (!usePTree) then (
      ((new tParseTreeLexer lex actions),      (* tree lexer *)
       (makeParseTreeActions actions tables))  (* tree actions *)
    )
    else (
      (lex, actions)                           (* unchanged *)
    )
  in
  
  (* parse the input *)
  let sval:tSemanticValue =
    if (not !useGLR) then (
      (* use LR *)
      (parse lex' tables actions')
    )
    else (
      (* use GLR *)
      let glr:tGLR = (makeGLR tables actions') in
      let treeTop: tSemanticValue ref = ref cNULL_SVAL in

      if (not (glrParse glr lex' treeTop)) then (
        (failwith "GLR parse error")
      );

      (* print accounting statistics from glr.ml *)
      (Printf.printf "stack nodes: num=%d max=%d\n"
                     !numStackNodesAllocd
                     !maxStackNodesAllocd);
      (flush stdout);
      
      !treeTop
    )
  in

  (* interpret the result *)
  if (not !usePTree) then (
    (* assume it's an int *)
    let s:int = ((Obj.obj sval) : int) in
    (Printf.printf "%s parse result: %d\n"
                   (if (!useGLR) then "GLR" else "LR")
                   s);
  )
  else (
    (* it's a tree *)
    let t:tPTreeNode = ((Obj.obj sval) : tPTreeNode) in
    (printTree t stdout true(*expand*));
  );
  (flush stdout);

end
;;

Printexc.catch main()
;;
