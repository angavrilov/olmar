// t0114.cc
// duplicate defns in templates..

class Foo {
      void addr(int &r) const;
      void addr(int const &r) const;
};

template <class T>
class nsCppSharedAllocator
{
    public:
      typedef T*         pointer;
      typedef const T*   const_pointer;

      typedef T&         reference;
      typedef const T&   const_reference;


      pointer
      address( reference r ) const;

      const_pointer
      address( const_reference r ) const;
      
      void addr(int &r) const;
      void addr(int const &r) const;
};

typedef nsCppSharedAllocator<unsigned short> blah;
