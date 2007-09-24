(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cc_ast_gen_type
open Ast_annotation
open Ast_util
open Cfg_type
open Cfg_util
open Build

(***********************************************************************
 *
 * general state
 *
 ***********************************************************************)

let default_extension = ".pml"

let the = function
  | None -> assert false
  | Some x -> x

let output_name = ref None

let saved_command_line = ref ""

(***********************************************************************
 *
 * output generation
 *
 ***********************************************************************)

let out_channel = ref stdout

let p s = output_string !out_channel s

let pc = function
  | [] -> ()
  | [s] -> p ("/* " ^ s ^ " */\n")
  | s::sl ->
      p ("/* " ^ s ^ "\n");
      List.iter (fun s -> p (" * " ^ s ^ "\n")) sl;
      p (" */\n")

let pf fmt = Printf.fprintf !out_channel fmt

let print_header () =
  let module U = Unix in
  let tm = U.localtime(U.time()) 
  in
    p "/**********************************************************\n";
    p " *\n";
    p " * kernel property model\n";
    p " *\n";
    pf " * automatically generated from %s\n" !saved_command_line;
    pf " * on %d.%02d.%d %d:%d:%d\n"
      tm.U.tm_mday
      (tm.U.tm_mon +1)
      (tm.U.tm_year + 1900)
      tm.U.tm_hour
      tm.U.tm_min
      tm.U.tm_sec;
    p " *\n";
    p " **********************************************************/\n\n\n"
      

(***********************************************************************
 *
 * misc
 *
 ***********************************************************************)

let id () = ()

let gc_report () =
  let stat = Gc.stat()
  in
    Printf.printf "GC size %d\n"
      stat.Gc.live_words

(***********************************************************************
 *
 * ast recursion
 *
 ***********************************************************************)


let rec block_fun = function
  | S_skip(_annot, _sourceLoc) -> 
      pc ["skip statement"]

  | S_label(_annot, _sourceLoc, _stringRef, statement) -> 
      pc ["label statement"];
      block_fun statement

  | S_case(_annot, _sourceLoc, _expression, statement, _int32) -> 
      pc ["case statement"];
      block_fun statement;

  | S_default(_annot, _sourceLoc, statement) -> 
      pc ["default statement"];
      block_fun statement

  | S_expr(_annot, _sourceLoc, _fullExpression) -> 
      pc ["expression statement"]

  | S_compound(_annot, _sourceLoc, statement_list) -> 
      pc ["block"];
      List.iter block_fun statement_list

  | S_if(_annot, _sourceLoc, _condition, statement_then, statement_else) -> 
      pc ["if statement"];
      block_fun statement_then;
      pc ["else branch"];
      block_fun statement_else	

  | S_switch(_annot, _sourceLoc, _condition, statement) -> 
      pc ["switch statement"];
      block_fun statement

  | S_while(_annot, _sourceLoc, _condition, statement) -> 
      pc ["while statement"];
      block_fun statement

  | S_doWhile(_annot, _sourceLoc, statement, _fullExpression) -> 
      pc ["do while statement"];
      block_fun statement

  | S_for(_annot, _sourceLoc, _statement_init, _condition, _fullExpression, 
	  statement_body) -> 
      pc ["for statement"];
      block_fun statement_body

  | S_break(_annot, _sourceLoc) -> 
      pc ["break statement"]

  | S_continue(_annot, _sourceLoc) -> 
      pc ["continue statement"]

  | S_return(_annot, _sourceLoc, _fullExpression_opt, _statement_opt) -> 
      pc ["return statement"]

  | S_goto(_annot, _sourceLoc, _stringRef) -> 
      pc ["goto statement"]

  | S_decl(_annot, _sourceLoc, _declaration) -> 
      pc ["declaration"]

  | S_try(_annot, _sourceLoc, statement, _handler_list) -> 
      pc ["try statement"];
      block_fun statement

  | S_asm(_annot, _sourceLoc, _e_stringLit) -> 
      pc ["S_asm ??"]

  | S_namespaceDecl(_annot, _sourceLoc, _namespaceDecl) -> 
      pc ["namespace declaration"]

  | S_function(_annot, _sourceLoc, _func) -> 
      pc ["nested function def"]

  | S_rangeCase(_annot, _sourceLoc, 
		_expression_lo, _expression_hi, statement, 
		_label_lo, _label_hi) -> 
      pc ["case range statement"];
      block_fun statement

  | S_computedGoto(_annot, _sourceLoc, _expression) -> 
      pc ["computed goto statement"]


      
let report_func context_opt fun_id func_appl = 
  let con_text =
    match context_opt with
      | None -> "no caller known"
      | Some caller ->
	  Printf.sprintf "called from %s (%s)"
	    (fun_def_name caller) (error_location caller.loc)
  in
    match func_appl with
      | Func_def(def, func) ->
	  pc ["Function " ^ (fun_def_name def);
	      "defined in " ^ error_location def.loc;
	      Printf.sprintf "node %d in %s" def.node_id def.oast;
	      con_text
	     ];
	  block_fun (body_of_function func);
	  p "\n\n";
	  
      | Short_oast def -> 
	  pc ["Error on " ^ fun_def_name def;
	      "defined in " ^ error_location def.loc;
	      Printf.sprintf "Oast index too large (%d) in %s"
		def.node_id def.oast;
	      con_text];
	  p "\n\n";

      | Bad_oast(def, _) ->
	  pc ["Error on " ^ fun_def_name def;
	      "defined in " ^ error_location def.loc;
	      Printf.sprintf "Node %d in %s is not a function def"
		def.node_id def.oast;
	      con_text];
	  p "\n\n";

      | Undefined ->
	  pc ["Error on " ^ fun_id_name fun_id;
	      "Function undefined";
	      con_text];
	  p "\n\n"


(***********************************************************************
 *
 * option processing and main
 *
 ***********************************************************************)

let funs = ref []

let oasts = ref []

let arguments = Arg.align
  [
    ("-fn", Arg.String (fun s -> funs := s :: !funs),
     "f process function f (and all dependencies)");
    ("-o", Arg.String (fun s -> output_name := Some s),
     "out save output in file out");
  ]

let usage_msg = 
  "usage: kernel_prop_model [options...] <files>\n\
   recognized options are:"

let usage () =
  Arg.usage arguments usage_msg;  
  exit(1)
  
let anonfun oast = 
  oasts := oast :: !oasts

let get_output_channel () =
  if !output_name = None 
  then stdout 
  else
    open_out (the !output_name)


let get_ids overload funs =
  List.fold_left
    (fun res f_name ->
       try 
	 match Hashtbl.find overload f_name with
	   | [] -> assert false
	   | [id] -> id :: res
	   | ids -> 
	       Printf.eprintf 
		 "Function name %s is overloaded. Process all %d candidates\n"
		 f_name (List.length ids);
	       ids @ res
       with
	 | Not_found ->
	     Printf.eprintf "Function %s unknown\n" f_name;
	     res)
    []
    funs
       

let main () =
  saved_command_line := String.concat " " (Array.to_list Sys.argv);
  Arg.parse arguments anonfun usage_msg;
  if !oasts = [] or !funs = [] then
    usage();				(* does not return *)
  let cfg = do_file_list !oasts [] in
  let overload = make_overload_hash cfg in
  let fun_ids = get_ids overload !funs in
  let _ = if fun_ids = [] then usage()
  in
    out_channel := get_output_channel();
    print_header ();
    iter_callees (apply_func_def report_func) cfg fun_ids
    (* 
     * iter_callees (apply_func_def_gc gc_report report_func id) cfg fun_ids;
     * gc_report()
     *)
;;


Printexc.catch main ()
