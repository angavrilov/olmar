struct FILE;
char $_1 * fgets(char $_1* s, int size, FILE *stream);
int main() {
  char line[512] ;
//    char *line;
  FILE *f ;
  char $tainted *tmp___0 ;
  tmp___0 = fgets(line, (int )sizeof(line), f);
}
