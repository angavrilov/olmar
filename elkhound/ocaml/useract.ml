(* useract.ml *)
(* interface for user-defined reduction (etc.) actions *)
(* based on elkhound/useract.h *)

(* for now, some actual user actions *)


(* secret to type casting in OCaml: the Obj module *)
type tSemanticValue = Obj.t


(* ---------------- reduction actions -------------- *)
(* this is how ocamlyacc does it, so I assume it's fastest way *)
let actionArray : (tSemanticValue array -> tSemanticValue) array = [|
  (fun svals ->
    let top = (Obj.obj svals.(0) : int) in 
    (Obj.repr (
      top
    ))
  );
  (fun svals ->
    let e1 = (Obj.obj svals.(0) : int) in
    let e2 = (Obj.obj svals.(2) : int) in
    (Obj.repr (
      e1 + e2
    ))
  );
  (fun svals ->
    let e1 = (Obj.obj svals.(0) : int) in
    let e2 = (Obj.obj svals.(2) : int) in
    (Obj.repr (
      e1 - e2
    ))
  );
  (fun svals ->
    let e1 = (Obj.obj svals.(0) : int) in
    let e2 = (Obj.obj svals.(2) : int) in
    (Obj.repr (
      e1 * e2
    ))
  );
  (fun svals ->
    let e1 = (Obj.obj svals.(0) : int) in
    let e2 = (Obj.obj svals.(2) : int) in
    (Obj.repr (
      e1 / e2
    ))
  );
  (fun svals ->
    let n = (Obj.obj svals.(0) : int) in
    (Obj.repr (
      n
    ))
  );
  (fun svals ->
    let p = (Obj.obj svals.(0) : int) in
    (Obj.repr (
      p
    ))
  );
  (fun svals ->
    let e = (Obj.obj svals.(1) : int) in
    (Obj.repr (
      e
    ))
  )
|]

let reductionAction (productionId:int) (svals:tSemanticValue array)
  : tSemanticValue =
begin
  (actionArray.(productionId) svals)
end


(* EOF *)
