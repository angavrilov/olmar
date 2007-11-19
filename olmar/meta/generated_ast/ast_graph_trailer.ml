(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)


(************************************************************************
 *
 * Node selection
 *
 ************************************************************************)

type node_selection =
  | All_nodes
  | Real_nodes
  | No_nodes
  | Add_node of int
  | Del_node of int
  | Loc of string option * int option * int option
  | Add_diameter of int
  | Del_noloc
  | Add_noloc
  | Normal_top_scope
  | Special_top_scope


let print_this_node node_array i = node_array.(i) <- true

let mark_noloc_iter _node_array _flag _i _node = ()
  (* 
   * no source locs in astgen asts
   * let loc_opt = super_source_loc node
   * in
   *   match loc_opt with
   *     | Some (file, line, char) ->
   * 	  if (file = "<noloc>" or file = "<init>")
   * 	    && line = 1 && char = 1 
   * 	  then
   * 	    node_array.(i) <- flag
   *     | None -> ()
   *)


let mark_noloc_nodes node_array ast_array flag =
  match ast_array with
    | Ast_oast ast_array -> 
	Superast.iteri (mark_noloc_iter node_array flag) ast_array


let mark_all_nodes node_array flag =
  for i = 0 to Array.length node_array -1 do
    node_array.(i) <- flag
  done


let mark_direction node_array ast_array visited dir_fun nodes = 
  let top_scope_opt = 
    match ast_array with
      | Ast_oast _ -> None
      (* 
       * | Elsa_oast ast_array ->
       * 	  (match ast_array.(1) with
       * 	     | CompilationUnit_type(_, _, (_,_, Some(scope))) -> 
       * 		 Some(id_annotation scope.poly_scope)
       * 	     | CompilationUnit_type(_, _, _) -> None
       * 	     | _ -> 
       * 		prerr_endline "Couldn't find top level node at index 1";
       * 		 assert false)
       *)
  in
  let rec doit = function
    | [] -> ()
    | (_, []) :: ll -> doit ll
    | (dia, i::l) :: ll ->
	if dia > visited.(i) then begin
	  print_this_node node_array i;
	  visited.(i) <- dia;
	  if (!normal_top_level_scope or top_scope_opt <> Some i)
	  then	     
	    doit (((max 0 (dia -1)), (dir_fun i)) :: (dia, l) :: ll)
	  else
	    doit ((dia, l) :: ll)
	end
	else
	    doit ((dia, l) :: ll)
  in
    doit nodes

let mark_real_nodes node_array ast_array down =
  let visited = Array.create (Array.length node_array) (-1) 
  in
    mark_direction node_array ast_array visited (fun i -> down.(i)) [0, [0]]


let mark_node_diameter node_array ast_array up down diameter =
  let rec get_current_selection i res = 
    if i = Array.length node_array 
    then res
    else
      get_current_selection (i+1) (if node_array.(i) then i::res else res)
  in
  let visited () = Array.create (Array.length node_array) 0 in    
  let start_selection = get_current_selection 1 [] 
  in
    mark_direction node_array ast_array (visited ())
      (fun i -> up.(i)) [(diameter +1, start_selection)];
    mark_direction node_array ast_array (visited ())
      (fun i -> down.(i)) [(diameter +1, start_selection)]


let match_file_line file line loc_opt =
  match loc_opt with
    | Some(lfile, lline, _) -> lfile = file && lline = line
    | None -> false

let match_line line loc_opt =
  match loc_opt with
    | Some(_, lline, _) -> lline = line
    | None -> false

let mark_line _node_array ast_array file line char =
  let _match_fun =
    match file,line,char with
      | (Some f, Some l, None) -> match_file_line f l
      | (None,   Some l, None) -> match_line l
      | _ -> assert false
  in
  let ast_doit _i _node = ()
    (* 
     * no sourcelocs in astgen asts
     * if match_fun (super_source_loc node) then
     *   print_this_node node_array i
     *)
  in
    match ast_array with
      | Ast_oast ast_array -> Superast.iteri ast_doit ast_array
  
  

let mark_nodes node_array ast_array up down sels = 
  List.iter
    (function
       | All_nodes -> mark_all_nodes node_array true
       | Real_nodes -> mark_real_nodes node_array ast_array down
       | No_nodes -> mark_all_nodes node_array false
       | Add_node i -> node_array.(i) <- true
       | Del_node i -> node_array.(i) <- false
       | Add_diameter i -> mark_node_diameter node_array ast_array up down i
       | Del_noloc -> mark_noloc_nodes node_array ast_array false
       | Add_noloc -> mark_noloc_nodes node_array ast_array true
       | Normal_top_scope -> normal_top_level_scope := true
       | Special_top_scope -> normal_top_level_scope := false
       | Loc(file,line,char) -> mark_line node_array ast_array file line char
    )
    sels



(************************************************************************
 *
 * global state and argument processing
 *
 ************************************************************************)

let out_file = ref (Some("nodes.dot"))

let size_flag = ref false

let dot_page_attribute = ref ""

let node_selections = ref []

let parse_loc_string s =
  try
    let first_colon = String.index s ':' 
    in
      try
	ignore(String.index_from s (first_colon +1) ':');
	(* two colons present in s *)
	Scanf.sscanf s "%s@:%d:%d"
	  (fun s l c -> (Some s, Some l, Some c))
      with
	| Not_found ->
	    (* one colon in s *)
	    Scanf.sscanf s "%s@:%d"
	      (fun s l -> (Some s, Some l, None))
  with
    | Not_found ->
	(* no colon in s *)
	Scanf.sscanf s "%d" 
	  (fun l -> (None, Some l, None))

let select x = 
  node_selections := x :: !node_selections

let out_file_fun s =
  if s = "-" 
  then out_file := None
  else out_file := Some s


let arguments = Arg.align
  [
    ("-all", Arg.Unit (fun () -> select All_nodes),
     " select all nodes");
    ("-real", Arg.Unit (fun () -> select Real_nodes),
     " select tree with root node 0");
    ("-none", Arg.Unit (fun () -> select No_nodes),
     " unselect all nodes");
    ("-loc", 
     Arg.String (fun loc -> 
		   let (f,l,c) = parse_loc_string loc
		   in
		     select (Loc (f,l,c))),
     "loc select location, loc is of the form [file:]line[:char]");
    ("-node", Arg.Int (fun i -> select(Add_node i)),
     "node_id add node_id");
    ("-del-node", Arg.Int (fun i -> select(Del_node i)),
     "node_id del node_id");
    ("-dia", Arg.Int (fun i -> select(Add_diameter i)),
     "n select diameter around currently selected nodes");
    ("-noloc", Arg.Unit (fun () -> select Del_noloc),
     " unselect nodes with a <noloc> location");
    ("-with-noloc", Arg.Unit (fun () -> select Add_noloc),
     " select nodes with a <noloc> location");
    ("-normal-top-scope", Arg.Unit (fun () -> select Normal_top_scope),
     " don't treat top level scope special");
    ("-special-top-scope", Arg.Unit (fun () -> select Special_top_scope),
     " don't treat top level scope special");
    ("-o", Arg.String out_file_fun,
     "file set output file name [default nodes.dot]");
    ("-size", Arg.Set size_flag,
     " limit size of output");
    ("-page", Arg.Set_string dot_page_attribute,
     "pattr add pattr as page attribute in the output");
  ]


(************************************************************************
 *
 * main
 *
 ************************************************************************)


(* 
 * let start_file infile =
 *   output_string !oc "digraph ";
 *   Printf.fprintf !oc "\"%s\"" infile;
 *   output_string !oc " {\n";
 *   if !size_flag then
 *     output_string !oc "    size=\"90,90\";\n";
 *   if !dot_page_attribute <> "" then
 *     Printf.fprintf !oc "    page=\"%s\";\n" !dot_page_attribute;
 *   output_string !oc 
 *     "    color=white;\n";
 *   output_string !oc
 *     "    ordering=out;\n";
 *   output_string !oc
 *     "    node [ style = filled ];\n";
 *   output_string !oc
 *     "    edge [ arrowtail=odot ];\n"
 *)


let dot_commands () =
  (if !size_flag then ["size=\"90,90\""] else [])
  @ (if !dot_page_attribute <> "" then
       [Printf.sprintf "page=\"%s\"" !dot_page_attribute]
     else []
    )
  @ ["color=white";
     "splines=true";
     "node [ style = filled ]";
     "edge [ arrowtail=odot ]"
    ]



let real_node_selection = function
  | All_nodes
  | Real_nodes
  | No_nodes
  | Add_node _
  | Del_node _
  | Loc _
  | Add_diameter _
  | Del_noloc
  | Add_noloc
      -> true
  | Normal_top_scope
  | Special_top_scope
      -> false


let oast_action f = function
  | Ast_oast ast -> Ast_oast(f ast)


let do_marked_nodes node_array ast_array =
  for i = 0 to Array.length node_array -1 do
    if node_array.(i) then
      match ast_array with
	| Ast_oast ast_array -> 
	    ignore(ast_node_fun node_array ast_array.(i))
  done

let main () =
  let (oast_file, max_node, ast, first_selection) = 
    let (file, max, ast_ast) = 
      setup_oast_translation arguments "ast_graph"
    in
      (file, max, Ast_oast ast_ast, All_nodes)
  in
  let (ast_array, _tree_node_order) = 
    match ast with
      | Ast_oast ast -> 
	  let (ast_array, tree_node_order) = Superast.into_array max_node ast 
	  in
	    (Ast_oast ast_array, tree_node_order)
  in
  let (up, down) = match ast_array with
    | Ast_oast ast_array -> Uplinks.create ast_array 
  in
  let node_array = Array.create (max_node +1) false in
  let sels = List.rev !node_selections in
  let sels = 
    match List.filter real_node_selection sels with
      | (Del_node _) :: _
      | Del_noloc :: _
      | []
	-> first_selection :: sels

      | Loc _ :: _
      | All_nodes :: _
      | Real_nodes :: _
      | No_nodes :: _
      | (Add_node _) :: _ 
      | (Add_diameter _) :: _
      | Add_noloc :: _
	-> sels
      | Normal_top_scope :: _
      | Special_top_scope :: _
	-> assert false
  in
    mark_nodes node_array ast_array up down sels;
    do_marked_nodes node_array ast_array;
    G.write_tree oast_file (dot_commands()) !out_file;
    Printf.printf "graph with %d nodes and %d edges generated\n%!"
      !nodes_counter !edge_counter
      
;;


Printexc.catch main ()


