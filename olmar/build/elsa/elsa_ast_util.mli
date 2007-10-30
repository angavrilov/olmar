(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Elsa_reflect_type

val is_named_atomic_type : 'a atomicType_type -> bool

val is_named_atomic_type_option : 'a atomicType_type option -> bool

val name_of_function : 'a function_type -> string

val body_of_function : 'a function_type -> 'a statement_type
