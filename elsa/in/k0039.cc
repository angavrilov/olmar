// funky namespace resolution after 'using namespace'

// originally found in package dcmtk_3.5.3-1

// Assertion failed: (!!v) == set.isNotEmpty(), file cc_scope.cc line 773

namespace NS {
}
using namespace NS;

typedef int fooint;
namespace NS {
    using ::fooint;
}
