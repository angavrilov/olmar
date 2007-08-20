(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* generate a dot graph from the ast
 *
 * to make use of it you need the graphviz package (http://www.graphviz.org/)
 * 
 * to visualize the graph 
 * - zgrviewer on the dot file (http://zvtm.sourceforge.net/zgrviewer.html)
 * - run dot -Tps nodes.dot > nodes.ps
 *        if ghostview cannot allocate a bitmap big enough to hold 
 *        the graph you can experiment with the -size option or change
 *        the dot file to get multiple pages
 * - run dot -Tfig nodes.dot > nodes.fig  and use xfig
 *        works surprisingly good, even for the biggest graphs
 * - consult the dot documentation for other options (jpeg for instance)
 *)          

open Cc_ml_types
open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation
open Ast_accessors
open Superast



(* node type done: 1 - 30, 32, 33, 36 - 40
   not done: 31, 34, 35,
 *)

(* some global variables *)
let oc = ref stdout;;

let print_node = ref (Array.create 0 false)

let print_caddr = ref false



module DS = Dense_set

let visited_nodes = DS.make ()

let visited (annot : annotated) =
  DS.mem (id_annotation annot) visited_nodes

let visit (annot : annotated) =
  (* Printf.eprintf "visit %d\n%!" (id_annotation annot); *)
  DS.add (id_annotation annot) visited_nodes


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

let any_label (field_name, value) =
  Printf.fprintf !oc "\\n%s: %s" field_name (dot_escape value)

let string_opt = function
  | None -> "(nil)"
  | Some s -> s

let count_rev base l =
  let counter = ref (List.length l)
  in
    List.rev_map 
      (fun x -> decr counter; (x, Printf.sprintf "%s[%d]" base !counter)) 
      l

let nodes_counter = ref 0

let start_label name color id =
  incr nodes_counter;
  Printf.fprintf !oc "    \"%d\" [color=\"%s\", label=\"%s %d" 
    id color name id

let finish_label _caddr =
  output_string !oc "\"];\n"


let loc_label (file, line, char) =
  ("loc", Printf.sprintf "%s:%d:%d" file line char)

let edge_counter = ref 0

let child_edge id (cid,label) =
  if !print_node.(id) && !print_node.(cid)
  then begin
    incr edge_counter;
    Printf.fprintf !oc "    \"%d\" -> \"%d\" [label=\"%s\"];\n" id cid label
  end

let child_edges id childs = 
  List.iter (child_edge id) childs

  
let opt_child child_fun field_name opt = 
  match opt with
    | None -> []
    | Some c -> [(child_fun c, field_name)]

let string_hash_child child_fun field_name hash =
  Hashtbl.fold 
    (fun string_key value res ->
       (child_fun value,
	Printf.sprintf "%s{%s}" field_name string_key) :: res)
    hash
    []


let caddr_label caddr =
  ("caddr", Printf.sprintf "0x%lx" (Int32.shift_left (Int32.of_int caddr) 1))

let ast_node color annot name (labels :(string*string) list) childs =
  let id = id_annotation annot
  in
    if !print_node.(id) 
    then begin
      start_label name color id;
      List.iter any_label 
	(if !print_caddr then
	   labels @ [(caddr_label (caddr_annotation annot))]
	 else
	   labels);
      finish_label ();
      child_edges id childs;
      (* the return value must be the same as retval annot: *)
      (* assert(id = retval annot); *)
      id
    end
    else
      id


let retval annot = id_annotation annot


let ast_loc_node color annot loc name labels childs =
  ast_node color annot name ((loc_label loc) :: labels) childs


(***************** colors *****************************)

let color_CompilationUnit = "LightCoral"
let color_TranslationUnit = "red"
let color_TF = "firebrick2"
let color_Declaration = "cyan"
let color_Declarator = "SkyBlue"
let color_Function = "magenta"
let color_MemberInit = "HotPink"
let color_IDeclarator = "SteelBlue"
let color_Member = "SlateBlue1"
let color_MemberList = "SlateBlue4"
let color_PQName = "SkyBlue"
let color_Variable = "purple"
let color_TypeSpecifier = "OliveDrab"
let color_ASTTypeof = "SeaGreen"
let color_Statement = "yellow"
let color_handler = "khaki1"
let color_FullExpression = "coral"
let color_Expression = "orange"
let color_ArgExpression = "DarkOrange"
let color_ArgExpressionListOpt = "tomato"
let color_FullExpressionAnnot = "tan1"
let color_Condition = "OrangeRed"
let color_OperatorName = "chocolate1"
let color_Initializer = "gold"
let color_ASTTypeID = "MediumAquamarine"
let color_Enumerator = "PaleTurquoise"
let color_CType = "green"
let color_ATomicType = "LimeGreen"
let color_Compound_info = "LawnGreen"
let color_Enum_Value = "OliveDrab1"
let color_BaseClassSpec = "PaleGreen"
let color_BaseClass = "SpringGreen"
let color_exceptionSpec = "salmon"
let color_TemplateDeclaration = "brown3"
let color_TemplateParameter = "tan3"
let color_TemplateArgument = "peru"
let color_STemplateArgument = "maroon"
let color_TemplateInfo = "burlywood3"
let color_InheritedTemplateParams = "PaleVioletRed4"
let color_NamespaceDecl = "grey50"
let color_Designator = "LemonChiffon"
let color_Attribute = "orchid1"
let color_Scope = "grey"

(***************** variable ***************************)

let rec variable_fun (v : annotated variable) =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {			
    poly_var = v.poly_var; loc = v.loc; var_name = v.var_name;
    var_type = v.var_type; flags = v.flags; value = v.value;
    defaultParam = v.defaultParam; funcDefn = v.funcDefn;
    overload = v.overload; virtuallyOverride = v.virtuallyOverride;
    scope = v.scope; templ_info = v.templ_info;
  }
  in
    if visited v.poly_var then retval v.poly_var
    else begin
      visit v.poly_var;
      ast_loc_node color_Variable v.poly_var v.loc "Variable"
	[("name", string_opt v.var_name);
	 ("flags", string_of_declFlags v.flags)
	]
	(let l1 = opt_child cType_fun "type" !(v.var_type) in
	 let l2 = opt_child expression_fun "varValue" !(v.value) in
	 let l3 = opt_child cType_fun "defaultParamType" v.defaultParam in
	 let l4 = opt_child func_fun "funcDefn" !(v.funcDefn) in 
	 let l5 = 
	   count_rev "overload"
	     (List.rev_map variable_fun !(v.overload)) in
	 let l6 =
	   count_rev "virtuallyOverride"
	     (List.rev_map variable_fun v.virtuallyOverride) in
	 let l7 = opt_child scope_fun "scope" v.scope in
	 let l8 = opt_child templ_info_fun "templInfo" v.templ_info
	 in
	   l1 @ l2 @ l3 @ l4 @ l5 @ l6 @ l7 @ l8)
    end

(***************** templateInfo ***********************)

and templ_info_fun ti =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_templ = ti.poly_templ; templ_kind = ti.templ_kind;
    template_params = ti.template_params;
    template_var = ti.template_var; inherited_params = ti.inherited_params; 
    instantiation_of = ti.instantiation_of; 
    instantiations = ti.instantiations; 
    specialization_of = ti.specialization_of; 
    specializations = ti.specializations; arguments = ti.arguments; 
    inst_loc = ti.inst_loc; 
    partial_instantiation_of = ti.partial_instantiation_of; 
    partial_instantiations = ti.partial_instantiations; 
    arguments_to_primary = ti.arguments_to_primary; 
    defn_scope = ti.defn_scope; 
    definition_template_info = ti.definition_template_info; 
    instantiate_body = ti.instantiate_body; 
    instantiation_disallowed = ti.instantiation_disallowed; 
    uninstantiated_default_args = ti.uninstantiated_default_args; 
    dependent_bases = ti.dependent_bases;
  }
  in
    if visited ti.poly_templ then retval ti.poly_templ
    else begin
      visit ti.poly_templ;
      ast_loc_node color_TemplateInfo ti.poly_templ ti.inst_loc "TemplateInfo"
	[("templKind", string_of_templateThingKind ti.templ_kind);
	 ("instantiateBody",  string_of_bool ti.instantiate_body);
	 ("instantiationDisallowed", 
	  string_of_bool ti.instantiation_disallowed);
	 ("uninstantiatedDefaultArgs", 
	  string_of_int ti.uninstantiated_default_args)
	]
	(let l1 = 
	   count_rev "params"
	     (List.rev_map variable_fun ti.template_params) in
	 let l2 = opt_child variable_fun "var" !(ti.template_var) in
	 let l3 = 
	   count_rev "inheritedParams"
	     (List.rev_map inherited_templ_params_fun ti.inherited_params) in
	 let l4 = 
	   opt_child variable_fun "instantiationOf" !(ti.instantiation_of) in
	 let l5 = 
	   count_rev "instantiations" 
	     (List.rev_map variable_fun ti.instantiations) in
	 let l6 = 
	   opt_child variable_fun "specializationOf" !(ti.specialization_of) in
	 let l7 = 
	   count_rev "specializations"
	     (List.rev_map variable_fun ti.specializations) in
	 let l8 =
	   count_rev "arguments"
	     (List.rev_map sTemplateArgument_fun ti.arguments) in
	 let l9 = 
	   opt_child variable_fun 
	     "partialInstantiationOf" !(ti.partial_instantiation_of) in
	 let l10 = 
	   count_rev "partialInstantiations"
	     (List.rev_map variable_fun ti.partial_instantiations) in
	 let l11 =
	   count_rev "argumentsToPrimary"
	     (List.rev_map sTemplateArgument_fun ti.arguments_to_primary) in
	 let l12 = opt_child scope_fun "defnScope" ti.defn_scope in
	 let l13 = 
	   opt_child templ_info_fun
	     "definitionTemplateInfo" ti.definition_template_info in
	 let l14 =
	   count_rev "dependentBases"
	     (List.rev_map cType_fun ti.dependent_bases)
	 in
	   l1 @ l2 @ l3 @ l4 @ l5 @ l6 @ l7 @ l8 
	   @ l9 @ l10 @ l11 @ l12 @ l13 @ l14)
    end


(************* inheritedTemplateParams ****************)

and inherited_templ_params_fun itp =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_inherited_templ = itp.poly_inherited_templ;
    inherited_template_params = itp.inherited_template_params;
    enclosing = itp.enclosing;
  }
  in
    if visited itp.poly_inherited_templ then retval itp.poly_inherited_templ
    else begin
      assert(!(itp.enclosing) <> None);
      visit itp.poly_inherited_templ;
      ast_node color_InheritedTemplateParams itp.poly_inherited_templ
	"InheritedTemplateParams"
	[]
	(let l1 = 
	   count_rev "params"
	     (List.rev_map variable_fun itp.inherited_template_params) in
	 let l2 = 
	   opt_child compound_info_fun "enclosing" !(itp.enclosing)
	 in
	   l1 @ l2)
    end


(***************** cType ******************************)

and baseClass_fun bc =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_base = bc.poly_base; compound = bc.compound;
    bc_access = bc.bc_access; is_virtual = bc.is_virtual
  }
  in
    if visited bc.poly_base then retval bc.poly_base
    else begin
      visit bc.poly_base;
      ast_node color_BaseClass bc.poly_base "BaseClass"
	[("access", string_of_accessKeyword bc.bc_access);
	 ("isVirtual", string_of_bool bc.is_virtual)]
	[(compound_info_fun bc.compound, "ct")]
    end

and compound_info_fun i = 
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    compound_info_poly = i.compound_info_poly;
    compound_name = i.compound_name; typedef_var = i.typedef_var;
    ci_access = i.ci_access; compound_scope = i.compound_scope;
    is_forward_decl = i.is_forward_decl;
    is_transparent_union = i.is_transparent_union; keyword = i.keyword;
    data_members = i.data_members; bases = i.bases;
    conversion_operators = i.conversion_operators;
    friends = i.friends; inst_name = i.inst_name; syntax = i.syntax;
    self_type = i.self_type;
  }
  in
    if visited i.compound_info_poly then retval i.compound_info_poly 
    else begin
      visit i.compound_info_poly;
      assert(match !(i.syntax) with
	       | None
	       | Some(TS_classSpec _) -> true
	       | _ -> false);
      ast_node color_Compound_info i.compound_info_poly "CompoundType" 
	[("name", string_opt i.compound_name);
	 ("access", string_of_accessKeyword i.ci_access);
	 ("forward", string_of_bool i.is_forward_decl);
	 ("transparantUnion", string_of_bool i.is_transparent_union);
	 ("keyword", string_of_compoundType_Keyword i.keyword);
	 ("instName", string_opt i.inst_name)
	]
	(let x1 = variable_fun i.typedef_var, "typedefVar" in
	 let x2 = scope_fun i.compound_scope, "inherited scope" in
	 let l3 = 
	   count_rev "dataMembers"
	     (List.rev_map variable_fun i.data_members) in
	 let l4 =
	   count_rev "virtualBases"
	     (List.rev_map baseClass_fun i.bases) in
	 let l5 = 
	   count_rev "conversionOperators"
	     (List.rev_map variable_fun i.conversion_operators) in
	 let l6 = opt_child typeSpecifier_fun "syntax" !(i.syntax) in
	 let l7 = 
	   count_rev "friends"
	     (List.rev_map variable_fun i.friends) in
	 let l8 = opt_child cType_fun "selfType" !(i.self_type)
	 in
	   x1 :: x2 :: l3 @ l4 @ l5 @ l6 @ l7 @ l8)
    end


and enum_value_fun (annot, string, nativeint) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_Enum_Value annot "Enum::Value"
      [("name", string);
       ("value", Nativeint.to_string nativeint)]
      []
  end


and atomicType_fun at = 
  let annot = atomicType_annotation at
  in
    if visited annot then retval annot
    else 
      let atnode = ast_node color_ATomicType annot 
      in match at with
	(*
	 * put calls to visit here before in each case, except for CompoundType
	 *)

	| SimpleType(annot, simpleTypeId) ->
	    visit annot;
	    atnode "SimpleType" 
	      [("type", string_of_simpleTypeId simpleTypeId)] []

	| CompoundType(compound_info) ->
	    compound_info_fun compound_info

	| PseudoInstantiation(annot, str, variable_opt, accessKeyword, 
			      compound_info, sTemplateArgument_list) ->
	    visit annot;
	    atnode "PseudoInstantiation" 
	      [("name", str);
	       ("access", string_of_accessKeyword accessKeyword)]
	      (let l1 = opt_child variable_fun "typedefVar" variable_opt in
	       let l2 = [compound_info_fun compound_info, "primary"] in
	       let l3 = 
		 count_rev "args" 
		   (List.rev_map 
		      sTemplateArgument_fun sTemplateArgument_list)
	       in
		 l1 @ l2 @ l3)

	| EnumType(annot, string, variable, accessKeyword, 
		   enum_value_list, has_negatives) ->
	    visit annot;
	    atnode "EnumType" 
	      [("name", string_opt string);
	       ("access", string_of_accessKeyword accessKeyword);
	       ("has_negatives", string_of_bool has_negatives)]
	      (let l1 = opt_child variable_fun "typedefVar" variable in
	       let l2 = 
		 count_rev "values"
		   (List.rev_map enum_value_fun enum_value_list)
	       in 
		 l1 @ l2)

	| TypeVariable(annot, string, variable, accessKeyword) ->
	    visit annot;
	    atnode "TypeVariable" 
	      [("name", string);
	       ("access", string_of_accessKeyword accessKeyword)]
	      [(variable_fun variable, "typedefVar")]

	| DependentQType(annot, string, variable, 
			 accessKeyword, atomic, pq_name) ->
	    visit annot;
	    atnode "DependentQType"
	      [("name", string);
	       ("access", string_of_accessKeyword accessKeyword)]
	      (let x1 = variable_fun variable, "typedefVar" in
	       let x2 = atomicType_fun atomic, "first" in
	       let x3 = pQName_fun pq_name, "rest"
	       in
		 [x1; x2; x3])





and cType_fun t = 
  let annot = cType_annotation t 
  in
    if visited annot then retval annot
    else 
      let _ = visit annot in
      let tnode = ast_node color_CType annot in
      let tnode_1d name field value childs = tnode name [(field, value)] childs
      in match t with
	| CVAtomicType(_annot, cVFlags, atomicType) ->
	    tnode_1d "CVAtomicType" "cv" (string_of_cVFlags cVFlags)
	      [(atomicType_fun atomicType, "atomic")]

	| PointerType(_annot, cVFlags, cType) ->
	    tnode_1d "PointerType" "cv" (string_of_cVFlags cVFlags)
	      [(cType_fun cType, "atType")]
      

	| ReferenceType(_annot, cType) ->
	    tnode "ReferenceType" [] [(cType_fun cType, "atType")]

	| FunctionType(_annot, function_flags, cType, 
		       variable_list, cType_list_opt) ->
	    tnode "FunctionType" 
	      [("flags", (string_of_function_flags function_flags));
	       ("exnSpec", 
		match cType_list_opt with
		  | None -> "not present"
		  | Some [] -> "present, empty"
		  | Some _ -> "present, nonempty")
	      ]
	      (let x1 = (cType_fun cType, "retType") in
	       let l2 = 
		 count_rev "params" (List.rev_map variable_fun variable_list) in
	       let l3 = match cType_list_opt with
		 | None -> []
		 | Some l -> count_rev "exnSpec" (List.rev_map cType_fun l)
	       in
		 x1 :: l2 @ l3)

	| ArrayType(_annot, cType, array_size) ->
	    tnode_1d "ArrayType" "size" (string_of_array_size array_size)
	      [(cType_fun cType, "eltType")]

	| DependentSizeArrayType(_annot, cType, size_expr) ->
	    tnode "DependentSizeArrayType"
	      []
	      (let x1 = (cType_fun cType, "eltType") in
	       let x2 = (expression_fun size_expr, "sizeExpr")
	       in
		 [x1; x2])

	| PointerToMemberType(_annot, atomicType (* = NamedAtomicType *), 
			      cVFlags, cType) ->
	    assert(match atomicType with 
		     | SimpleType _ -> false
		     | CompoundType _
		     | PseudoInstantiation _
		     | EnumType _
		     | TypeVariable _ 
		     | DependentQType _ -> true);
	    tnode_1d "PointerToMemberType" "cv" (string_of_cVFlags cVFlags)
	      (let x1 = (atomicType_fun atomicType, "inClassNat") in
	       let x2 = (cType_fun cType, "atType")
	       in
		 [x1; x2])


and sTemplateArgument_fun ta = 
  let annot = sTemplateArgument_annotation ta
  in
    if visited annot then retval annot
    else 
      let _ = visit annot in
      let tanode = ast_node color_STemplateArgument annot in
      let tanode_1c name field child = tanode name [] [(child, field)]
      in match ta with
	| STA_NONE _annot -> 
	    tanode "STA_NONE" [] []

	| STA_TYPE(_annot, cType) -> 
	    tanode "STA_TYPE" [] [(cType_fun cType, "sta_value.t")]

	| STA_INT(_annot, int) -> 
	    tanode "STA_INT" [("sta_value.i", string_of_int int)] []

	| STA_ENUMERATOR(_annot, variable) -> 
	    tanode_1c "STA_ENUMERATOR" "sta_value.v" (variable_fun variable)

	| STA_REFERENCE(_annot, variable) -> 
	    tanode_1c "STA_REFERENCE" "sta_value.v" (variable_fun variable)

	| STA_POINTER(_annot, variable) -> 
	    tanode_1c "STA_POINTER" "sta_value.v" (variable_fun variable)

	| STA_MEMBER(_annot, variable) ->
	    tanode_1c "STA_MEMBER" "sta_value.v" (variable_fun variable)

	| STA_DEPEXPR(_annot, expression) -> 
	    tanode_1c "STA_DEPEXPR" "sta_value.e" (expression_fun expression)

	| STA_TEMPLATE _annot -> 
	    tanode "STA_TEMPLATE" [] []

	| STA_ATOMIC(_annot, atomicType) -> 
	    tanode_1c "STA_ATOMIC" "sta_value.at" (atomicType_fun atomicType)


and scope_fun s =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_scope = s.poly_scope; variables = s.variables; 
    type_tags = s.type_tags; parent_scope = s.parent_scope;
    scope_kind = s.scope_kind; namespace_var = s.namespace_var;
    scope_template_params = s.scope_template_params; 
    parameterized_entity = s.parameterized_entity;
    scope_compound = s.scope_compound
  }
  in
    if visited s.poly_scope then retval s.poly_scope
    else begin
      visit s.poly_scope;
      ast_node color_Scope s.poly_scope "Scope"
	[("variable hash", string_of_int (Hashtbl.length s.variables));
	 ("type tags hash", string_of_int (Hashtbl.length s.type_tags));
	 ("scopeKind", string_of_scopeKind s.scope_kind)
	]
	(let l1 = string_hash_child variable_fun "variables" s.variables in
	 let l2 = string_hash_child variable_fun "typeTags" s.type_tags in
	 let l3 = opt_child scope_fun "parentScope" s.parent_scope in
	 let l4 = opt_child variable_fun "namespaceVar" !(s.namespace_var) in
	 let l5 = 
	   count_rev "templateParams"
	     (List.rev_map variable_fun s.scope_template_params) in
	 let l6 = opt_child variable_fun "parameterizedEntity" 
	   s.parameterized_entity in
	 let l7 = opt_child compound_info_fun "scope_compound" 
	   !(s.scope_compound)
	 in
	   l1 @ l2 @ l3 @ l4 @ l5 @ l6 @ l7)
    end


(***************** generated ast nodes ****************)

and compilationUnit_fun
    ((annot, name, tu) : annotated compilationUnit_type) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_CompilationUnit annot "CompilationUnit" 
      [("name", name);]
      [(translationUnit_fun tu, "unit")]
  end

and translationUnit_fun (annot, topForm_list, scope_opt) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_TranslationUnit annot "TranslationUnit" []
      (let l1 = count_rev "topForms" (List.rev_map topForm_fun topForm_list) in
       let l2 = opt_child scope_fun "scope" scope_opt
       in
	 l1 @ l2)
  end

and topForm_fun tf =
  let annot = topForm_annotation tf
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let tf_node = ast_loc_node color_TF annot (topForm_loc tf) in
      let tf_node_11 name label child = tf_node name [label] [child] 
      in match tf with
	| TF_decl(_annot, _loc, declaration) -> 
	    tf_node "TF_decl" [] [(declaration_fun declaration, "decl")]

	| TF_func(_annot, _loc, func) -> 
	    tf_node "TF_func" [] [(func_fun func, "f")]

	| TF_template(_annot, _loc, templateDeclaration) -> 
	    tf_node "TF_template" []
	      [(templateDeclaration_fun templateDeclaration, "td")]

	| TF_explicitInst(_annot, _loc, declFlags, declaration) -> 
	    tf_node_11 "TF_explicitInst" 
	      ("instFlags", string_of_declFlags declFlags)
	      (declaration_fun declaration, "d")

	| TF_linkage(_annot, _loc, linkage, translationUnit) -> 
	    tf_node_11 "TF_linkage" 
	      ("linkage", linkage)
	      (translationUnit_fun translationUnit, "forms")

	| TF_one_linkage(_annot, _loc, linkage, topForm) -> 
	    tf_node_11 "TF_one_linkage"
	      ("linkage", linkage)
	      (topForm_fun topForm, "form")

	| TF_asm(_annot, _loc, e_stringLit) -> 
	    assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	    tf_node "TF_asm" [] [(expression_fun e_stringLit, "text")]

	| TF_namespaceDefn(_annot, _loc, name_opt, topForm_list) -> 
	    tf_node "TF_namespaceDefn" 
	      [("name", string_opt name_opt)]
	      (count_rev "forms" (List.rev_map topForm_fun topForm_list))

	| TF_namespaceDecl(_annot, _loc, namespaceDecl) -> 
	    tf_node "TF_namespaceDecl" [] 
	      [(namespaceDecl_fun namespaceDecl, "decl")]



and func_fun(annot, declFlags, typeSpecifier, declarator, memberInit_list, 
	     s_compound_opt, handler_list, func, variable_opt_1, 
	     variable_opt_2, statement_opt, bool) =
  if visited annot then retval annot
  else begin
    visit annot;
    assert(match s_compound_opt with
	     | None -> true
	     | Some s_compound ->
		 match s_compound with 
		   | S_compound _ -> true 
		   | _ -> false);
    assert(match func with 
      | FunctionType _ -> true
      | _ -> false);
    ast_node color_Function annot "Function"
      [("dflags", string_of_declFlags declFlags);
       ("implicit def", string_of_bool bool)]
      (let x1 = typeSpecifier_fun typeSpecifier, "retspec" in
       let x2 = declarator_fun declarator, "nameAndParams" in
       let l4 = count_rev "inits" (List.rev_map memberInit_fun memberInit_list) 
       in
       let l5 = opt_child statement_fun "body" s_compound_opt in
       let l6 = count_rev "handlers" (List.rev_map handler_fun handler_list) in
       let x7 = cType_fun func, "funcType" in
       let l8 = opt_child variable_fun "receiver" variable_opt_1 in
       let l9 = opt_child variable_fun "retVar" variable_opt_2 in
       let l10= opt_child statement_fun "dtor" statement_opt 
       in
	 x1 :: x2 :: l4 @ l5 @ l6 @ x7 :: l8 @ l9 @ l9 @ l10)
  end


and memberInit_fun(annot, pQName, argExpression_list, 
		   variable_opt_1, compound_opt, variable_opt_2, 
		   full_expr_annot, statement_opt) =
  if visited annot then retval annot
  else begin
    (* it's either a member or base class init, therefore not both
     * of variable_opt_1 and compound_opt is Some _
     * Both can aparently be None if the member to initialize is a 
     * template parameter.
     *)
    assert(match (variable_opt_1, compound_opt) with
    	     | (Some _, Some _) -> false
    	     | _ -> true);
    assert(match compound_opt with
      | None
      | Some(CompoundType _) -> true
      | _ -> false);
    visit annot;
    ast_node color_MemberInit annot "MemberInit" []
      (let x1 = (pQName_fun pQName, "name") in
       let l2 = 
	 count_rev "args" (List.rev_map argExpression_fun argExpression_list) in
       let l3 = opt_child variable_fun "member" variable_opt_1 in
       let l4 = opt_child atomicType_fun "base" compound_opt in
       let l5 = opt_child variable_fun "ctorVar" variable_opt_2 in
       let x6 = fullExpressionAnnot_fun full_expr_annot, "annot" in
       let l7 = opt_child statement_fun "ctorStatement" statement_opt
       in
	 x1 :: l2 @ l3 @ l4 @ l5 @ x6 :: l7)
  end

and declaration_fun(annot, declFlags, typeSpecifier, declarator_list) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_Declaration annot "Declaration" 
      [("dflags", string_of_declFlags declFlags)]
      (let x1 = (typeSpecifier_fun typeSpecifier, "spec") in
       let l2 = 
	 count_rev "decllist" (List.rev_map declarator_fun declarator_list)
       in
	 x1 :: l2)
  end


and aSTTypeId_fun(annot, typeSpecifier, declarator) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_ASTTypeID annot "ASTTypeId" []
      (let x1 = (typeSpecifier_fun typeSpecifier, "spec") in
       let x2 = (declarator_fun declarator, "decl")
       in
	 [x1; x2])
  end


and pQName_fun pq = 
  let annot = pQName_annotation pq
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let pq_node = ast_loc_node color_PQName annot (pQName_loc pq)
      in match pq with
	| PQ_qualifier(_annot, _loc, stringRef_opt, 
		       templateArgument_opt, pQName, 
		      variable_opt, s_template_arg_list) -> 
	    pq_node "PQ_qualifier" 
	      [("qualifier", string_opt stringRef_opt)]
	      (let l1 = 
		 opt_child templateArgument_fun "templArgs" templateArgument_opt
	       in
	       let x2 = pQName_fun pQName, "rest" in
	       let l3 = opt_child variable_fun "qualifierVar" variable_opt in
	       let l4 = 
		 count_rev "sargs"
		   (List.rev_map sTemplateArgument_fun s_template_arg_list) 
	       in
		 l1 @ x2 :: l3 @ l4)

	| PQ_name(_annot, _loc, stringRef) -> 
	    pq_node "PQ_name" [("name", stringRef)] []

	| PQ_operator(_annot, _loc, operatorName, stringRef) -> 
	    pq_node "PQ_operator" 
	      [("fakeName", stringRef)]
	      [(operatorName_fun operatorName, "o")]

	| PQ_template(_annot, _loc, stringRef, templateArgument_opt, 
		     s_template_arg_list) -> 
	    pq_node "PQ_template" 
	      [("name", stringRef)]
	      (let l1 = opt_child
		 templateArgument_fun "templArgs" templateArgument_opt in
	       let l2 =
		 count_rev "sargs"
		   (List.rev_map sTemplateArgument_fun s_template_arg_list)
	       in
		 l1 @ l2)

	| PQ_variable(_annot, _loc, variable) -> 
	    pq_node "PQ_variable" [] [(variable_fun variable, "var")]



and typeSpecifier_fun ts =
  let annot = typeSpecifier_annotation ts
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let tsnode name labels childs = 
	ast_loc_node color_TypeSpecifier annot 
	  (typeSpecifier_loc ts) name
	  (("cv", string_of_cVFlags (typeSpecifier_cv ts)) :: labels)
	  childs
      in
      let tsnode_1d name field value childs = 
	tsnode name [(field, value)] childs
      in match ts with
	| TS_name(_annot, _loc, _cVFlags, pQName, bool, 
		 var_opt_1, var_opt_2) -> 
	    tsnode_1d "TS_name" "typenameUsed" (string_of_bool bool)
	      (let x1 = pQName_fun pQName, "name" in
	       let l2 = opt_child variable_fun "var" var_opt_1 in
	       let l3 = opt_child variable_fun "nondependentVar" var_opt_2
	       in
		 x1 :: l2 @ l3)

	| TS_simple(_annot, _loc, _cVFlags, simpleTypeId) -> 
	    tsnode_1d "TS_simple" "id" (string_of_simpleTypeId simpleTypeId) []

	| TS_elaborated(_annot, _loc, _cVFlags, typeIntr, 
		       pQName, namedAtomicType_opt) -> 
	    assert(match namedAtomicType_opt with
	      | Some(SimpleType _) -> false
	      | _ -> true);
	    tsnode_1d "TS_elaborated" "keyword" (string_of_typeIntr typeIntr)
	      (let x1 = pQName_fun pQName, "name" in
	       let l2 = opt_child atomicType_fun "atype" namedAtomicType_opt
	       in
		 x1 :: l2)

	| TS_classSpec(_annot, _loc, _cVFlags, typeIntr, pQName_opt, 
		       baseClassSpec_list, memberList, compoundType) -> 
	    assert(match compoundType with
	      | CompoundType _ -> true
	      | _ -> false);
	    tsnode_1d "TS_classSpec" "keyword" (string_of_typeIntr typeIntr)
	      (let l1 = opt_child pQName_fun "name" pQName_opt in
	       let l2 = 
		 count_rev "bases" 
		   (List.rev_map baseClassSpec_fun baseClassSpec_list) in
	       let l3 = [memberList_fun memberList, "members"] in
	       let x4 = atomicType_fun compoundType, "ctype"
	       in
		 l1 @ l2 @ l3 @ x4 :: [])

	| TS_enumSpec(_annot, _loc, _cVFlags, 
		      stringRef_opt, enumerator_list, enumType) -> 
	    assert(match enumType with 
	      | EnumType _ -> true
	      | _ -> false);
	    tsnode_1d "TS_enumSpec" "name" (string_opt stringRef_opt)
	      (let l1 = count_rev "elts"
		 (List.rev_map enumerator_fun enumerator_list) in
	       let x2 = atomicType_fun enumType, "etype"
	       in
		 l1 @ [x2])

	| TS_type(_annot, _loc, _cVFlags, cType) -> 
	    tsnode "TS_type" [] [(cType_fun cType, "type")]

	| TS_typeof(_annot, _loc, _cVFlags, aSTTypeof) -> 
	    tsnode "TS_typeof" [] [(aSTTypeof_fun aSTTypeof, "atype")]



(* and baseClassSpec_fun(_annot, bool, accessKeyword, pQName) = *)
and baseClassSpec_fun(annot, bool, accessKeyword, pQName, compoundType_opt) =
  if visited annot then retval annot
  else begin
    assert(match compoundType_opt with
	     | None 
    	     | Some(CompoundType _ ) -> true
    	     | _ -> false
    	  );
    visit annot;
    ast_node color_BaseClassSpec annot "BaseClassSpec"
      [("isVirtual", string_of_bool bool);
       ("access", string_of_accessKeyword accessKeyword)]
      (let x1 = (pQName_fun pQName, "name") in
       let l2 = opt_child atomicType_fun "type" compoundType_opt
       in
	 x1 :: l2)
  end


and enumerator_fun(annot, loc, stringRef, 
		   expression_opt, variable, int32) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_loc_node color_Enumerator annot loc "Enumerator" 
      [("name", stringRef);
       ("enumValue", Int32.to_string int32)]
      (let l1 = opt_child expression_fun "expr" expression_opt in
       let x2 = variable_fun variable, "var"
       in
	 l1 @ x2 :: [])
  end


and memberList_fun(annot, member_list) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_MemberList annot "MemberList" []
      (count_rev "list" (List.rev_map member_fun member_list))
  end

and member_fun m = 
  let annot = member_annotation m
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let mnode = ast_loc_node color_Member annot (member_loc m) in
      let mnode_1c name child field = mnode name [] [(child, field)] 
      in match m with
	| MR_decl(_annot, _loc, declaration) -> 
	    mnode_1c "MR_decl" (declaration_fun declaration) "d"

	| MR_func(_annot, _loc, func) -> 
	    mnode_1c "MR_func" (func_fun func) "f"

	| MR_access(_annot, _loc, accessKeyword) -> 
	    mnode "MR_access" [("k", string_of_accessKeyword accessKeyword)] []

	| MR_usingDecl(_annot, _loc, nd_usingDecl) -> 
	    assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
	    mnode_1c "MR_usingDecl" (namespaceDecl_fun nd_usingDecl) "decl"

	| MR_template(_annot, _loc, templateDeclaration) -> 
	    mnode_1c "MR_template" 
	      (templateDeclaration_fun templateDeclaration) "d"


and declarator_fun(annot, iDeclarator, init_opt, 
		   variable_opt, ctype_opt, declaratorContext,
		   statement_opt_ctor, statement_opt_dtor) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_Declarator annot "Declarator" 
      [("context", string_of_declaratorContext declaratorContext)]
      (let x1 = (iDeclarator_fun iDeclarator, "decl") in
       let l2 = opt_child init_fun "init" init_opt in
       let l3 = opt_child variable_fun "var" variable_opt in
       let l4 = opt_child cType_fun "type" ctype_opt in
       let l5 = opt_child statement_fun "ctor" statement_opt_ctor in
       let l6 = opt_child statement_fun "dtor" statement_opt_dtor
       in
	 x1 :: l2 @ l3 @ l4 @ l5 @ l6)
  end
    

and iDeclarator_fun idecl =
  let annot = iDeclarator_annotation idecl
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let inode = ast_loc_node color_IDeclarator annot (iDeclarator_loc idecl) 
      in
      let inode_1d name field value childs = inode name [(field, value)] childs
      in match idecl with
	| D_name(_annot, _loc, pQName_opt) -> 
	    inode "D_name" [] (opt_child pQName_fun "name" pQName_opt)

	| D_pointer(_annot, _loc, cVFlags, iDeclarator) -> 
	    inode_1d "D_pointer" "cv" (string_of_cVFlags cVFlags)
	      [(iDeclarator_fun iDeclarator, "base")]

	| D_reference(_annot, _loc, iDeclarator) -> 
	    inode "D_reference" [] [(iDeclarator_fun iDeclarator, "base")]

	| D_func(_annot, _loc, iDeclarator, aSTTypeId_list, cVFlags, 
		 exceptionSpec_opt, pq_name_list, bool) -> 
	    assert(List.for_all (function | PQ_name _ -> true | _ -> false) 
	    	     pq_name_list);
	    inode "D_func" 
	      [("cv", string_of_cVFlags cVFlags);
	       ("isMember", string_of_bool bool)]
	      (let x1 = (iDeclarator_fun iDeclarator, "base") in
	       let l2 = 
		 count_rev "params" 
		   (List.rev_map aSTTypeId_fun aSTTypeId_list) in
	       let l3 = 
		 opt_child exceptionSpec_fun "exnSpec" exceptionSpec_opt in
	       let l4 = 
		 count_rev "kAndR_params" 
		   (List.rev_map pQName_fun pq_name_list)
	       in
		 x1 :: l2 @ l3 @ l4)

	| D_array(_annot, _loc, iDeclarator, expression_opt, bool) -> 
	    inode_1d "D_array" "isNewSize" (string_of_bool bool)
	      (let x1 = (iDeclarator_fun iDeclarator, "base") in
	       let l2 = opt_child expression_fun "size" expression_opt
	       in
		 x1 :: l2)

	| D_bitfield(_annot, _loc, pQName_opt, expression, int) -> 
	    inode_1d "D_bitfield" "numBits" (string_of_int int)
	      (let l1 = opt_child pQName_fun "name" pQName_opt in
	       let l2 = [(expression_fun expression, "bits")]
	       in
		 l1 @ l2)

	| D_ptrToMember(_annot, _loc, pQName, cVFlags, iDeclarator) -> 
	    inode_1d "D_ptrToMember" "cv" (string_of_cVFlags cVFlags)
	      (let x1 = (pQName_fun pQName, "nestedName") in
	       let x2 = (iDeclarator_fun iDeclarator, "base")
	       in
		 [x1; x2])

	| D_grouping(_annot, _loc, iDeclarator) -> 
	    inode "D_grouping" [] [(iDeclarator_fun iDeclarator, "base")]

	| D_attribute(_annot, _loc, iDeclarator, attribute_list_list) ->
	    inode "D_attribute" [] 
	      ((iDeclarator_fun iDeclarator, "base") ::
		 (List.flatten
		    (List.map
		       (fun (al, fname) -> count_rev fname al)
		       (count_rev "alist" 
			  (List.rev_map (List.rev_map attribute_fun)
			     attribute_list_list)))))


and exceptionSpec_fun(annot, aSTTypeId_list) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_exceptionSpec annot "ExceptionSpec" []
      (count_rev "types" (List.rev_map aSTTypeId_fun aSTTypeId_list))
  end


and operatorName_fun on = 
  let annot = operatorName_annotation on
  in 
    if visited annot then retval annot
    else
      let _ = visit annot in
      let onode = ast_node color_OperatorName annot
      in match on with
	| ON_newDel(_annot, bool_is_new, bool_is_array) -> 
	    onode "ON_newDel" 
	      [("is_new", string_of_bool bool_is_new);
	       ("is_array", string_of_bool bool_is_array)]
	      []

	| ON_operator(_annot, overloadableOp) -> 
	    onode "ON_operator"
	      [("op", string_of_overloadableOp overloadableOp)]
	      []

	| ON_conversion(_annot, aSTTypeId) -> 
	    onode "ON_conversion" []
	      [(aSTTypeId_fun aSTTypeId, "type")]


and statement_fun s =
  let annot = statement_annotation s
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let snode = ast_loc_node color_Statement annot (statement_loc s) in
      let snode_1d name field value childs = snode name [(field, value)] childs
      in match s with
	| S_skip(_annot, _loc) -> 
	    snode "S_skip" [] []

	| S_label(_annot, _loc, stringRef, statement) -> 
	    snode_1d "S_label" "name" stringRef [(statement_fun statement, "s")]

	| S_case(_annot, _loc, expression, statement, int32) -> 
	    snode_1d "S_case" "labelVal" (Int32.to_string int32)
	      (let x1 = (expression_fun expression, "expr") in
	       let x2 = (statement_fun statement, "s")
	       in
		 [x1; x2])

	| S_default(_annot, _loc, statement) -> 
	    snode "S_default" [] [(statement_fun statement, "s")]

	| S_expr(_annot, _loc, fullExpression) -> 
	    snode "S_expr" [] [(fullExpression_fun fullExpression, "s")]

	| S_compound(_annot, _loc, statement_list) -> 
	    snode "S_compound" []
	      (count_rev "stmts" (List.rev_map statement_fun statement_list))

	| S_if(_annot, _loc, condition, statement_then, statement_else) -> 
	    snode "S_if" []
	      (let x1 = (condition_fun condition, "cond") in
	       let x2 = (statement_fun statement_then, "then") in
	       let x3 = (statement_fun statement_else, "else")
	       in
		 [x1; x2; x3])

	| S_switch(_annot, _loc, condition, statement) -> 
	    snode "S_switch" []
	      (let x1 = (condition_fun condition, "cond") in
	       let x2 = (statement_fun statement, "branches")
	       in
		 [x1; x2])

	| S_while(_annot, _loc, condition, statement) -> 
	    snode "S_while" []
	      (let x1 = (condition_fun condition, "cond") in
	       let x2 = (statement_fun statement, "body")
	       in
		 [x1; x2])

	| S_doWhile(_annot, _loc, statement, fullExpression) -> 
	    snode "S_doWhile" []
	      (let x1 = (statement_fun statement, "body") in
	       let x2 = (fullExpression_fun fullExpression, "expr")
	       in
		 [x1; x2])

	| S_for(_annot, _loc, statement_init, condition, 
		fullExpression, statement_body) -> 
	    snode "S_for" []
	      (let x1 = (statement_fun statement_init, "init") in
	       let x2 = (condition_fun condition, "cond") in
	       let x3 = (fullExpression_fun fullExpression, "after") in
	       let x4 = (statement_fun statement_body, "body")
	       in
		 [x1; x2; x3; x4])

	| S_break(_annot, _loc) -> 
	    snode "S_break" [] []

	| S_continue(_annot, _loc) ->
	    snode "S_continue" [] []

	| S_return(_annot, _loc, fullExpression_opt, statement_opt) -> 
	    snode "S_return" []
	      (let l1 = 
		 opt_child fullExpression_fun "expr" fullExpression_opt in
	       let l2 = opt_child statement_fun "copy_ctor" statement_opt
	       in 
		 l1 @ l2)

	| S_goto(_annot, _loc, stringRef) -> 
	    snode_1d "S_goto" "target" stringRef []

	| S_decl(_annot, _loc, declaration) -> 
	    snode "S_decl" [] [(declaration_fun declaration, "decl")]

	| S_try(_annot, _loc, statement, handler_list) -> 
	    snode "S_try" []
	      (let x1 = (statement_fun statement, "body") in
	       let l2 = 
		 count_rev "handler" (List.rev_map handler_fun handler_list) 
	       in
		 x1 :: l2)

	| S_asm(_annot, _loc, e_stringLit) -> 
	    assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
	    snode "S_asm" [] [(expression_fun e_stringLit, "text")]

	| S_namespaceDecl(_annot, _loc, namespaceDecl) -> 
	    snode "S_namespaceDecl" [] 
	      [(namespaceDecl_fun namespaceDecl, "decl")]

	| S_function(_annot, _loc, func) -> 
	    snode "S_function" []
	      [(func_fun func, "f")]

	| S_rangeCase(_annot, _loc, 
		      expression_lo, expression_hi, statement, 
		     label_lo, label_hi) -> 
	    snode "S_rangeCase" 
	      [("labelValLo", string_of_int label_lo);
	       ("labelValHi", string_of_int label_hi)]
	      (let x1 = (expression_fun expression_lo, "exprLo") in
	       let x2 = (expression_fun expression_hi, "exprHi") in
	       let x3 = (statement_fun statement, "s")
	       in
		 [x1; x2; x3])

	| S_computedGoto(_annot, _loc, expression) -> 
	    snode "S_computedGoto" [] [(expression_fun expression, "target")]


and condition_fun co = 
  let annot = condition_annotation co
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let conode = ast_node color_Condition annot
      in match co with
	| CN_expr(_annot, fullExpression) -> 
	    conode "CN_expr" [] [(fullExpression_fun fullExpression, "expr")]

	| CN_decl(_annot, aSTTypeId) -> 
	    conode "CN_decl" [] [(aSTTypeId_fun aSTTypeId, "typeId")]


and handler_fun(annot, aSTTypeId, statement_body, variable_opt, 
		fullExpressionAnnot, expression_opt, statement_gdtor) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_handler annot "Handler" []
      (let x1 = (aSTTypeId_fun aSTTypeId, "typeId") in
       let x2 = (statement_fun statement_body, "body") in
       let l3 = opt_child variable_fun "globalVar" variable_opt in
       let x4 = fullExpressionAnnot_fun fullExpressionAnnot, "annot" in
       let l5 = (opt_child expression_fun "localArg" expression_opt) in
       let l6 = (opt_child statement_fun "globalDtor" statement_gdtor)
       in
	 x1 :: x2 :: l3 @ x4 :: l5 @ l6)
  end


and expression_fun ex = 
  let annot = expression_annotation ex
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let type_child = opt_child cType_fun "type" (expression_type ex) in
      let exnode name attribs childs = 
	ast_node color_Expression annot name attribs (type_child @ childs) in
      let exnode_1d name field value childs = 
	                        exnode name [(field, value)] childs
      in match ex with
	| E_boolLit(_annot, _type_opt, bool) -> 
	    exnode_1d "E_boolLit" "b" (string_of_bool bool) []

	| E_intLit(_annot, _type_opt, stringRef, ulong) -> 
	    exnode "E_intLit" 
	      [("text", stringRef);
	       ("i", Int32.to_string ulong)]
	      []

	| E_floatLit(_annot, _type_opt, stringRef, double) -> 
	    exnode "E_floatLit" 
	      [("text", stringRef);
	       ("d", string_of_float double)]
	      []

	| E_stringLit(_annot, _type_opt, stringRef, 
		      e_stringLit_opt, stringRef_opt) -> 
	    assert(match e_stringLit_opt with 
		     | Some(E_stringLit _) -> true 
		     | None -> true
		     | _ -> false);
	    exnode "E_stringLit" 
	      [("text", stringRef);
	       ("fullTextNQ", string_opt stringRef_opt)]
	      (opt_child expression_fun "continuation" e_stringLit_opt)

	| E_charLit(_annot, _type_opt, stringRef, int32) -> 
	    exnode "E_charLit"
	      [("text", stringRef);
	       ("c", Int32.to_string int32)]
	      []

	| E_this(_annot, _type_opt, variable) -> 
	    exnode "E_this" [] 
	      [(variable_fun variable, "receiver")]

	| E_variable(_annot, _type_opt, pQName, var_opt, nondep_var_opt) -> 
	    exnode "E_variable" [] 
	      (let x1 = pQName_fun pQName, "name" in
	       let l2 = opt_child variable_fun "var" var_opt in
	       let l3 = opt_child variable_fun "nondependentVar" nondep_var_opt
	       in
		 x1 :: l2 @ l3)

	| E_funCall(_annot, _type_opt, expression_func, argExpression_list, 
		    expression_retobj_opt) -> 
	    exnode "E_funCall" []
	      (let x1 = (expression_fun expression_func, "func") in
	       let l2 = 
		 count_rev "args"
		   (List.rev_map argExpression_fun argExpression_list) in
	       let l3 = opt_child expression_fun "retObj" expression_retobj_opt
	       in
		 x1 :: l2 @ l3)

	| E_constructor(_annot, _type_opt, typeSpecifier, argExpression_list, 
			var_opt, bool, expression_opt) -> 
	    exnode_1d "E_constructor" "artificial" (string_of_bool bool)
	      (let x1 = (typeSpecifier_fun typeSpecifier, "spec") in
	       let l2 = 
		 count_rev "args" 
		   (List.rev_map argExpression_fun argExpression_list) in
	       let l3 = opt_child variable_fun "ctorVar" var_opt in
	       let l4 = opt_child expression_fun "retObj" expression_opt
	       in
		 x1 :: l2 @ l3 @ l4)

	| E_fieldAcc(_annot, _type_opt, expression, pQName, var_opt) -> 
	    exnode "E_fieldAcc" []
	      (let x1 = (expression_fun expression, "obj") in
	       let x2 = (pQName_fun pQName, "fieldName") in
	       let l3 = opt_child variable_fun "field" var_opt
	       in
		 x1 :: x2 :: l3)

	| E_sizeof(_annot, _type_opt, expression, int) -> 
	    exnode_1d "E_sizeof" "size" (string_of_int int)
	      [(expression_fun expression, "expr")]

	| E_unary(_annot, _type_opt, unaryOp, expression) -> 
	    exnode_1d "E_unary" "op" (string_of_unaryOp unaryOp)
	      [(expression_fun expression, "expr")]

	| E_effect(_annot, _type_opt, effectOp, expression) -> 
	    exnode_1d "E_effect" "op" (string_of_effectOp effectOp)
	      [(expression_fun expression, "expr")]

	| E_binary(_annot, _type_opt, 
		   expression_left, binaryOp, expression_right) -> 
	    exnode_1d "E_binary" "op" (string_of_binaryOp binaryOp) 
	      (let x1 = (expression_fun expression_left, "e1") in
	       let x2 = (expression_fun expression_right, "e2")
	       in
		 [x1; x2])

	| E_addrOf(_annot, _type_opt, expression) -> 
	    exnode "E_addrOf" [] [(expression_fun expression, "expr")]

	| E_deref(_annot, _type_opt, expression) -> 
	    exnode "E_deref" [] [(expression_fun expression, "prt")]

	| E_cast(_annot, _type_opt, aSTTypeId, expression, bool) -> 
	    exnode_1d "E_cast" "tcheckedType" (string_of_bool bool)
	      (let x1 = (expression_fun expression, "expr") in
	       let x2 = (aSTTypeId_fun aSTTypeId, "ctype")
	       in
		 [x1; x2])

	| E_cond(_annot, _type_opt, 
		 expression_cond, expression_true, expression_false) -> 
	    exnode "E_cond" []
	      (let x1 = (expression_fun expression_cond, "cond") in
	       let x2 = (expression_fun expression_true, "th") in
	       let x3 = (expression_fun expression_false, "el")
	       in
		 [x1; x2; x3])

	| E_sizeofType(_annot, _type_opt, aSTTypeId, int, bool) -> 
	    exnode "E_sizeofType" 
	      [("size", string_of_int int);
	       ("tchecked", string_of_bool bool)]
	      [(aSTTypeId_fun aSTTypeId, "atype")]

	| E_assign(_annot, _type_opt, expression_target, binaryOp, expression_src) -> 
	    exnode_1d "E_assign" "op" (string_of_binaryOp binaryOp)
	      (let x1 = (expression_fun expression_target, "target") in
	       let x2 = (expression_fun expression_src, "src")
	       in
		 [x1; x2])

	(* 
         * | E_new(_annot, type_opt, bool, argExpression_list, aSTTypeId, 
	 * 	argExpressionListOpt_opt, statement_opt) -> 
         *)
	| E_new(_annot, _type_opt, bool, argExpression_list, aSTTypeId, 
		argExpressionListOpt_opt, array_size_opt, ctor_opt,
	        statement_opt, heap_var_opt) -> 
	    exnode_1d "E_new" "colonColon" (string_of_bool bool)
	      (let l1 = 
		 count_rev "placementArgs"
		   (List.rev_map argExpression_fun argExpression_list) in
	       let x2 = (aSTTypeId_fun aSTTypeId, "atype") in
	       let l3 = 
		 opt_child argExpressionListOpt_fun 
		   "ctorArgs" argExpressionListOpt_opt in
	       let l4 = opt_child expression_fun "arraySize" array_size_opt in
	       let l5 = opt_child variable_fun "ctorVar" ctor_opt in
	       let l6 = opt_child statement_fun "ctorStatement" statement_opt in
	       let l7 = opt_child variable_fun "heapVar" heap_var_opt
	       in
		 l1 @ x2 :: l3 @ l4 @ l5 @ l6 @ l7)

	| E_delete(_annot, _type_opt, bool_colon, bool_array, 
		   expression_opt, statement_opt) ->
	    exnode "E_delete"
	      [("colonColon", string_of_bool bool_colon);
	       ("array", string_of_bool  bool_array)]
	      (let l1 = opt_child expression_fun "expr" expression_opt in
	       let l2 = opt_child statement_fun "dtorStatement" statement_opt
	       in
		 l1 @ l2)

	| E_throw(_annot, _type_opt, expression_opt, var_opt, statement_opt) -> 
	    exnode "E_throw" []
	      (let l1 = opt_child expression_fun "expr" expression_opt in
	       let l2 = opt_child variable_fun "globalVar" var_opt in
	       let l3 = opt_child 
		 statement_fun "globalCtorStatement" statement_opt 
	       in
		 l1 @ l2 @ l3)

	| E_keywordCast(_annot, _type_opt, 
			castKeyword, aSTTypeId, expression) -> 
	    exnode_1d "E_keywordCast" "key" (string_of_castKeyword castKeyword)
	      (let x1 = (aSTTypeId_fun aSTTypeId, "ctype") in
	       let x2 = (expression_fun expression, "expr")
	       in
		 [x1; x2])

	| E_typeidExpr(_annot, _type_opt, expression) -> 
	    exnode "E_typeidExpr" [] [(expression_fun expression, "expr")]

	| E_typeidType(_annot, _type_opt, aSTTypeId) -> 
	    exnode "E_typeidType" [] [(aSTTypeId_fun aSTTypeId, "ttype")]

	| E_grouping(_annot, _type_opt, expression) -> 
	    exnode "E_grouping" [] [(expression_fun expression, "expr")]

	| E_arrow(_annot, _type_opt, expression, pQName) -> 
	    exnode "E_arrow" []
	      (let x1 = (expression_fun expression, "obj") in
	       let x2 = (pQName_fun pQName, "fieldName")
	       in
		 [x1; x2])

	| E_statement(_annot, _type_opt, s_compound) -> 
	    assert(match s_compound with | S_compound _ -> true | _ -> false);
	    exnode "E_statement" [] 
	      [(statement_fun s_compound, "s")]

	| E_compoundLit(_annot, _type_opt, aSTTypeId, in_compound) -> 
	    assert(match in_compound with | IN_compound _ -> true | _ -> false);
	    exnode "E_compoundLit" []
	      (let x1 = (aSTTypeId_fun aSTTypeId, "stype") in
	       let x2 = (init_fun in_compound, "init")
	       in
		 [x1; x2])

	| E___builtin_constant_p(_annot, _type_opt, loc, expression) -> 
	    ast_loc_node color_Expression annot loc "E___builtin_constant_p" []
	      [(expression_fun expression, "expr")]

	| E___builtin_va_arg(_annot, _type_opt, loc, expression, aSTTypeId) -> 
	    ast_loc_node color_Expression annot loc "E___builtin_va_arg" []
	      (let x1 = (expression_fun expression, "expr") in
	       let x2 = (aSTTypeId_fun aSTTypeId, "atype")
	       in
		 [x1; x2])

	| E_alignofType(_annot, _type_opt, aSTTypeId, int) -> 
	    exnode_1d "E_alignofType" "alignment" (string_of_int int)
	      [(aSTTypeId_fun aSTTypeId, "atype")]

	| E_alignofExpr(_annot, _type_opt, expression, int) -> 
	    exnode_1d "E_alignofExpr" "alignment" (string_of_int int)
	      [(expression_fun expression, "expr")]

	| E_gnuCond(_annot, _type_opt, expression_cond, expression_false) -> 
	    exnode "E_gnuCond" []
	      (let x1 = (expression_fun expression_cond, "cond") in
	       let x2 = (expression_fun expression_false, "el")
	       in
		 [x1; x2])

	| E_addrOfLabel(_annot, _type_opt, stringRef) -> 
	    exnode_1d "E_addrOfLabel" "labelName" stringRef []


(* and fullExpression_fun(_annot, expression_opt) = *)
and fullExpression_fun(annot, expression_opt, fullExpressionAnnot) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_FullExpression annot "FullExpression" []
      (let l1 = opt_child expression_fun "expr" expression_opt in
       let x2 = fullExpressionAnnot_fun fullExpressionAnnot, "annot"
       in
	 l1 @ x2 :: [])
  end


and argExpression_fun(annot, expression) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_ArgExpression annot "ArgExpression" []
      [(expression_fun expression, "expr")]
  end


and argExpressionListOpt_fun(annot, argExpression_list) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_ArgExpressionListOpt annot "ArgExpressionListOpt" []
      (count_rev "list" (List.rev_map argExpression_fun argExpression_list))
  end


and init_fun i = 
  let annot = init_annotation i
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let inode = ast_loc_node color_Initializer annot (init_loc i) 
      in match i with
(* everything in here *)
	| IN_expr(_annot, _loc, fullExpressionAnnot, expression_opt) -> 
	    inode "IN_expr" [] 
	      (let l1 = opt_child expression_fun "e" expression_opt in
	       let x2 = fullExpressionAnnot_fun fullExpressionAnnot, "annot"
	       in
		 l1 @ [x2])

	| IN_compound(_annot, _loc, fullExpressionAnnot, init_list) -> 
	    inode "IN_compound" []
	      (let x1 = fullExpressionAnnot_fun fullExpressionAnnot, "annot" in
	       let l2 = count_rev "inits" (List.rev_map init_fun init_list)
	       in
		 x1 :: l2)

	| IN_ctor(_annot, _loc, fullExpressionAnnot, 
		 argExpression_list, var_opt, bool) -> 
	    inode "IN_ctor" 
	      [("was_IN_expr", string_of_bool bool)]
	      (let x1 = fullExpressionAnnot_fun fullExpressionAnnot, "annot" in
	       let l2 = count_rev "args" 
		 (List.rev_map argExpression_fun argExpression_list) in
	       let l3 = opt_child variable_fun "ctorVar" var_opt
	       in
		 x1 :: l2 @ l3)

	| IN_designated(_annot, _loc, fullExpressionAnnot, designator_list, init) -> 
	    inode "IN_designated" []
	      (let x1 = fullExpressionAnnot_fun fullExpressionAnnot, "annot" in
	       let l2 = 
		 count_rev "designator"
		   (List.rev_map designator_fun designator_list) in
	       let x3 = init_fun init, "init"
	       in
		 x1 :: l2 @ x3 :: [])


and templateDeclaration_fun td = 
  let annot = templateDeclaration_annotation td
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let tdnode name childs = 
	ast_node color_TemplateDeclaration annot name [] childs
      in match td with
	| TD_func(_annot, templateParameter_opt, func) -> 
	    tdnode "TD_func" 
	      (let l1 = opt_child
		 templateParameter_fun "params" templateParameter_opt in
	       let l2 = [(func_fun func, "f")]
	       in
		 l1 @ l2)

	| TD_decl(_annot, templateParameter_opt, declaration) -> 
	    tdnode "TD_decl" 
	      (let l1 = opt_child
		 templateParameter_fun "params" templateParameter_opt in
	       let l2 = [(declaration_fun declaration, "d")]
	       in
		 l1 @ l2)

	| TD_tmember(_annot, templateParameter_opt, templateDeclaration) -> 
	    tdnode "TD_tmember"
	      (let l1 = 
		 opt_child 
		   templateParameter_fun "params" templateParameter_opt in
	       let l2 = [(templateDeclaration_fun templateDeclaration, "d")]
	       in
		 l1 @ l2)


and templateParameter_fun tp = 
  let annot = templateParameter_annotation tp
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let tpnode name next labels childs =
	ast_loc_node color_TemplateParameter annot (templateParameter_loc tp)
	  name labels
	  (childs @ (opt_child templateParameter_fun "next" next))
      in match tp with
	| TP_type(_annot, _loc, variable, stringRef, 
		  aSTTypeId_opt, templateParameter_opt) -> 
	    tpnode "TP_type" templateParameter_opt 
	      [("name", stringRef)]
	      (let x1 = variable_fun variable, "var" in
	       let l2 = opt_child aSTTypeId_fun "defaultType" aSTTypeId_opt
	       in
		 x1 :: l2)

	| TP_nontype(_annot, _loc, variable,
		    aSTTypeId, templateParameter_opt) -> 
	    tpnode "TP_nontype" templateParameter_opt []
	      (let x1 = variable_fun variable, "var" in
	       let x2 = aSTTypeId_fun aSTTypeId, "param"
	       in
		 [x1; x2])



and templateArgument_fun ta = 
  let annot = templateArgument_annotation ta
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let tanode name next childs = 
	ast_node color_TemplateArgument annot name []
	  (childs @ (opt_child templateArgument_fun "next" next))
      in match ta with
	| TA_type(_annot, aSTTypeId, templateArgument_opt) -> 
	    tanode "TA_type" templateArgument_opt 
	      [(aSTTypeId_fun aSTTypeId, "type")]

	| TA_nontype(_annot, expression, templateArgument_opt) -> 
	    tanode "TA_nontype" templateArgument_opt
	      [(expression_fun expression, "expr")]

	| TA_templateUsed(_annot, templateArgument_opt) -> 
	    tanode "TA_templateUsed" templateArgument_opt []


and namespaceDecl_fun nd = 
  let annot = namespaceDecl_annotation nd
  in 
    if visited annot then retval annot
    else
      let _ = visit annot in
      let ndnode = ast_node color_NamespaceDecl annot
      in match nd with
	| ND_alias(_annot, stringRef, pQName) -> 
	    ndnode "ND_alias" 
	      [("alias", stringRef)]
	      [(pQName_fun pQName, "original")]

	| ND_usingDecl(_annot, pQName) -> 
	    ndnode "ND_usingDecl" []
	      [(pQName_fun pQName, "name")]

	| ND_usingDir(_annot, pQName) -> 
	    ndnode "ND_usingDir" []
	      [(pQName_fun pQName, "name")]

(* 
 * 31 and fullExpressionAnnot_fun(declaration_list) =
 *     List.iter declaration_fun declaration_list
 *)
and fullExpressionAnnot_fun(annot, declaration_list) =
  if visited annot then retval annot
  else begin
    visit annot;
    ast_node color_FullExpressionAnnot annot "FullExpressionAnnot" []
      (count_rev "declarations"
	 (List.rev_map declaration_fun declaration_list))
  end



and aSTTypeof_fun a = 
  let annot = aSTTypeof_annotation a
  in
    if visited annot then retval annot
    else
      let _ = visited annot in
      let anode = ast_node color_ASTTypeof annot
      in match a with
	| TS_typeof_expr(_annot, ctype, fullExpression) -> 
	    anode "TS_typeof_expr" [] 
	      (let x1 = cType_fun ctype, "type" in
	       let x2 = fullExpression_fun fullExpression, "expr" 
	       in
		 [x1; x2])

	| TS_typeof_type(_annot, ctype, aSTTypeId) -> 
	    anode "TS_typeof_type" []
	      (let x1 = cType_fun ctype, "type" in
	       let x2 = aSTTypeId_fun aSTTypeId, "atype"
	       in
		 [x1; x2])



and designator_fun d = 
  let annot = designator_annotation d
  in 
    if visited annot then retval annot
    else
      let _ = visit annot in
      let dnode = ast_loc_node color_Designator annot (designator_loc d)
      in match d with
	| FieldDesignator(_annot, _loc, stringRef) -> 
	    dnode "FieldDesignator" [("id", stringRef)] []

	(* | SubscriptDesignator(_annot, loc, expression, expression_opt) ->  *)
	| SubscriptDesignator(_annot, _loc, expression, expression_opt, 
			     idx_start, idx_end) -> 
	    dnode "SubscriptDesignator" 
	      [("idx_computed", string_of_int idx_start);
	       ("idx_computed2", string_of_int idx_end)]
	      (let x1 = (expression_fun expression, "idx_expr") in
	       let l2 = opt_child expression_fun "idx_expr2" expression_opt
	       in
		 x1 :: l2)


and attribute_fun a = 
  let annot = attribute_annotation a
  in
    if visited annot then retval annot
    else
      let _ = visit annot in
      let anode = ast_loc_node color_Attribute annot (attribute_loc a)
      in match a with
	| AT_empty(_annot, _loc) -> 
	    anode "AT_empty" [] []

	| AT_word(_annot, _loc, stringRef) -> 
	    anode "AT_word" [("w", stringRef)] []

	| AT_func(_annot, _loc, stringRef, argExpression_list) -> 
	    anode "AT_func" 
	      [("f", stringRef)]
	      (count_rev "args" 
		 (List.rev_map argExpression_fun argExpression_list))




(**************************************************************************
 *
 * end of astiter.ml 
 *
 **************************************************************************)

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

let out_file = ref ""

let size_flag = ref false

let dot_page_attribute = ref ""

let node_selections = ref []

let normal_top_level_scope = ref false

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

let arguments = Arg.align
  [
    ("-all", Arg.Unit (fun () -> select All_nodes),
     " select all nodes");
    ("-real", Arg.Unit (fun () -> select Real_nodes),
     " select all nodes apart from builtin functions in the top level scope");
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
    ("-o", Arg.Set_string out_file,
     "file set output file name [default nodes.dot]");
    ("-size", Arg.Set size_flag,
     " limit size of output");
    ("-page", Arg.Set_string dot_page_attribute,
     "pattr add pattr as page attribute in the output");
  ]

let usage_msg = 
  "usage: ast_graph [options...] <file>\n\
   recognized options are:"

let usage () =
  prerr_endline usage_msg;
  exit(1)
  
let file = ref ""

let file_set = ref false

let anonfun fn = 
  if !file_set 
  then
    begin
      Printf.eprintf "don't know what to do with %s\n" fn;
      usage()
    end
  else
    begin
      file := fn;
      file_set := true
    end


let print_this_node i = !print_node.(i) <- true



let mark_noloc_iter flag i node = 
  let loc_opt = node_loc node
  in
    match loc_opt with
      | Some (file, line, char) ->
	  if (file = "<noloc>" or file = "<init>")
	    && line = 1 && char = 1 
	  then
	    !print_node.(i) <- flag
      | None -> ()


let mark_noloc_nodes ast_array flag =
  Superast.iteri (mark_noloc_iter flag) ast_array

let mark_all_nodes flag =
  for i = 0 to Array.length !print_node -1 do
    !print_node.(i) <- flag
  done



let mark_direction ast_array visited dir_fun nodes = 
  let top_scope_opt = match ast_array.(1) with
    | CompilationUnit_type(_, _, (_,_, Some(scope))) -> 
	Some(id_annotation scope.poly_scope)
    | CompilationUnit_type(_, _, _) -> None
    | _ -> 
	prerr_endline "Couldn't find top level node at index 1";
	assert false
  in
  let rec doit = function
    | [] -> ()
    | (_, []) :: ll -> doit ll
    | (dia, i::l) :: ll ->
	if dia > visited.(i) then begin
	  print_this_node i;
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

let mark_real_nodes ast_array down =
  let visited = Array.create (Array.length !print_node) (-1) 
  in
    mark_direction ast_array visited (fun i -> down.(i)) [0, [1]]


let mark_node_diameter ast_array up down diameter =
  let rec get_current_selection i res = 
    if i = Array.length !print_node 
    then res
    else
      get_current_selection (i+1) (if !print_node.(i) then i::res else res)
  in
  let visited () = Array.create (Array.length !print_node) 0 in    
  let start_selection = get_current_selection 1 [] 
  in
    mark_direction ast_array (visited ())
      (fun i -> up.(i)) [(diameter +1, start_selection)];
    mark_direction ast_array (visited ())
      (fun i -> down.(i)) [(diameter +1, start_selection)]


let match_file_line file line loc_opt =
  match loc_opt with
    | Some(lfile, lline, _) -> lfile = file && lline = line
    | None -> false

let match_line line loc_opt =
  match loc_opt with
    | Some(_, lline, _) -> lline = line
    | None -> false

let mark_line ast_array file line char =
  let match_fun =
    match file,line,char with
      | (Some f, Some l, None) -> match_file_line f l
      | (None,   Some l, None) -> match_line l
      | _ -> assert false
  in
  let doit i node =
    if match_fun (node_loc node) then
      print_this_node i
  in
    Superast.iteri doit ast_array
  
  


let mark_nodes ast_array up down = function
  | All_nodes -> mark_all_nodes true
  | Real_nodes -> mark_real_nodes ast_array down
  | No_nodes -> mark_all_nodes false
  | Add_node i -> !print_node.(i) <- true
  | Del_node i -> !print_node.(i) <- false
  | Add_diameter i -> mark_node_diameter ast_array up down i
  | Del_noloc -> mark_noloc_nodes ast_array false
  | Add_noloc -> mark_noloc_nodes ast_array true
  | Normal_top_scope -> normal_top_level_scope := true
  | Special_top_scope -> normal_top_level_scope := false
  | Loc(file,line,char) -> mark_line ast_array file line char



let start_file infile =
  output_string !oc "digraph ";
  Printf.fprintf !oc "\"%s\"" infile;
  output_string !oc " {\n";
  if !size_flag then
    output_string !oc "    size=\"90,90\";\n";
  if !dot_page_attribute <> "" then
    Printf.fprintf !oc "    page=\"%s\";\n" !dot_page_attribute;
  output_string !oc 
    "    color=white;\n";
  output_string !oc
    "    ordering=out;\n";
  output_string !oc
    "    node [ style = filled ];\n";
  output_string !oc
    "    edge [ arrowtail=odot ];\n"


let finish_file () =
  output_string !oc "}\n"


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


let main () =
  Arg.parse arguments anonfun usage_msg;
  if not !file_set then
    usage();				(* does not return *)
  let ofile = 
    if !out_file <> "" 
    then !out_file
    else "nodes.dot"
  in
  let _ = oc := open_out (ofile) in
  let (max_node, ast) = Oast_header.unmarshal_oast !file in
  let ast_array = Superast.into_array max_node ast in
  let (up, down) = Uplinks.create ast_array in
  let print_node_array = Array.create (max_node +1) false in
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
      | Normal_top_scope :: _
      | Special_top_scope :: _
	-> assert false
  in
    print_node := print_node_array;
    List.iter (mark_nodes ast_array up down) sels;
    start_file !file;
    ignore(compilationUnit_fun ast);
    finish_file ();
    Printf.printf "graph with %d nodes and %d edges generated\n%!"
      !nodes_counter !edge_counter
      
;;


Printexc.catch main ()


