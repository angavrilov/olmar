(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Superast
open Ast_annotation

(* returns the uplinks and downlinks arrays
 * the downlinks array is in left-to-right order
 * order of the uplinks is unspecified
 *)
val create : annotated super_ast array -> (int list array * int list array)

