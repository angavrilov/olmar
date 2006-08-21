
type t = string array

    (* just small enough to end up in the minor heap *)
let string_min_length = 1023

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

let get_position i =
  (i / ( Sys.max_string_length * 8),
   (i mod ( Sys.max_string_length * 8)) / 8,
   1 lsl (i mod 8))

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


let array_size = 
  int_of_float 
    (ceil
       (float_of_int max_int) /. (float_of_int Sys.max_string_length) /. 8.0)

let make () =
  let a = Array.make array_size ""
  in
    a.(0) <- String.make string_min_length '\000';
    a
