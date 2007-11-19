(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

module type Node_id = sig
  type t
  val string_of_t : t -> string
end


module type Dot_graph = sig
  type id

  type node

  exception Node_id_not_unique
  exception Graph_incomplete

  (* make_node id label_lines attributes childs *)
  val make_node : id -> string list -> (string * string) list -> 
    (id * (string option)) list -> node

  val make_node_unlabelled : 
    id -> string list -> (string * string) list -> id list -> node

  (* write_graph graph_name command_lines outfile_name *)
  val write_graph : string -> string list -> string option -> unit

  (* write_tree graph_name command_lines outfile_name *)
  val write_tree : string -> string list -> string option -> unit

  (* write_ordered_tree int_from_id graph_name command_lines 
   *                                           outfile_name tree roots
   * 
   *)
  val write_ordered_tree : 
    (id -> int) -> 
      string -> string list -> string option -> id list array -> id list -> unit

end


module Make(Node_id : Node_id) : Dot_graph
  with type id = Node_id.t
