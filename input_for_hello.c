int bar(char *a, char *b) {
  strcpy(a, b);
}

int fez(char *a, char *b, char *c) {
  bar(a, b);
  strcat(a, c);
}

void strfn(char *a, char *b, int x)
{
  char c = 'y';
  strcat(a, b);
}

void main(int argc, char *argv[]) {
  char a[] = "abc";
  char str[20] = "hello";
  char str1[20] = "world";

  fez(a, str, str1);
  bar(a, str);
  strfn(str, str1, 1);
}
