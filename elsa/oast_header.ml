
include Oast_header_version

open Ast_annotation
open Cc_ast_gen_type


let header = 
  Printf.sprintf "Marshaled Olmar C++ abstract syntax tree, version %d"
    oast_header_version

let output_header oc max =
  output_string oc header;
  output_string oc "\n";
  Marshal.to_channel oc (max : int) []

let read_header ic =
  let line = input_line ic
  in
    if line <> header then raise (Failure "oast_header.read_header");
    (Marshal.from_channel ic : int)


let marshal_oast (u : annotated translationUnit_type) fname =
  let oc = open_out fname 
  in
    output_header oc (max_annotation());
    Marshal.to_channel oc u [];
    close_out oc
    (* 
     * ;
     * Gc.print_stat stdout;
     * let c = Gc.get() 
     * in
     *   Printf.printf "minor heap size : %d\nmajor heap increment %d\n%!"
     * 	c.Gc.minor_heap_size c.Gc.major_heap_increment
     *)

let unmarshal_oast file =
  let ic = open_in file 
  in
    try
      let max_node = read_header ic in
      let ast = (Marshal.from_channel ic : annotated translationUnit_type)
      in
	close_in ic;
	(max_node, ast)
    with
      | x -> close_in ic; raise x
    

