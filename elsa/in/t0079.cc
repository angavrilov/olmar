// cc.in79
// problem with templatized forward decl?

class istream;

template<class TP> class smanip;  

template<class TP>
inline istream& operator>>(istream& i, const smanip<TP>& m);
