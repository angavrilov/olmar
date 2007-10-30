(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* simple types needed in the ocaml ast *)

type unsigned_long = int32
type double = float
type unsigned_int = int32

(* SourceLoc is defined as an enum in srcloc.h, here we take the
 * xml representation, which is file * line * char
 *
 * xmlserialization: hardwired in astgen:
 * toXml_SourceLoc -> sourceLocManager->getString(loc)
 *)
type sourceLoc = string * int * int


(* from strtable.h
 * xmlserialization as string, hardwired in astgen
 *)
type stringRef = string


type boolValue = bool


(* In the elsa array size is a positive integer, where -1 and -2 
 * have the special meanings NO_SIZE and DYN_SIZE.
 *)
type array_size =
  | NO_SIZE				(* size unspecified *)
  | DYN_SIZE				(* some gnu extension *)
  | FIXED_SIZE of int			(* suppostly >= 0 *)


let string_of_array_size = function
  | NO_SIZE	  -> "unspecified"
  | DYN_SIZE	  -> "dynamic"
  | FIXED_SIZE i  -> Printf.sprintf "%d fixed" i


