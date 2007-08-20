(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cfg_type

let fun_id_name fun_id = String.concat "::" fun_id.name

let fun_def_name fun_def = fun_id_name fun_def.fun_id


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
    
	 
