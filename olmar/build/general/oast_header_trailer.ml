(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

let header = 
  Printf.sprintf header_comment oast_header_version

let output_header oc max =
  output_string oc header;
  output_string oc "\n";
  Marshal.to_channel oc (max : int) []

let read_header ic =
  let line = input_line ic
  in
    if line <> header then raise (Failure "oast_header.read_header");
    (Marshal.from_channel ic : int)


let marshal_oast (u : oast_type) fname =
  let oc = open_out fname 
  in
    output_header oc (max_annotation());
    Marshal.to_channel oc u [];
    close_out oc

let marshal_oast_catch_all u fname =
  try
    marshal_oast u fname
  with
    | x -> 
	Printf.eprintf
	  "MARSHALL ERROR: exception in marshal_oast:\n%s\n%!"
	  (Printexc.to_string x);
	exit(1)


let unmarshal_oast file =
  let ic = open_in file 
  in
    try
      let max_node = read_header ic in
      let ast = (Marshal.from_channel ic : oast_type)
      in
	close_in ic;
	(max_node, ast)
    with
      | x -> close_in ic; raise x



let file = ref None

let general_args = Arg.align
  [
  ]

let usage_msg application_name = 
  Printf.sprintf
    "usage: %s [options...] <file>\n\
     recognized options are:"
    application_name


let usage args application_name =
  Arg.usage args (usage_msg application_name);
  exit(1)
  
let anonfun args application_name arg = 
  match !file with
    | None -> file := Some arg
    | Some _ ->
	Printf.eprintf "don't know what to do with %s\n" arg;
	usage args application_name



let setup_oast args application_name =
  let args = general_args @ args
  in
    Arg.parse args (anonfun args application_name)
      (usage_msg application_name);
    match !file with
      | None ->
	  prerr_endline "oast file missing!";
	  usage args application_name;
	  exit 1
      | Some f -> 
	  let (size, ast) = unmarshal_oast f  
	  in
	    (f, size, ast)


let register_oast_header_callbacks () =
  Callback.register "marshal_oast_catch_all" marshal_oast_catch_all;
  ()
