// t0525.cc
// was causing error: cannot convert `<dependent> &' to `bool'
// sorry for the mess, feeling lazy right now


typedef unsigned int size_t;
namespace std
{
  template < typename _Alloc > class allocator;
  template < class _CharT > struct char_traits;
    template < typename _CharT, typename _Traits =
    char_traits < _CharT >, typename _Alloc =
    allocator < _CharT > >class basic_string;
  typedef basic_string < char >string;
    template < class _T1, class _T2 > struct pair
  {
  };
}
namespace __gnu_cxx
{
  template < typename _Tp > class __mt_alloc
  {
  };
}

namespace std
{
  template < typename _Tp > class allocator:public __gnu_cxx::__mt_alloc <
    _Tp >
  {
  public:typedef size_t size_type;
      template < typename _Tp1 > struct rebind
    {
      typedef allocator < _Tp1 > other;
    };
  };
  template < class _Arg, class _Result > struct unary_function
  {
  };
  template < class _Arg1, class _Arg2, class _Result > struct binary_function
  {
  };
  template < class _Tp > struct less:public binary_function < _Tp, _Tp, bool >
  {
  };
  template < class _Pair > struct _Select1st:public unary_function < _Pair,
    typename _Pair::first_type >
  {
  };
}

namespace __gnu_internal
{
  typedef char __one;
    template < typename _Tp > __one __test_type (int _Tp::*);
}
namespace std
{
  template < typename _Tp > struct __is_pod
  {
    enum
    {
      _M_type =
	(sizeof (__gnu_internal::__test_type < _Tp > (0)) !=
	 sizeof (__gnu_internal::__one))
    };
  };
  struct _Rb_tree_node_base
  {
  };
    template < typename _Val > struct _Rb_tree_node:public _Rb_tree_node_base
  {
  };
    template < typename _Key, typename _Val, typename _KeyOfValue,
    typename _Compare, typename _Alloc = allocator < _Val > >class _Rb_tree
  {
    typedef typename _Alloc::template rebind < _Rb_tree_node < _Val >
      >::other _Node_allocator;
  protected:  template < typename _Key_compare, bool _Is_pod_comparator =
      std::__is_pod < _Key_compare >::_M_type >
      struct _Rb_tree_impl:public _Node_allocator
    {
    };
      _Rb_tree_impl < _Compare > _M_impl;
  };
  template < typename _Key, typename _Tp, typename _Compare =
    less < _Key >, typename _Alloc =
    allocator < pair < const _Key, _Tp > > >class map
  {
  public:typedef _Key key_type;
    typedef pair < const _Key, _Tp > value_type;
    typedef _Compare key_compare;
  private:typedef _Rb_tree < key_type, value_type, _Select1st < value_type >,
      key_compare, _Alloc > _Rep_type;
    _Rep_type _M_t;
  };
}

class Parameter
{
  struct param_t
  {
  };
protected:  std::map < std::string, param_t > mapParam;
};
