(* objpool.ml *)
(* pool of allocated objects for explicit re-use *)
                                  
(* This object pool maintains a set of objects that are available
 * for use.  It must be given a way to create new objects. *)
class ['a] tObjectPool (allocFunc: unit -> 'a) =
let dummy:'a = (allocFunc ()) in   (* interesting that this works.. *)
object (self)
  (* ---- data ---- *)
  (* dummy element for unused array slots *)
  val null: 'a = dummy;

  (* number of elements in the pool *)
  val mutable poolLength: int = 0;

  (* array of elements in the pool; the array's length may be greater
   * than 'poolLength', to accomodate adding more elements to the
   * pool without resizing the array *)
  val mutable pool: 'a array = (Array.make 16 dummy);

  (* ---- funcs ---- *)
  (* retrieve an object ready to be used; might return a pool element,
   * or if the pool is empty, will make a new element *)
  method alloc() : 'a =
  begin
    if (poolLength > 0) then (
      (* just grab the last element in the pool *)
      poolLength <- poolLength - 1;
      pool.(poolLength)
    )
    
    else (
      (* make a new object; I thought about making several at a time
       * but there seems little advantage.. *)
      (allocFunc ())
    )
  end

  (* return an object to the pool so it can be re-used *)
  method dealloc (obj: 'a) : unit =
  begin
    if (poolLength = (Array.length pool)) then (
      (* need to expand the pool array *)
      let newPool : 'a array = (Array.make (poolLength * 2) null) in

      (* copy *)
      (Array.blit
        pool                  (* source array *)
        0                     (* source start position *)
        newPool               (* dest array *)
        0                     (* dest start position *)
        poolLength            (* number of elements to copy *)
      );

      (* switch from old to new *)
      pool <- newPool;
    );

    (* put new element into the pool at the end *)
    pool.(poolLength) <- obj;
    poolLength <- poolLength + 1;
  end
end


(* EOF *)
