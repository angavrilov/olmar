// CoderInfo-ootm.ii:9:14: error: cannot find scope name `std'
// CoderInfo-ootm.ii:9:14: error: there is no type called `std::string'

template<class T> class basic_string {};

typedef basic_string<char> string;

class CoderInfo {
  CoderInfo (std::string &name);
};
