
open Cc_ast_gen_type
open Ast_annotation
open Elsa_ast_type_descr


let file = ref ""

let file_set = ref false

let verbose_flags = 
  [Heapcheck.Channel stdout; Heapcheck.Verbose_blocks; 
   Heapcheck.Verbose_statistics; Heapcheck.Verbose_types; 
   Heapcheck.Start_indent 2]

let check_flags = 
  ref [Heapcheck.Channel stdout; Heapcheck.Verbose_statistics; 
       Heapcheck.Verbose_spinner; Heapcheck.Start_indent 0]


let arguments = Arg.align
  [
    ("-v",
     Arg.Unit (fun () -> check_flags := verbose_flags),
     " verbose");
    ("-q",
     Arg.Unit (fun () -> check_flags := []),
     " quiet");
  ]

let usage_msg = 
  "usage: check_oast [options...] <file>\n\
   recognized options are:"

let usage () =
  Arg.usage arguments usage_msg;  
  exit(1)
  
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
  if not !file_set then
    usage();				(* does not return *)
  let ic = open_in !file in
  let ast = (Marshal.from_channel ic : annotated translationUnit_type) in
  let res = 
    Heapcheck.check !check_flags ast 
      (translationUnit_type_type_descr annotated_type_descr)
  in
    exit (if res then 0 else 1)
;;


Printexc.catch main ()


