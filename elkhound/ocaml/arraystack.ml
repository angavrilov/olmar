(* arraystack.ml *)
(* stack of pointers implemented as an array *)

(* the stack must be given a dummy value for unused array slots *)
class ['a] tArrayStack (null: 'a) =
object (self)
  (* ---- data ---- *)
  (* number of (non-null) elements in the array *)
  val mutable len: int = 0;

  (* the array; its length may be greater than 'poolLength', to 
   * accomodate adding more elements without resizing the array *)
  val mutable arr: 'a array = (Array.make 16 null);

  (* ---- funcs ---- *)
  method length() : int = len
  
  method isEmpty() : bool = len=0
  method isNotEmpty() : bool = len>0
  
  (* get topmost element but don't change what is stored *)
  method top() : 'a =
  begin
    arr.(len-1)
  end

  (* get topmost and remove it *)
  method pop() : 'a =
  begin
    len <- len - 1;
    arr.(len)
  end
  
  (* add a new topmost element *)
  method push (obj: 'a) : unit =
  begin
    if (len = (Array.length arr)) then (
      (* need to expand the array *)
      let newArr : 'a array = (Array.make (len * 2) null) in

      (* copy *)
      (Array.blit
        arr                   (* source array *)
        0                     (* source start position *)
        newArr                (* dest array *)
        0                     (* dest start position *)
        len                   (* number of elements to copy *)
      );

      (* switch from old to new *)
      arr <- newArr;
    );

    (* put new element into the array at the end *)
    arr.(len) <- obj;
    len <- len + 1;
  end
end


(* EOF *)
