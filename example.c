#include <stdio.h>
#include <string.h>

#include "alloc_stack.h"

void print_stack(void);

/*
  example
*/

char *xor42(const char *str)
{
  AS_INIT();

  int slen = strlen(str);

  char *xored = as_malloc(slen + 1);
  if(!xored)
    return NULL;
  xored[slen] = '\0';

  for(int i = 0; i < slen; i++)
    xored[i] = str[i] ^ 42;

  // move pointer up the stack
  printf("STACK at xor42 before `alloc_up`\n");
  print_stack();
  as_up(xored);
  printf("STACK at xor42 after `alloc_up`\n");
  print_stack();
  return xored;
}

char *concat_xor42(char *a, char *b)
{
  // use vector here
  AS_INITV(4);

  int len_a = strlen(a);
  int len_b = strlen(b);
  int len_c = len_a + len_b;

  char *concat = as_malloc(len_a + len_b + 1);
  if(!concat)
    return NULL;

  memcpy(concat, a, len_a);
  memcpy(concat + len_a, b, len_b);
  concat[len_c] = '\0';

  printf("STACK at concat_xor42 before `xor42()`\n");
  print_stack();
  char *xored = xor42(concat);
  printf("STACK at concat_xor42 before `alloc_up`\n");
  print_stack();
  as_up(xored);
  printf("STACK at concat_xor42 after `alloc_up`\n");
  print_stack();


  return xored;
}

int main(int argc, char**argv)
{
  AS_INIT();
  if(argc != 3)
    return 1;  //printf(...)

  printf("STACK at main before call\n");
  print_stack();

  printf("xored42: %s\n", concat_xor42(argv[1], argv[2]));

  printf("STACK at main after call\n");
  print_stack();


  return 0;
}
