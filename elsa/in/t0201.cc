// t0201.cc
// specialize one parameter completely, leave behind one template parameter

// primary: two parameters
template <class S, class T>
struct C
{};

// specialization: one (the first!) parameter
template <class T>
struct C<int, T>
{};

// make use of the primary
C<float,float> *p;

// and also use the specialization
C<int,float> *q;
