// t0289.cc
// repeated 'using' things

typedef int Int;

namespace N
{
  using ::Int;
  using ::Int;
}
