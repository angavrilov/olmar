(* main.ml *)
(* porting Elkhound to OCaml *)

open Lexerint      (* tLexerInterface *)
open Lrparse       (* parse *)
open Glr           (* tGLR, makeGLR, glrParse *)
open Useract       (* tSemanticValue *)

(* ------------------ lexer ------------------- *)
class tLexer =
object (self)
  inherit tLexerInterface

  (* hardcoded input *)
  val mutable input: string = "2+3";

  method getToken() : unit =
  begin
    try
      (self#setIntSval 0);        (* clear previous *)

      let c:char =
        if (true) then (
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
  (print_string "hello\n");
  (flush stdout);
  (*failwith "hi";*)

  let lex:tLexerInterface = ((new tLexer) :> tLexerInterface) in
  (*(printTokens lex);*)

  (* prime the lexer: get first token *)
  (lex#getToken());

  if ((Array.length Sys.argv) = 1) then (
    (* no arguments, use LR *)
    let sval:int = (parse lex) in
    (Printf.printf "LR parse result: %d\n" sval);
  )
  else (
    (* some arguments, use GLR *)
    let glr:tGLR = (makeGLR()) in
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
