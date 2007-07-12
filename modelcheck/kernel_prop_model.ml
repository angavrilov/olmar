(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cc_ast_gen_type
open Ast_annotation
open Ast_util_2

(***********************************************************************
 *
 * general state
 *
 ***********************************************************************)

let default_extension = ".pml"

let file = ref None

let the = function
  | None -> assert false
  | Some x -> x

let output_name = ref None




(***********************************************************************
 *
 * output generation
 *
 ***********************************************************************)

let out_channel = ref stdout

let p s = output_string !out_channel s

let pc s = p ("/* " ^ s ^ " */\n")

let pf fmt = Printf.fprintf !out_channel fmt

let print_header () =
  let module U = Unix in
  let tm = U.localtime(U.time()) 
  in
    p "/**********************************************************\n";
    p " *\n";
    p " * kernel property model\n";
    p " *\n";
    pf " * automatically generated from %s\n" (the !file);
    pf " * on %d.%02d.%d %d:%d:%d\n"
      tm.U.tm_mday
      (tm.U.tm_mon +1)
      (tm.U.tm_year + 1900)
      tm.U.tm_hour
      tm.U.tm_min
      tm.U.tm_sec;
    p " *\n";
    p " **********************************************************/\n\n\n"
      

let rec translationUnit_fun 
    ((_annot, topForm_list, _scope_opt)  : annotated translationUnit_type) =
  pf "/* translation unit with %d top forms */\n\n" (List.length topForm_list);
  List.iter topForm_fun topForm_list


and topForm_fun = function
    (* a function declaration *)
  | TF_func(_annot, _sourceLoc, func) -> 
      pf "/* function %s */\n" (name_of_function func);
      block_fun (body_of_function func)
      
      (* ignore the following *)
  | TF_decl _
  | TF_template _
  | TF_explicitInst _
  | TF_linkage _
  | TF_one_linkage _
  | TF_asm _
  | TF_namespaceDefn _
  | TF_namespaceDecl _
      -> ()


and block_fun = function
  | S_skip(_annot, _sourceLoc) -> 
      pc "skip statement"

  | S_label(_annot, _sourceLoc, _stringRef, statement) -> 
      pc "label statement";
      block_fun statement

  | S_case(_annot, _sourceLoc, _expression, statement, _int32) -> 
      pc "case statement";
      block_fun statement;

  | S_default(_annot, _sourceLoc, statement) -> 
      pc "default statement";
      block_fun statement

  | S_expr(_annot, _sourceLoc, _fullExpression) -> 
      pc "expression statement"

  | S_compound(_annot, _sourceLoc, statement_list) -> 
      pc "block";
      List.iter block_fun statement_list

  | S_if(_annot, _sourceLoc, _condition, statement_then, statement_else) -> 
      pc "if statement";
      block_fun statement_then;
      pc "else branch";
      block_fun statement_else	

  | S_switch(_annot, _sourceLoc, _condition, statement) -> 
      pc "switch statement";
      block_fun statement

  | S_while(_annot, _sourceLoc, _condition, statement) -> 
      pc "while statement";
      block_fun statement

  | S_doWhile(_annot, _sourceLoc, statement, _fullExpression) -> 
      pc "do while statement";
      block_fun statement

  | S_for(_annot, _sourceLoc, _statement_init, _condition, _fullExpression, 
	  statement_body) -> 
      pc "for statement";
      block_fun statement_body

  | S_break(_annot, _sourceLoc) -> 
      pc "break statement"

  | S_continue(_annot, _sourceLoc) -> 
      pc "continue statement"

  | S_return(_annot, _sourceLoc, _fullExpression_opt, _statement_opt) -> 
      pc "return statement"

  | S_goto(_annot, _sourceLoc, _stringRef) -> 
      pc "goto statement"

  | S_decl(_annot, _sourceLoc, _declaration) -> 
      pc "declaration"

  | S_try(_annot, _sourceLoc, statement, _handler_list) -> 
      pc "try statement";
      block_fun statement

  | S_asm(_annot, _sourceLoc, _e_stringLit) -> 
      pc "S_asm ??"

  | S_namespaceDecl(_annot, _sourceLoc, _namespaceDecl) -> 
      pc "namespace declaration"

  | S_function(_annot, _sourceLoc, _func) -> 
      pc "nested function def"

  | S_rangeCase(_annot, _sourceLoc, 
		_expression_lo, _expression_hi, statement, 
		_label_lo, _label_hi) -> 
      pc "case range statement";
      block_fun statement

  | S_computedGoto(_annot, _sourceLoc, _expression) -> 
      pc "computed goto statement"


      

(***********************************************************************
 *
 * option processing and main
 *
 ***********************************************************************)

let arguments = Arg.align
  [
    ("-o", Arg.String (fun s -> output_name := Some s),
     "out save output in file out");
  ]

let usage_msg = 
  "usage: kernel_prop_model [options...] <file>\n\
   recognized options are:"

let usage () =
  Arg.usage arguments usage_msg;  
  exit(1)
  
let anonfun fn = 
  if !file <> None
  then
    begin
      Printf.eprintf "don't know what to do with %s\n" fn;
      usage()
    end
  else
    file := Some fn

let get_output_channel () =
  if !output_name = None 
  then
    let fname = 
      (if Filename.check_suffix (the !file) ".oast"
       then
	 Filename.chop_suffix (the !file) ".oast"
       else 
	 the !file)
      ^ default_extension
    in 
      open_out fname
  else if !output_name = Some "-"
  then stdout
  else
    open_out (the !output_name)


let main () =
  Arg.parse arguments anonfun usage_msg;
  if !file = None then
    usage();				(* does not return *)
  let ic = open_in (the !file) in
  let _ = Oast_header.read_header ic in
  let ast = (Marshal.from_channel ic : annotated translationUnit_type) 
  in
    out_channel := get_output_channel();
    print_header ();
    translationUnit_fun ast
;;


Printexc.catch main ()

