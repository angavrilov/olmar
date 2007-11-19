(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)


(************************************************************************
 *
 * Node selection
 *
 ************************************************************************)

let is_scope_node ast_array i =
  match ast_array.(i) with
    | No_ast_node -> assert false
    | Scope_type _ -> true
    | CompilationUnit_type _
    | TranslationUnit_type _
    | TopForm_type _
    | Function_type _
    | MemberInit_type _
    | Declaration_type _
    | ASTTypeId_type _
    | PQName_type _
    | TypeSpecifier_type _
    | BaseClassSpec_type _
    | Enumerator_type _
    | MemberList_type _
    | Member_type _
    | ExceptionSpec_type _
    | OperatorName_type _
    | Statement_type _
    | Condition_type _
    | Handler_type _
    | Expression_type _
    | FullExpression_type _
    | ArgExpression_type _
    | ArgExpressionListOpt_type _
    | Initializer_type _
    | TemplateDeclaration_type _
    | TemplateParameter_type _
    | TemplateArgument_type _
    | NamespaceDecl_type _
    | Declarator_type _
    | IDeclarator_type _
    | FullExpressionAnnot_type _
    | ASTTypeof_type _
    | Designator_type _
    | AttributeSpecifierList_type _
    | AttributeSpecifier_type _
    | Attribute_type _
    | Variable_type _
    | OverloadSet_type _
    | TemplateInfo_type _
    | InheritedTemplateParams_type _
    | BaseClass_type _
    | BaseClassSubobj_type _
    | EnumValue_type _
    | AtomicType_type _
    | CompoundType_type _
    | FunctionExnSpec_type _
    | CType_type _
    | STemplateArgument_type _
	-> false


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
  | Normal_scope
  | Special_scope
  | Normal_top_scope
  | Special_top_scope


let print_this_node node_array i = node_array.(i) <- true

let mark_noloc_iter node_array flag i node =
  let loc_opt = super_source_loc node
  in
    match loc_opt with
      | Some (file, line, char) ->
  	  if (file = "<noloc>" or file = "<init>")
  	    && line = 1 && char = 1 
  	  then
  	    node_array.(i) <- flag
      | None -> ()


let mark_noloc_nodes node_array ast_array flag =
  Superast.iteri (mark_noloc_iter node_array flag) ast_array


let mark_all_nodes node_array flag =
  for i = 0 to Array.length node_array -1 do
    node_array.(i) <- flag
  done


let mark_direction node_array ast_array visited dir_fun nodes = 
  let top_scope_opt = 
    match ast_array.(0) with
      | CompilationUnit_type {unit = { globalScope = Some(scope) }} ->
      	  Some(id_annotation scope.scope_annotation)
      | CompilationUnit_type _ -> None
      | _ -> 
      	  prerr_endline "Couldn't find top level node at index 1";
      	  assert false
  in
  let rec doit = function
    | [] -> ()
    | (_, []) :: ll -> doit ll
    | (dia, i::l) :: ll ->
	if dia > visited.(i) then begin
	  print_this_node node_array i;
	  visited.(i) <- dia;
	  if (top_scope_opt = Some i && not !normal_top_level_scope) or
	    (top_scope_opt <> Some i &&
	       is_scope_node ast_array i && not !normal_scope)
	  then	     
	    doit ((dia, l) :: ll)
	  else
	    doit (((max 0 (dia -1)), (dir_fun i)) :: (dia, l) :: ll)
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
  let start_selection = get_current_selection 0 [] 
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

let mark_line node_array ast_array file line char =
  let match_fun =
    match file,line,char with
      | (Some f, Some l, None) -> match_file_line f l
      | (None,   Some l, None) -> match_line l
      | _ -> assert false
  in
  let ast_doit i node =
    if match_fun (super_source_loc node) then
      print_this_node node_array i
  in
    Superast.iteri ast_doit ast_array
  
  

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
       | Normal_scope -> normal_scope := true
       | Special_scope -> normal_scope := false
       | Normal_top_scope -> normal_top_level_scope := true
       | Special_top_scope -> normal_top_level_scope := false
       | Loc(file,line,char) -> mark_line node_array ast_array file line char
    )
    sels


(************************************************************************
 *
 * direct child array
 *
 ************************************************************************)

module DS = Dense_set

let is_variable_or_scope_node ast_array id =
    match ast_array.(id) with
      | No_ast_node -> assert false
      | CompilationUnit_type _ 
      | TranslationUnit_type _ 
      | TopForm_type _
      | Function_type _ 
      | MemberInit_type _ 
      | Declaration_type _ 
      | ASTTypeId_type _ 
      | PQName_type _
      | TypeSpecifier_type _
      | BaseClassSpec_type _ 
      | Enumerator_type _
      | MemberList_type _ 
      | Member_type _
      | ExceptionSpec_type _ 
      | OperatorName_type _ 
      | Statement_type _
      | Condition_type _ 
      | Handler_type _ 
      | Expression_type _ 
      | FullExpression_type _ 
      | ArgExpression_type _ 
      | ArgExpressionListOpt_type _ 
      | Initializer_type _
      | TemplateDeclaration_type _ 
      | TemplateParameter_type _
      | TemplateArgument_type _ 
      | NamespaceDecl_type _ 
      | Declarator_type _ 
      | IDeclarator_type _
      | FullExpressionAnnot_type _ 
      | ASTTypeof_type _ 
      | Designator_type _
      | AttributeSpecifierList_type _ 
      | AttributeSpecifier_type _ 
      | Attribute_type _
      | OverloadSet_type _ 
      | TemplateInfo_type _
      | InheritedTemplateParams_type _ 
      | BaseClass_type _ 
      | BaseClassSubobj_type _ 
      | EnumValue_type _ 
      | AtomicType_type _ 
      | CompoundType_type _ 
      | FunctionExnSpec_type _ 
      | CType_type _ 
      | STemplateArgument_type _ 
	-> false

      | Scope_type _ 
      | Variable_type _ -> true


let filter_scope_option ast_array up all_childs = function
  | None -> all_childs
  | Some scope ->
      if List.for_all 
	(is_variable_or_scope_node ast_array) 
	up.(id_annotation (scope_annotation scope))
      then
	all_childs
      else
	List.filter 
	  (fun id -> id <> id_annotation (scope_annotation scope))
	  all_childs


let filter_scopes ast_array up down node_id =
  match ast_array.(node_id) with
    | No_ast_node -> assert false
    | CompilationUnit_type _ 
    | TranslationUnit_type _ 
    | TopForm_type _
    | Function_type _ 
    | MemberInit_type _ 
    | Declaration_type _ 
    | ASTTypeId_type _ 
    | PQName_type _
    | TypeSpecifier_type _
    | BaseClassSpec_type _ 
    | Enumerator_type _
    | MemberList_type _ 
    | Member_type _
    | ExceptionSpec_type _ 
    | OperatorName_type _ 
    | Statement_type _
    | Condition_type _ 
    | Handler_type _ 
    | Expression_type _ 
    | FullExpression_type _ 
    | ArgExpression_type _ 
    | ArgExpressionListOpt_type _ 
    | Initializer_type _
    | TemplateDeclaration_type _ 
    | TemplateParameter_type _
    | TemplateArgument_type _ 
    | NamespaceDecl_type _ 
    | Declarator_type _ 
    | IDeclarator_type _
    | FullExpressionAnnot_type _ 
    | ASTTypeof_type _ 
    | Designator_type _
    | AttributeSpecifierList_type _ 
    | AttributeSpecifier_type _ 
    | Attribute_type _
    | OverloadSet_type _ 
    | TemplateInfo_type _
    | InheritedTemplateParams_type _ 
    | BaseClass_type _ 
    | BaseClassSubobj_type _ 
    | EnumValue_type _ 
    | AtomicType_type _ 
    | CompoundType_type _ 
    | FunctionExnSpec_type _ 
    | CType_type _ 
    | STemplateArgument_type _ 
	-> down.(node_id)

    | Scope_type s ->
	filter_scope_option ast_array up down.(node_id) s.parentScope
    | Variable_type v ->
	filter_scope_option ast_array up down.(node_id) v.scope


let rec direct_child_rec (ast_array : 'a array) up down ds direct_childs node_id =
  assert(not (DS.mem node_id ds));
  DS.add node_id ds;
  List.iter
    (fun child_id ->
       if not (DS.mem child_id ds) 
       then
	 begin
	   direct_childs.(node_id) <- child_id :: direct_childs.(node_id);
	   direct_child_rec ast_array up down ds direct_childs child_id
	 end)
    (filter_scopes ast_array up down node_id);
  direct_childs.(node_id) <- List.rev direct_childs.(node_id)


let direct_child_array ast_array up down =
  let len = Array.length down in
  let ds = DS.make () in
  let direct_childs = Array.create len []
  in
    assert(Array.length up = len && Array.length ast_array = len);
    direct_child_rec ast_array up down ds direct_childs 0;
    assert(DS.interval ds 0 (len -1) true);
    direct_childs




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
    ("-normal-scope", Arg.Unit (fun () -> select Normal_scope),
     " don't treat scopes special");
    ("-special-scope", Arg.Unit (fun () -> select Special_scope),
     " treat scopes special");
    ("-normal-top-scope", Arg.Unit (fun () -> select Normal_top_scope),
     " don't treat top level scope special");
    ("-special-top-scope", Arg.Unit (fun () -> select Special_top_scope),
     " treat top level scope special");
    ("-field-len", Arg.Set_int max_string_length,
     "len set maximal field length");
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
  | Normal_scope
  | Special_scope
  | Normal_top_scope
  | Special_top_scope
      -> false


let do_marked_nodes node_array ast_array =
  for i = 0 to Array.length node_array -1 do
    if node_array.(i) then
      ignore(ast_node_fun node_array ast_array.(i))
  done

let main () =
  let (oast_file, max_node, ast) = setup_oast arguments "ast_graph"
  in
  let (ast_array, _tree_node_order) = Superast.into_array max_node ast in
  let (up, down) = Uplinks.create ast_array in
  let node_array = Array.create (max_node +1) false in
  let sels = List.rev !node_selections in
  let sels = 
    match List.filter real_node_selection sels with
      | (Del_node _) :: _
      | Del_noloc :: _
      | []
	-> Real_nodes :: sels

      | Loc _ :: _
      | All_nodes :: _
      | Real_nodes :: _
      | No_nodes :: _
      | (Add_node _) :: _ 
      | (Add_diameter _) :: _
      | Add_noloc :: _
	-> sels
      | Normal_scope :: _
      | Special_scope :: _
      | Normal_top_scope :: _
      | Special_top_scope :: _
	-> assert false
  in
    mark_nodes node_array ast_array up down sels;
    do_marked_nodes node_array ast_array;
    (* G.write_dot_file oast_file (dot_commands()) !out_file; *)
    G.write_ordered_tree (fun x -> x) oast_file (dot_commands()) !out_file
      (direct_child_array ast_array up down) 
      [0];
    Printf.printf "graph with %d nodes and %d edges generated\n%!"
      !nodes_counter !edge_counter
      
;;


Printexc.catch main ()


