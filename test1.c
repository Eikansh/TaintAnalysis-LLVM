void strfn(char *a, char *b, int x)
{
  char *c ;
  char *d ;
  strcpy(c, d);
}
void main(int argc, char *argv[]) {
  char str[20] = "hello";
  char str1[20] = "world";
  str[0] = 'H';

  strcat(str, str1);
  strfn(str, str1, 1);

}
