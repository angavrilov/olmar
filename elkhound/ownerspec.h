// ownerspec.h
// specification of "owner pointer", as a C++ template class

template <class T>
class OwnerPtr {
private:
  T *ptr;
  
  enum State { OP_NULL, OP_DEAD, OP_OWNING };
  State state;
  
public:
  OwnerPtr() : ptr(NULL), state(OP_NULL) {}

  OwnerPtr(T *src) : ptr(src), state(src? OP_OWNING : OP_NULL) {}

  OwnerPtr(OwnerPtr &src) {
    ptr = src.ptr;
    state = src.state;
    src.state = OP_DEAD;
  }

  ~OwnerPtr() {
    assert(state != OP_OWNING);
  }

  OwnerPtr& operator= (OwnerPtr &src) {
    if (this != &src) {
      assert(state != OP_OWNING);
      ptr = src.ptr;
      state = src.state;
      src.state = OP_DEAD;
    }
    return *this;
  }

  OwnerPtr& operator= (T *src) {
    assert(state != OP_OWNING);
    ptr = src;
    state = src? OP_OWNING : OP_NULL;
    return *this;
  }

  bool operator== (T *p) {
    assert(state != OP_DEAD);
    return ptr == p;
  }

  // yield serf for possible further use
  operator T* () {
    assert(state != OP_DEAD);
    return ptr;
  }

  // use directly
  T& operator* () {
    assert(state == OP_OWNING);
    return *ptr;
  }
  T* operator-> () {
    assert(state == OP_OWNING);
    return ptr;
  }
};






