// 13.3.3.2b.cc

int f(const int &);
int f(int &);
int g(const int &);
int g(int);

int i;
int j = f(i);                      // Calls f(int &)
int k = g(i);                      // ambiguous

class X {
public:
    void f() const;
    
    // TODO: working on this
    //void f();
};
void g(const X& a, X b)
{
    a.f();                         // Calls X::f() const
    b.f();                         // Calls X::f()
}
