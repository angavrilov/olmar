// destructors of temporaries

void f1(void);
void f2(void);

class A_Temp {
public:
  ~A_Temp() { f1(); }
};

class B_Temp {
public:
  ~B_Temp() { f2(); }
};

A_Temp h(int);
B_Temp g(A_Temp);

void t() {
  B_Temp x = g(h(1));
}
