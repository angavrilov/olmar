(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cfg_type

(* cfg_to_dot cfg outfile:
 * convert a cfg into dot and write to designated file
 * or stdout if outfile = None
 *)
val cfg_to_dot : cfg_type -> string option -> unit
