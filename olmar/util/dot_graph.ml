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


module Make(Node_id : Node_id) = struct

  module DS = Dense_set

  type id = Node_id.t

  type 'a child_link =
    | Unlabelled of 'a 
    | Labelled of 'a * string

  type node = {
    external_id : id;
    internal_id : int;
    label_lines : string list;
    attributes : (string * string) list;
    childs : child_type child_link list;
    mutable opt_childs : node child_link list;
    mutable ext_child_hash : (id, unit) Hashtbl.t option;
    mutable parents : node list
  }

  and child_type =
    | Child_id of id
    | Child_node of node

  let string_of_t = Node_id.string_of_t

  let char_width = 9

  let node_padding = 20

  let min_width = 50

  let row_space = 150


  let node_hash : (id, node) Hashtbl.t = Hashtbl.create 1543

  let unresolved_parents : (id, node list ref) Hashtbl.t = Hashtbl.create 1543

  let clear () =
    Hashtbl.clear node_hash;
    Hashtbl.clear unresolved_parents
    

  let next_interal_id = ref 0

  let internal_id () =
    let res = !next_interal_id
    in
      incr next_interal_id;
      res

  let hashtbl_find_with_error msg hash key =
    try
      Hashtbl.find hash key
    with
      | Not_found as x->
	  Printf.eprintf msg (string_of_t key);
	  raise x


  let get_childs node =
    if node.opt_childs <> [] then node.opt_childs
    else if node.childs = [] then []
    else
      let optc = 
	List.map
	  (function
	     | Labelled(Child_id id, l) -> 
		 Labelled(hashtbl_find_with_error "child %s missing\n%!"
			    node_hash id, l)
	     | Unlabelled(Child_id id) ->
		 Unlabelled(hashtbl_find_with_error "child %s missing\n%!"
			      node_hash id)
	     | Labelled(Child_node node, l) -> 
		 Labelled(node, l)
	     | Unlabelled(Child_node node) ->
		 Unlabelled(node))
	  node.childs
      in
	node.opt_childs <- optc;
	optc

  let get_childs_unlabelled node =
    List.map 
      (function
	 | Labelled(c, _)
	 | Unlabelled c -> c)
      (get_childs node)


  let get_ext_child_hash node =
    match node.ext_child_hash with
      | Some h -> h
      | None ->
	  let h = Hashtbl.create 13
	  in
	    List.iter
	      (fun c -> Hashtbl.add h c.external_id ())
	      (get_childs_unlabelled node);
	    node.ext_child_hash <- Some h;
	    h


  let set_parent parent = function
    | Labelled(Child_node child, _)
    | Unlabelled(Child_node child) ->
	child.parents <- parent :: child.parents

    | Labelled(Child_id child_id, _)
    | Unlabelled(Child_id child_id) ->
	try
	  let other_parents = Hashtbl.find unresolved_parents child_id
	  in
	    other_parents := parent :: !other_parents
	with
	  | Not_found ->
	      Hashtbl.add unresolved_parents child_id (ref [parent])


  exception Node_id_not_unique
  exception Graph_incomplete

  let make_node id label attr childs = 
    if Hashtbl.mem node_hash id then
      raise Node_id_not_unique
    else
      let childs =
	List.map 
	  (fun (id, label_opt) -> 
	     let child =
	       try
		 Child_node(Hashtbl.find node_hash id)
	       with
		 | Not_found -> Child_id id
	     in
	       match label_opt with
		 | None -> Unlabelled child
		 | Some l -> Labelled(child, l))
	  childs;
      in
      let node = {
	external_id = id;
	internal_id = internal_id();
	attributes = attr;
	label_lines = label;
	childs = childs;
	opt_childs = [];
	ext_child_hash = None;
	parents = 
	  try 
	    !(Hashtbl.find unresolved_parents id)
	  with
	    | Not_found -> []
      }
      in
	Hashtbl.remove unresolved_parents id;
	Hashtbl.add node_hash id node;
	List.iter (set_parent node) childs;
	(try
	   (* check whether circles added new parents in unresolved_parents *)
	   let new_parents = !(Hashtbl.find unresolved_parents id)
	   in
	     assert(List.for_all (fun parent -> parent == node) new_parents);
	     Hashtbl.remove unresolved_parents id;
	     node.parents <- new_parents @ node.parents
	 with
	   | Not_found -> ());
	node

  let make_node_unlabelled id label attr childs =
    make_node id label attr (List.map (fun c -> (c, None)) childs)


  let check_for_complete_graph () =
    if Hashtbl.length unresolved_parents <> 0
    then 
      begin
	Hashtbl.iter 
	  (fun id _parents -> 
	     Printf.eprintf "node %s missing\n" (string_of_t id))
	  unresolved_parents;
	raise Graph_incomplete
      end


  let get_root_nodes () =
    let _ = assert(Hashtbl.length unresolved_parents = 0) in
    let roots = ref []
    in
      Hashtbl.iter 
	(fun _ node -> 
	   if node.parents = [] 
	   then
	     roots := node :: !roots
	)
	node_hash;
      !roots


  let dot_escape s =
    let b = Buffer.create (max 31 (String.length s))
    in
      for i = 0 to String.length s -1 do
	match s.[i] with
	  | '\\' -> Buffer.add_string b "\\\\"
	  | '"' -> Buffer.add_string b "\\\""
	  | '\n' -> Buffer.add_string b "\\n"
	  | c -> Buffer.add_char b c
      done;
      Buffer.contents b


  let write_node_attribute oc (name, value) =
    Printf.fprintf oc ", %s=\"%s\""
      name
      (dot_escape value)



  let write_node oc node more_attributes =
    Printf.fprintf oc "    \"%s\" [label=\"%s\"" 
      (string_of_int node.internal_id)
      (dot_escape (String.concat "\n" node.label_lines));

    List.iter (write_node_attribute oc) more_attributes;
    List.iter (write_node_attribute oc) node.attributes;

    output_string oc "];\n";

    List.iter
      (function
	 | Labelled(child, label) ->
	     Printf.fprintf oc "    \"%d\" -> \"%d\" [label=\"%s\"];\n"
	       node.internal_id child.internal_id
	       (dot_escape label)
	 | Unlabelled child ->
	     Printf.fprintf oc "    \"%d\" -> \"%d\";\n"
	       node.internal_id child.internal_id
      )
      (get_childs node)


  let write_node_pos oc level left right node =
    let max_len = 
      List.fold_right
	(fun s m -> max (String.length s) m)
	node.label_lines
	0
    in
    let width = max (max_len * char_width + node_padding) min_width in
    let x = 
      if left = right
      then
	left + width / 2
      else
	((left + right) / 2) 
    in
    let y = -level * row_space
    in
      write_node oc node [("pos", Printf.sprintf "%d,%d" x y)];
      width


  let rec recurse_write oc ds node level current_x =
    if DS.mem node.internal_id ds 
    then 
      begin
	current_x
      end
    else 
      let _ = DS.add node.internal_id ds in
      let left_margin = current_x in
      let childs = get_childs_unlabelled node in
      let direct_childs = 
	List.filter (fun node -> not(DS.mem node.internal_id ds)) childs
      in
      let right_margin = 
	recurse_write_list oc ds direct_childs (level +1) left_margin
      in
      let width = write_node_pos oc level left_margin right_margin node
      in
	max right_margin (left_margin + width)


  and recurse_write_list oc ds node_list level current_x =
    List.fold_left
      (fun current_x node -> 
	 recurse_write oc ds node level current_x)
      current_x
      node_list
	  
	
  let write_header oc name graph_commands =
    Printf.fprintf oc "digraph \"%s\" {\n" (dot_escape name);
    List.iter
      (fun line ->
	 Printf.fprintf oc "    %s;\n" line)
      graph_commands


  let write_footer oc =
    output_string oc "}\n"


  let write_graph name graph_commands outfile =
    let oc = 
      match outfile with
  	| None -> stdout
  	| Some f -> open_out f
    in
      write_header oc name graph_commands;
      Hashtbl.iter (fun _ node -> write_node oc node []) node_hash;
      write_footer oc;
      (match outfile with
	 | Some _ -> close_out oc
	 | None -> ())	   


  let write_tree name graph_commands outfile =
    let ds = DS.make () in
    let oc = 
      match outfile with
	| None -> stdout
	| Some f -> open_out f
    in
    let _ = check_for_complete_graph () in
    let roots = get_root_nodes()
    in
      write_header oc name graph_commands;
      assert(roots <> []);
      ignore(recurse_write_list oc ds roots 0 0);
      write_footer oc;
      (match outfile with
	 | Some _ -> close_out oc
	 | None -> ())	   
   

  let rec write_ordered_subtree oc id_from_int tree node_id level current_x =
    let left_margin = current_x in
    let right_margin = 
      write_ordered_subtree_list oc id_from_int tree (level +1) 
	left_margin tree.(id_from_int node_id)
    in
    let width = 
      if Hashtbl.mem node_hash node_id
      then
	write_node_pos oc level left_margin right_margin 
	  (Hashtbl.find node_hash node_id)
      else 0
    in
      max right_margin (left_margin + width)

  and write_ordered_subtree_list 
      oc id_from_int tree level current_x node_id_list =
    List.fold_left
      (fun current_x node_id -> 
	 write_ordered_subtree oc id_from_int tree node_id level current_x)
      current_x
      node_id_list


  let write_ordered_tree id_from_int name graph_commands outfile tree roots =
    let oc = 
      match outfile with
	| None -> stdout
	| Some f -> open_out f
    in 
      write_header oc name graph_commands;
      ignore(
	List.fold_left
	  (fun current_x root_id ->
	     write_ordered_subtree oc id_from_int tree root_id 0 current_x)
	  0
	  roots);
      write_footer oc;
      (match outfile with
	 | Some _ -> close_out oc
	 | None -> ())	   
   
end
