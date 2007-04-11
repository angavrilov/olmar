
include Oast_header_version

let header = 
  Printf.sprintf "Marshaled Olmar C++ abstract syntax tree, version %d"
    oast_header_version

let output_header oc max =
  output_string oc header;
  output_string oc "\n";
  Marshal.to_channel oc (max : int) []

let read_header ic =
  let line = input_line ic
  in
    if line <> header then raise (Failure "oast_header.read_header");
    (Marshal.from_channel ic : int)
