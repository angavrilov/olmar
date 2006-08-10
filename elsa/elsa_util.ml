
let string_of_flag_list string_of_elem = function
  | [] -> "[]"
  | hd::tl ->
      let buf = Buffer.create 20
      in
	Printf.bprintf buf "[%s" (string_of_elem hd);
	List.iter
	  (fun flag -> Printf.bprintf buf ", %s" (string_of_elem flag))
	  tl;
	Buffer.add_char buf ']';
	Buffer.contents buf
