//  /home/ballAruns/tmpfiles/./Guppi-0.40.3-13/gnan-1Njx.i:3823:64:
//  error: there is no member called `__d' in union /*anonymous*/

int main() {
  (
   (union {int c;}) {c:0}
  ) .c ;
}
