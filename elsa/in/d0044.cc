// minimized from nsAtomTable.i
typedef unsigned int PRUint32;

template <class InputIterator>
struct nsCharSourceTraits {};

template <class CharT>
struct nsCharSourceTraits<CharT*> {
  PRUint32 f() {
    return PRUint32(3);
  }
};
