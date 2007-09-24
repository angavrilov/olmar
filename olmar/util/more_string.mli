(*  Copyright 2007 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* string_match_left space pattern checks if space starts with pattern *)
val string_match_left : string -> string -> bool

(* translate from tos s replaces in s all characters from from with the
 * corresponding ones in tos
 *)
val translate : string -> string -> string -> string

(* erases leading and trailing white space *)
val trim_white_space : string -> string


(* print an ocaml comment to channel *)
val pr_comment : out_channel -> string list -> unit

(* print a c comment to channel *)
val pr_c_comment : out_channel -> string list -> unit

(* print a line number directive *)
val pr_linenumber_directive : int -> string -> out_channel -> unit

(* DO NOT EDIT *)
val do_not_edit_line : string

val star_line : string
