// cc.in37
// publishing superclass members

class Super {
public:
  int x;
};

class Sub : private Super {
public:    
  // publish 'x' despite private inheritance
  Super::x;                                 
  
  //ERROR1: y;     // missing superclass
};
