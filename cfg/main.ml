(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Build
open Cfg_util
open Dot

let files = ref []

let dot_graph = ref false

let functions = ref []

let out_file = ref None

let error_report_level = ref []

let set_sloppy_error_report () =
  error_report_level := [Ignore_unimplemented]

let arguments = Arg.align
  [
    ("-dot", Arg.Set dot_graph,
     " output complete CFG in dot format");
    ("-fun", Arg.String (fun s -> functions := s :: !functions),
     "name generate the CFG for functions with name name in dot format");
    ("-o", Arg.String (fun s -> out_file := Some s),
     "file output into file");       
    ("-sloppy", Arg.Unit set_sloppy_error_report,
     " do not treat unimplemented gap as fatal");
  ]

let usage_msg = 
  "usage: cfg [options...] <files>\n\
   recognized options are:"

let usage () =
  Arg.usage arguments usage_msg;  
  exit(1)
  
let anonfun fn = 
  files := fn :: !files


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
  Arg.parse arguments anonfun usage_msg;
  if !files = [] then
    usage();				(* does not return *)
  let cfg = do_file_list (List.rev !files) !error_report_level
  in
    if !functions <> [] 
    then
      let overload = make_overload_hash cfg in
      let fun_ids = get_ids overload !functions in
      let _ = if fun_ids = [] then usage()
      in
	function_call_graphs_to_dot cfg !out_file fun_ids

    else if !dot_graph 
    then cfg_to_dot cfg !out_file
    else ()
;;


Printexc.catch main ()

