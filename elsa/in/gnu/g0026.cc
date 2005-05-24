// g0026.cc
// gcc-2 header bug from bastring.cc

template <class charT, class traits,
          class Allocator>
class basic_string
{
private:
  struct Rep {
    inline static Rep* create (unsigned);
  };
};

template <class charT, class traits, class Allocator>
inline /* should say "typename" here */ 
basic_string <charT, traits, Allocator>::Rep *
basic_string <charT, traits, Allocator>::Rep::
create (unsigned extra)
{
  // ...
}
