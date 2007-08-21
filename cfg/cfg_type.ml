(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cc_ml_types


(***********************************************************************
 *
 * Types
 *
 ***********************************************************************)

type function_id_type = {
  name : string list;
  param_types : string list;
}

let invalid_function_id = {
  name = []; param_types = []
}

let function_pointer_fun_id = {
  name = ["function pointer fun"];
  param_types = []
}

type function_def = {
  fun_id : function_id_type;
  loc : sourceLoc;
  oast : string;
  node_id : int;
  mutable callees : function_id_type list
}

type function_entry =
  | New of function_def * (function_id_type, unit) Hashtbl.t
  | Redef of function_def * 
      (function_id_type, unit) Hashtbl.t * 
      (function_id_type, unit) Hashtbl.t * 
      sourceLoc * bool ref

type cfg_type = (function_id_type, function_def) Hashtbl.t


type overload_hash_type = (string, function_id_type list) Hashtbl.t
