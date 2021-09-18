
void strfn(char *a, char *b, int x)
{
  char c[] = "qwerty";
  char d[] = "xyz";
  int len, len1;
  int p, q;

  strcat(c, d);

  strcpy(a, b);
  len = strlen(p);
  len1 = strlen(q);
  
  strcat(p, q);
}
void main(int argc, char *argv[]) {
  char str[20] = "hello";
  char str1[20] = "world";


  strcat(str, str1);
  strfn(str, str1, 1);

}
