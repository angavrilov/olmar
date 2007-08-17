(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Build
open Dot

let files = ref []

let dot_graph = ref false

let out_file = ref None

let arguments = Arg.align
  [
    ("-dot", Arg.Set dot_graph,
     " output CFG in dot format");
    ("-o", Arg.String (fun s -> out_file := Some s),
     "file output into file");       
  ]

let usage_msg = 
  "usage: cfg [options...] <files>\n\
   recognized options are:"

let usage () =
  Arg.usage arguments usage_msg;  
  exit(1)
  
let anonfun fn = 
  files := fn :: !files


let main () =
  Arg.parse arguments anonfun usage_msg;
  if !files = [] then
    usage();				(* does not return *)
  let cfg = do_file_list (List.rev !files) []
  in
    if !dot_graph then cfg_to_dot cfg !out_file
;;


Printexc.catch main ()

