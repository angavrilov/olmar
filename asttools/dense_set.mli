

type t

    (* test if an positive integer is in the given set *)
val mem : int -> t -> bool

    (* add the integer to the set (using side effects) *)
val add : int -> t -> unit

    (* create a fresh empty set *)
val make : unit -> t
