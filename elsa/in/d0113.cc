// CoderInfo-ootm.ii:17:3: error: no viable candidate for function call

// yes, gcc 2.96 will compile this.  I don't understand how the
// function tr::a() can be called when it has never been declared.  I
// don't understand how 'pos' can be used when it has never been
// declared

template<class C> struct S {};

template<bool t, int i> struct D {} ;

template<class C, class tr = S<C> >
struct B {
  struct Rep {};
  template<class I> B &f (I j1);
};

template<class C, class tr>
  template<class I> B<C, tr> &B<C, tr>::f(I j1)
{
  Rep *p;
  tr::a((*p)[pos], *j1);
  // if I replace this with the below elsa seems to also like it but
  // what the heck is going on?
//    tr::a();
}
