template < class charT > struct basic_string {
  basic_string (const charT * s) {}
};

typedef basic_string <char> string;

struct Geometry {
  Geometry (std::string geometry_);
  void f();
};

void Geometry::f() {
  const char *geometry;
  *this = std::string (geometry);
}
