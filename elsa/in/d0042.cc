// minimized from nsAtomTable.i
//  in/d0042.cc:44:14: error: `PRUint32' used as a variable, but it's actually a typedef
//  in/d0042.cc:44:7: error: `nsCharTraits' is a template, but template arguments were not supplied
//  in/d0042.cc:44:36: error: `CharT' used as a variable, but it's actually a typedef
typedef unsigned int PRUint32;
typedef unsigned short PRUint16;
typedef PRUint16 PRUnichar;
typedef signed int size_t;

template <class CharT> struct nsCharTraits {};

template <> 
  struct nsCharTraits<PRUnichar> {
    typedef PRUnichar char_type;
    static
      size_t
      length( const char_type* s ) {
      //          size_t result = 0;
      //          while ( !eq(*s++, char_type(0)) )
      //            ++result;
      //          return result;
    }
  };

template <> 
  struct nsCharTraits<char> {
    typedef char           char_type;
    static
      size_t
      length( const char_type* s ) {
      //          return strlen(s);
    }
  };

template <class InputIterator>
  struct nsCharSourceTraits {};

template <class CharT>
  struct nsCharSourceTraits<CharT*> {
    static
      PRUint32
      readable_distance( CharT* s )
    {
      return PRUint32(nsCharTraits<CharT>::length(s));
 
    }
  };
