// minimized from nsAtomTable.i
// just a modified version of d0042.cc
typedef unsigned int PRUint32;

template <class InputIterator>
struct nsCharSourceTraits {};

template <class CharT>
struct nsCharSourceTraits<CharT*> {
  PRUint32 f( CharT* s ) {
    return PRUint32(3);
  }
};
