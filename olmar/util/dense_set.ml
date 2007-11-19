(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* densely populated set of positive integers
 * (use a bitmap internally)
 *)

(* type of sets: use big string arrays as bitmaps 
 * on 32 bit architecture the array holds 9 strings
 * on 64 bit architecture it holds 5 string
 * initially the strings are empty, but they grow on demand 
 * to accomodate all positive integers (yes, including max_int!) *)
type t = string array

(* start size: just small enough to end up in the minor heap *)
let string_min_length = 1023

(* expand the string with index string_i to accomodate byte number char_i*)
let expand set string_i char_i =
  let new_size = 2.0 ** (ceil (log (float_of_int char_i) /. log 2.0) +. 0.25) in
  let new_size = int_of_float(new_size +. 0.5) in
  let new_size = max string_min_length (min new_size Sys.max_string_length) in
  let s = String.create new_size in
  let old_len = String.length set.(string_i)
  in
    String.blit set.(string_i) 0 s 0 old_len;
    String.fill s old_len (new_size - old_len) '\000';
    set.(string_i) <- s

(* compute array and byte index for bit number i *)
let get_position i =
  (i / ( Sys.max_string_length * 8),
   (i mod ( Sys.max_string_length * 8)) / 8,
   1 lsl (i mod 8))



(* array size to accomodate all positive integers *)
let array_size = 
  int_of_float 
    (ceil
       ((float_of_int max_int) /. (float_of_int Sys.max_string_length) /. 8.0))

(* create a new set *)
let make () =
  let a = Array.make array_size ""
  in
    a.(0) <- String.make string_min_length '\000';
    a

(* check if the set is empty *)
exception Nonempty_dense_set

let is_empty set =
  try
    for i = 0 to array_size -1 do
      for j = 0 to String.length set.(i) -1 do
	if set.(i).[j] <> '\000' then
	  raise Nonempty_dense_set
      done
    done;
    true
  with
    | Nonempty_dense_set -> false
  

(* check whether el is in the set set *)
let mem el set =
  let (string_i, char_i, mask) = get_position el
  in
    if char_i >= String.length set.(string_i)
    then false
    else
      ((int_of_char set.(string_i).[char_i]) land mask) <> 0

(* 
 * let mem el set =
 *   try
 *     mem el set
 *   with
 *     | Invalid_argument("index out of bounds") as exc ->
 * 	Printf.eprintf "Index out of bounds: Dense_set.mem %d\n%!" el;
 * 	raise exc
 *)
	

(* add el to the set set *)
let add el set =
  let (string_i, char_i, mask) = get_position el
  in
    if char_i >= String.length set.(string_i)
    then
      expand set string_i char_i;
    set.(string_i).[char_i] <- 
      char_of_int ((int_of_char set.(string_i).[char_i]) lor mask)

(* 
 * let add el set =
 *   try
 *     add el set
 *   with
 *     | Invalid_argument("index out of bounds") as exc ->
 * 	Printf.eprintf "Index out of bounds: Dense_set.add %d\n%!" el;
 * 	raise exc
 *)


(* remove el from the set set *)
let remove el set =
  let (string_i, char_i, mask) = get_position el
  in
    if char_i < String.length set.(string_i) &&
      ((int_of_char set.(string_i).[char_i]) land mask) <> 0
    then
      set.(string_i).[char_i] <- 
	char_of_int ((int_of_char set.(string_i).[char_i]) lxor mask)


(* interval ds start end in_set
 * checks if all elements in the closed interval start - end
 * are in the set (for in_set = true) or 
 * not in the set (for in_set = false)
 * 
 * val interval : t -> int -> int -> bool -> bool
 *)
let interval ds start ende in_set =
  let res = ref true in
  let start_fast = start - (start mod 8) + 8 in
  let (start_i, start_b, _) = get_position start_fast in
  let end_fast = ende - (ende mod 8) -1 in
  let (end_i, end_b, _) = get_position end_fast in
  let expected_val = if in_set then '\255' else '\000'
  in
    for i = start to start_fast -1 do
      if mem i ds <> in_set
      then
	res := false
    done;
    for i = start_i to end_i do
      let len_i = String.length ds.(i)
      in
	for b = (if i = start_i then start_b else 0) 
	to (if i = end_i then end_b else Sys.max_string_length)
	do
	  if (if b < len_i then ds.(i).[b] else '\000') <> expected_val 
	  then 
	    res := false
	done
    done;
    for i = end_fast + 1 to ende do
      if mem i ds <> in_set
      then
	res := false
    done;
    !res
    
