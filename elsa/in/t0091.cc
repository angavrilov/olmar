// t0091.cc
// problem with explicit dtor call

class morkNode {
  ~morkNode();
  void foo();
};

void morkNode::foo()
{
  this->~morkNode();
  this->morkNode::~morkNode();
  this->morkNode::foo();
}



class forkNode {
  void foo();
};

void forkNode::foo()
{
  this->~forkNode();
  this->forkNode::~forkNode();
}
