
open Cc_ast_gen_type

module U = Unix

let elsa_dir = ref ""

let start_dump_id = "(****** START DUMP ******)"

let output_filter ic =
  let skipping = ref true
  in
    try
      while !skipping do
	let l = input_line ic
	in
	  if l = start_dump_id then
	    begin
	      ignore(input_line ic);
	      skipping := false;
	    end
      done;
      while true do
	let l = input_line ic
	in
	  print_endline l
      done
    with
      | End_of_file -> ()

let pipe_ast oc ast =
  (* let oc = stdout in *)
  if !elsa_dir <> "" then
    output_string oc (Printf.sprintf "#directory \"%s\";;\n" !elsa_dir);
  output_string oc (Printf.sprintf "#print_depth %d;;\n" max_int);
  output_string oc (Printf.sprintf "#print_length %d;;\n" max_int);
  (* 
   * output_string oc (Printf.sprintf "#print_depth %d;;\n" 100000);
   * output_string oc (Printf.sprintf "#print_length %d;;\n" 100000);
   *)
  output_string oc "open Cc_ast_gen_type;;\n";
  output_string oc (Printf.sprintf "print_endline \"\n%s\";\n" start_dump_id);
  output_string oc "(Marshal.from_string \"";
  output_string oc
    (String.escaped 
       (Marshal.to_string
	  (ast : translationUnit_type)
	  []));
  output_string oc "\" 0 : ";
  output_string oc "translationUnit_type";
  output_string oc ");;\n"


let dump_to_toplevel ast =
  let (ic,oc) = U.open_process "ocaml"
  in
    if U.fork() = 0
    then
      begin
	close_in ic;
	pipe_ast oc ast;
	close_out oc;
      end
    else
      begin
	close_out oc;
	output_filter ic;
	close_in ic;
	exit 0;
      end


let arguments = Arg.align
  [("-I", Arg.Set_string elsa_dir,
    "dir search dir for cc_ast_gen_type.cmo");
  ]

let usage_msg = 
  "usage: dumpast [options...] <file>\n\
   recognized options are:"

let usage () =
  prerr_endline usage_msg;
  exit(1)
  
let file = ref ""

let file_set = ref false

let anonfun fn = 
  if !file_set 
  then
    begin
      Printf.eprintf "don't know what to do with %s\n" fn;
      usage()
    end
  else
    begin
      file := fn;
      file_set := true
    end

let main () =
  Arg.parse arguments anonfun usage_msg;
  if !elsa_dir = "" then
    begin
      if Sys.file_exists "cc_ast_gen_type.cmi"
      then ()
      else if Sys.file_exists "../elsa/cc_ast_gen_type.cmi"
      then
	elsa_dir := "../elsa"
      else
	begin
	  prerr_endline
	    "Cannot find cc_ast_gen_type.cmi. Please use -I and/or recompile.";
	  exit 2;
	end
    end;
  if not !file_set then
    usage();				(* does not return *)
  let ic = open_in !file in
  let ast = (Marshal.from_channel ic : translationUnit_type) 
  in
    close_in ic;
    dump_to_toplevel ast
;;


Printexc.catch main ()


(*** Local Variables: ***)
(*** compile-command: "ocamlc.opt -o dumpast -I ../elsa unix.cma dumpast.ml" ***)
(*** End: ***)


