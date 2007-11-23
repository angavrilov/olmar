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

type form_input_element =
    (* Form_input(name, type, value) *)
  | Form_input of string * form_input_type * string
    (* Textarea(name, value) *)
  | Form_textarea of string * string



val string_of_form_input_type : form_input_type -> string

val name_and_val_of_form_input : form_input_element -> (string * string)

val find_input : string -> form_input_element list -> form_input_element


exception Invalid_form_input_type of string


(* get_form_values form_name html_text
 * Searches for a form with the given name in html_text.
 * Raise Not_found if not found.
 * Otherwise return the action url of the form together with all 
 * input elements of the form
 *)
val get : string -> string -> string * form_input_element list
