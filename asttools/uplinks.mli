(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Superast
open Ast_annotation

val create : annotated super_ast array -> (int list array * int list array)
