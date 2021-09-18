void strfn(char *a, char *b, int x)
{
  char c[] = "qwerty";
  char d[] = "xyz";


  strcat(c, d);

  strcpy(a, b);
  strcat(a, b);
}
void main(int argc, char *argv[]) {
  char str[20] = "hello";
  char str1[20] = "world";


  strcat(str, str1);
  strfn(str, str1, 1);

}
