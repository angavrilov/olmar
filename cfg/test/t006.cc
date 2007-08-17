
class A {
public:
  A(int i) {};
  A() {};
};

int size();

void g(){
  A * a = new A(20);
  A * b = new A[size()];
  delete a;
  delete[] b;
}
