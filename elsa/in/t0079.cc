// cc.in79
// problem with templatized forward decl?
// no, was a problem with templatized prototypes

class istream;

template<class TP> class smanip;
//template<class TP> class smanip {};

template<class TP>
inline istream& operator>>(istream& i, const smanip<TP>& m);
//int foo(smanip<TP> &m);
