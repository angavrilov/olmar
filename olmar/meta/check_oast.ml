(*  Copyright 2006-2007 Hendrik Tews, All rights reserved.             *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* check oast file consitency with the memcheck library *)

open Ast_annotation
open Ast_ast_type_descr
(* elsa version *)
open Elsa_ast_type_descr


let file = ref ""

let file_set = ref false

let verbose_flags = 
  [Memcheck.Channel stdout; Memcheck.Verbose_blocks; 
   Memcheck.Verbose_statistics; Memcheck.Start_indent 2]

let verbose_type_flags = 
  [Memcheck.Channel stdout; Memcheck.Verbose_blocks; 
   Memcheck.Verbose_statistics; Memcheck.Verbose_type_ids; 
   Memcheck.Start_indent 2]

let trace_flags = 
  [Memcheck.Channel stdout; Memcheck.Verbose_statistics; 
   Memcheck.Verbose_spinner; Memcheck.Verbose_trace;
   Memcheck.Verbose_type_ids;
   Memcheck.Start_indent 0]

let check_flags = 
  ref [Memcheck.Channel stdout; Memcheck.Verbose_statistics; 
       Memcheck.Verbose_spinner; Memcheck.Start_indent 0]

let astgen_oast = ref false

let arguments = Arg.align
  [
    ("-ast",
     Arg.Set astgen_oast,
     " check trees from astgen");
    ("-trace",
     Arg.Unit (fun () -> check_flags := trace_flags),
     " print trace to error");
    ("-v",
     Arg.Unit (fun () -> check_flags := verbose_flags),
     " verbose");
    ("-vt",
     Arg.Unit (fun () -> check_flags := verbose_type_flags),
     " verbose (including type applications)");
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

  if !astgen_oast then 
    begin
      let (_size, ast) = Ast_oast_header.unmarshal_oast !file in
      let res = 
	Memcheck.check !check_flags ast 
	  (aSTSpecFile_type_type_descr annotated_type_descr)
      in
	exit (if res then 0 else 1)
    end
  else 
    (* non-elsa version *)
    (* 
     * begin
     *   Printf.eprintf "no elsa support at the moment\n";
     *   exit 1
     * end
     *)
    (* elsa version *)
    begin
      let (_size, ast) = Elsa_oast_header.unmarshal_oast !file in
      let res = 
    	Memcheck.check !check_flags ast 
    	  (compilationUnit_type_type_descr annotated_type_descr)
      in
    	exit (if res then 0 else 1)
    end
;;


Printexc.catch main ()


