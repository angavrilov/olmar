// t0185.cc
// explicit specialization of template class constructor
// cppstd 14.7.3 para 2

typedef unsigned int size_t;

template <class T>
struct ctype_byname {
public:
  ctype_byname(const char *s, size_t refs);
};

template <>
ctype_byname<char>::ctype_byname(const char*, size_t refs);
