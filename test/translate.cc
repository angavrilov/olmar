#include <strutil.h>

int main(){
  char ascii[256];
  char underscore[256];

  for(int i=0; i<=254; i++){
    ascii[i] = i+1;
    underscore[i] = '_';
  }
  ascii[255] = 0;
  underscore[255] = 0;

  cerr << "Hallo" << endl
       << ascii << endl
       << "Hallo2" << endl
       << translate(ascii, "\001-\057\072-\101\133-\140\173-\377", underscore)
       << endl;
}


/*** Local Variables: ***/
/*** compile-command: "make -C ../smbase; g++ -o translate translate.cc -g -Wall -Wno-deprecated -I../smbase ../smbase/libsmbase.a; ./translate" ***/
/*** End: ***/
 
