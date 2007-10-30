(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Ast_annotation
open Elsa_reflect_type
open Superast
open Cfg_type

let fun_id_name fun_id = String.concat "::" fun_id.name

let fun_def_name fun_def = fun_id_name fun_def.fun_id

let error_location (file, line, char) =
  Printf.sprintf "File \"%s\", line %d, character %d"
    file line char

let rec list_last = function
  | [] -> raise (Failure "list_last")
  | x :: [] -> x
  | _x :: xs -> list_last xs

let make_overload_hash cfg =
  let over = Hashtbl.create (Hashtbl.length cfg) in
  let _ = 
    Hashtbl.iter
      (fun fun_id _ ->
	 try
	   let funs = Hashtbl.find over (list_last fun_id.name)
	   in
	     funs := fun_id :: !funs
	 with
	   | Not_found -> 
	       Hashtbl.add over (list_last fun_id.name) (ref [fun_id])
      )
      cfg
  in 
  let res = Hashtbl.create (Hashtbl.length over) 
  in
    Hashtbl.iter
      (fun name funs -> Hashtbl.add res name !funs)
      over;
    res


let iter_callees f cfg funs =
  let visited = Hashtbl.create 149 in
  let rec doit context fun_id =
    if Hashtbl.mem visited fun_id
    then () 
    else 
      let _ = Hashtbl.add visited fun_id () in
      let def_opt =
	try 
	  Some(Hashtbl.find cfg fun_id)
	with
	  | Not_found -> None
      in
	(match def_opt with 
	   | Some def ->
	       List.iter (doit (Some def)) def.callees
	   | None -> ()
	);
	f context fun_id def_opt
  in
    List.iter (doit None) funs


type function_application =
  | Func_def of function_def * annotated function_type
  | Short_oast of function_def
  | Bad_oast of function_def * annotated super_ast
  | Undefined


let apply_func_def f context fun_id = function
  | None -> f context fun_id Undefined
  | Some def ->
      let (aa,_) = Superast.load_marshaled_ast_array def.oast
      in
	if def.node_id < Array.length aa
	then
	  match aa.(def.node_id) with
	    | Function_type func -> f context fun_id (Func_def(def, func))
	    | x -> f context fun_id (Bad_oast(def, x))
	else
	  f context fun_id (Short_oast def)


let apply_func_def_gc gc_before f gc_after context fun_id fun_def_opt =
  gc_before();
  let res = apply_func_def f context fun_id fun_def_opt
  in
    gc_after();
    res
