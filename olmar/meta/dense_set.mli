(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* densely populated set of positive integers
 * (use a bitmap internally)
 *)

(* type of sets (will use a string array as bitmap)
 * a freshly made set (returned from make) will occupy about 1KB
 * it will grow on demand to accomodate 
 * all positive integers (yes, even including max_int!)
 *)
type t

    (* test if the set is empty *)
val is_empty : t -> bool

    (* test if an positive integer is in the given set *)
val mem : int -> t -> bool

    (* add the integer to the set (using side effects) *)
val add : int -> t -> unit

    (* remove the integer to the set (using side effects) *)
val remove : int -> t -> unit

    (* create a fresh empty set *)
val make : unit -> t
