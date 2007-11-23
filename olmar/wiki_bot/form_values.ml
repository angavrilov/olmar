(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)


type form_input_type =
  | Form_input_text
  | Form_input_password
  | Form_input_checkbox
  | Form_input_radio
  | Form_input_submit
  | Form_input_reset
  | Form_input_file
  | Form_input_hidden
  | Form_input_image
  | Form_input_button

exception Invalid_form_input_type of string

let get_form_input_type t =
  match String.lowercase t with
  | "text" -> Form_input_text
  | "password" -> Form_input_password
  | "checkbox" -> Form_input_checkbox
  | "radio" -> Form_input_radio
  | "submit" -> Form_input_submit
  | "reset" -> Form_input_reset
  | "file" -> Form_input_file
  | "hidden" -> Form_input_hidden
  | "image" -> Form_input_image
  | "button" -> Form_input_button
  | _ -> raise (Invalid_form_input_type t)


let string_of_form_input_type = function
  | Form_input_text -> "text"
  | Form_input_password -> "password"
  | Form_input_checkbox -> "checkbox"
  | Form_input_radio -> "radio"
  | Form_input_submit -> "submit"
  | Form_input_reset -> "reset"
  | Form_input_file -> "file"
  | Form_input_hidden -> "hidden"
  | Form_input_image -> "image"
  | Form_input_button -> "button"



type form_input_element =
    (* Form_input(name, type, value) *)
  | Form_input of string * form_input_type * string
    (* Textarea(name, value) *)
  | Form_textarea of string * string


let name_and_val_of_form_input = function
  | Form_input(name, _, value)
  | Form_textarea(name, value) -> (name, value)


let rec find_input name = function
  | [] -> raise Not_found
  | ((Form_input(iname,_,_) | Form_textarea(iname,_)) as el) :: _
      when iname = name -> el
  | _ :: rest -> find_input name rest


let make_html_regex_builder format =
  let hash = Hashtbl.create 31
  in
    fun s ->
      try
	Hashtbl.find hash s
      with
	| Not_found ->
	    let re =
	      Pcre.regexp ~flags: [`CASELESS] (Printf.sprintf format s)
	    in
	      Hashtbl.add hash s re;
	      re


let tag_open_re = make_html_regex_builder "<%s [^>]*>"

let tag_close_re = make_html_regex_builder "</%s>"

let attribute_re = make_html_regex_builder "%s=([\"'])([^\"']*)\\1"


let get_attribute attr buf =
  let re = attribute_re attr
  in
    try
      Pcre.get_substring (Pcre.exec ~rex:re buf) 2
    with
      | Not_found as ex ->
	  Printf.eprintf "attribute %s not found\n%!" attr;
	  raise ex


let html_decode = Netencoding.Html.decode_to_latin1


let get form_name buf =
  let form_start_re =
    Pcre.regexp ~flags: [`CASELESS]
      (Printf.sprintf "<form [^>]*name=\"%s\"[^>]*>" form_name)
  in
  let form_open_substrings = Pcre.exec ~rex:form_start_re buf in
  let (form_start, _) = Pcre.get_substring_ofs form_open_substrings 0 in
  let (_, form_end) =
    Pcre.get_substring_ofs
      (Pcre.exec ~rex:(tag_close_re "form")  ~pos:form_start buf)
      0
  in
  let action =
    html_decode
      (get_attribute "action" (Pcre.get_substring form_open_substrings 0))
  in
  let form = String.sub buf form_start (form_end - form_start) in
  let input_re = tag_open_re "input" in
  let textarea_re = tag_open_re "textarea" in
  let inputs = ref [] in

  let input_substrings =
    try
      Pcre.exec_all ~rex:input_re form
    with
      | Not_found -> [||]
  in
  let textarea_substrings =
    try
      Pcre.exec_all ~rex:textarea_re form
    with
      | Not_found -> [||]
  in
    Array.iter
      (fun substrings ->
	 let input_tag = Pcre.get_substring substrings 0 in
	   inputs :=
	     Form_input(
	       html_decode(get_attribute "name" input_tag),
	       get_form_input_type (get_attribute "type" input_tag),
	       html_decode(get_attribute "value" input_tag))
	   :: !inputs)
      input_substrings;
    Array.iter
      (fun substrings ->
	 let textarea_tag = Pcre.get_substring substrings 0 in
	 let text_start = snd(Pcre.get_substring_ofs substrings 0) in
	 let text_end =
	   fst(Pcre.get_substring_ofs
		 (Pcre.exec ~rex:(tag_close_re "textarea") ~pos:text_start form)
		 0)
	 in
	 let text_val = 
	   html_decode(String.sub form text_start (text_end - text_start))
	 in
	   inputs :=
	     Form_textarea(
	       get_attribute "name" textarea_tag,
	       text_val)
	   :: ! inputs)
      textarea_substrings;
    (action, !inputs)



(*** Local Variables: ***)
(*** compile-command: "ocamlfind opt -c -package pcre form_values.ml" ***)
(*** End: ***)
