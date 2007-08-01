(*
 * $Id$
 *
 * cf. SML'97 Basis Library: The "Option" structure
 *     http://www.smlnj.org/doc/basis/pages/option.html
 *     (Last Modified January 22, 1997)
 *
 * Author: Tjark Weber, (c) 2007 Radboud University
 *)

(* ------------------------------------------------------------------------- *)

module type OPTION =
sig

  exception Option

  val app : ('a -> unit) -> 'a option -> unit
  val compose : ('a -> 'b) * ('c -> 'a option) -> 'c -> 'b option
  val composePartial : ('a -> 'b option) * ('c -> 'a option) -> 'c -> 'b option
  val filter : ('a -> bool) -> 'a -> 'a option
  val getOpt : 'a option * 'a -> 'a
  val isSome : 'a option -> bool
  val join : 'a option option -> 'a option
  val map : ('a -> 'b) -> 'a option -> 'b option
  val mapPartial : ('a -> 'b option) -> 'a option -> 'b option
  val valOf : 'a option -> 'a

end  (* OPTION *)

(* ------------------------------------------------------------------------- *)

module Option : OPTION =
struct

  exception Option

  let app f = function
      None   -> ()
    | Some x -> f x

  let compose (g, f) x =
    match f x with
        None   -> None
      | Some y -> Some (g y)

  let composePartial (g, f) x =
    match f x with
        None   -> None
      | Some y -> g y

  let filter f x =
    if f x then Some x else None

  let getOpt = function
      (None, d) -> d
    | (Some x, _) -> x

  let isSome = function
      None -> false
    | Some _ -> true

  let join = function
      None -> None
    | Some x -> x

  let map f = function
      None -> None
    | Some x -> Some (f x)

  let mapPartial f = function
      None -> None
    | Some x -> f x

  let valOf = function
      None -> raise Option
    | Some x -> x

end  (* Option *)
