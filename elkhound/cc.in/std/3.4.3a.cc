// section 3.4.3 example #1

class A {
public: 
  static int n; 
}; 
int main() 
{ 
  int A; 
  A::n = 42;          // OK    
  //ERROR1: A b;                // ill-formed: A does not name a type
}

