(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cfg_type

let fun_id_name fun_id = String.concat "::" fun_id.name

let fun_def_name fun_def = fun_id_name fun_def.fun_id
