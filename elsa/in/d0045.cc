// minimized from nsAtomTable.i
typedef unsigned int PRUint32;

template <class InputIterator>
struct nsCharSourceTraits {
  PRUint32 f( InputIterator* s ) {
    return PRUint32(3);
  }
};

//  template <class CharT>
//  struct nsCharSourceTraits<CharT*> {
//    PRUint32 f( CharT* s ) {
//      return PRUint32(3);
//    }
//  };
