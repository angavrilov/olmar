// t0284.cc
// out-of-line class definition

namespace std
{
  class locale
  {
    class facet;
    class id;
    class _Impl;
  };
  class locale::_Impl
  {
    void _M_install_facet (const locale::id *, facet *);
  };
}
