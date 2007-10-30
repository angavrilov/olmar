(*  Copyright 2007 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

let rec list_pos_rec x count = function
  | [] -> raise Not_found
  | y :: xs -> if x = y then count else list_pos_rec x (count +1) xs


(* list_pos x xs searches x in xs and returns the position,
 * raises Not_found if not found
 *)
let list_pos x xs = list_pos_rec x 0 xs


let the = function 
  | None -> assert false
  | Some x -> x
