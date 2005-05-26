// t0488.cc
// difficult type matching scenario

// Elsa, gcc and icc all get this wrong because they process the
// arguments in order, and cannot bind a type to a DQT.  But if
// they were to get lucky and process the second argument first,
// T would be bound and hence T::INT could be resolved (however,
// Elsa currently does not do DQT resolution during matching, so
// it would still fail; see t0487.cc).

// The standard does not mandate an order in which arguments are
// to be processed, nor does it prevent matching with DQTs, so
// this is apparently valid code.  However, it is hard to imagine
// how any translator could accept it without going to herculean
// efforts during matching.

template <class T>
void f(typename T::INT, T*, typename T::INT, int);

struct C {
  typedef int INT;
};

void g(C *c, int i)
{
  f(c, 1, i, 1);
}
