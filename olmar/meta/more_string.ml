(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

let is_space = function
  | ' ' 
  | '\012' (* \f *)
  | '\n'
  | '\r'
  | '\t'
  | '\011' (* \v *)
    -> true
  | _ -> false

let trim_white_space s =
  let start = ref 0 in
  let len = ref (String.length s)
  in
    (* skip white space at the front *)
    while !len > 0 && is_space(s.[!start]) do
      incr start;
      decr len
    done;
    (* skip white space at the end *)
    while !len > 0 && is_space(s.[!start + !len -1]) do
      decr len
    done;
    String.sub s !start !len


let string_match space pattern pos =
  let space_len = String.length space in
  let pattern_len = String.length pattern 
  in
    String.sub space pos (min pattern_len (max 0 (space_len - pos))) = pattern


let string_match_left space pattern =
  string_match space pattern 0


let translate from tos s =
  let s = String.copy s in
  let s_len = String.length s
  in
    if String.length from <> String.length tos 
    then
      raise (Invalid_argument "Meta_ast.translate");
    for i = 0 to String.length from -1 do
      let j = ref 0
      in
	while !j < s_len do
	  try
	    let k = String.index_from s !j from.[i]
	    in
	      s.[k] <- tos.[i];
	      j := k
	  with
	    | Not_found -> j := s_len
	done
    done;
    s


(******************************************************************************
 ******************************************************************************
 *
 * print utilities
 *
 ******************************************************************************
 ******************************************************************************)

let pr_comment oc = function
  | [] -> ()
  | [line] -> Printf.fprintf oc "(* %s *) " line
  | first::remaining ->
      Printf.fprintf oc "(* %s\n" first;
      List.iter
	(fun l -> Printf.fprintf oc " * %s\n" l)
	remaining;
      Printf.fprintf oc " *)\n"

let pr_c_comment oc = function
  | [] -> ()
  | [line] -> Printf.fprintf oc "/* %s */ " line
  | first::remaining ->
      Printf.fprintf oc "/* %s\n" first;
      List.iter
	(fun l -> Printf.fprintf oc " * %s\n" l)
	remaining;
      Printf.fprintf oc " */\n"

let pr_linenumber_directive line file oc =
  Printf.fprintf oc "# %d \"%s\"\n"
    line
    (if Filename.is_relative file 
     then
       Filename.concat (Sys.getcwd()) file
     else
       file)
       

(******************************************************************************
 ******************************************************************************
 *
 * some constant strings
 *
 ******************************************************************************
 ******************************************************************************)

let do_not_edit_line =
  "** DO NOT EDIT ***** DO NOT EDIT ***** DO NOT EDIT ***** DO NOT EDIT ***"

let star_line =
  "**************************************************************************"
