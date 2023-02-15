#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h> //might not need? someone said for ssize_t

void prompt_function(); //function to print PS1 parameter
char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub);

int main(int argc, char *argv[]){
  char *line = NULL;
  size_t n = 0;
  for(;;) {
    prompt_function();
    ssize_t line_length = getline(&line, &n, stdin);

    if(line_length == -1) {
      exit(1);
    } else {
      printf("it works?");
      exit(0);
    }
  }
}

void prompt_function(){
  char *ps_parameter = getenv("PS1");
  if(ps_parameter == NULL) {
    fprintf(stderr, "");
  } else {
    fprintf(stderr, "%s", ps_parameter);
  }
}

char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub){
  char *str = *haystack;
  size_t haystack_len = strlen(str);
  size_t const needle_len = strlen(needle),
               sub_len = strlen(sub);

  for (; (str = strstr(str, needle));) {
    ptrdiff_t off = str - *haystack;
    if (sub_len > needle_len) {
      str = realloc(*haystack, sizeof **haystack * (haystack_len * sub_len - needle_len + 1));
      if (!str) goto exit;
      *haystack = str;
      str = *haystack + off;
    }
    memmove(str + sub_len, str + needle_len, haystack_len + 1 - off - needle_len);
    memcpy(str, sub, sub_len);
    haystack_len = haystack_len + sub_len - needle_len;
    str += sub_len;
  }
  str = *haystack;
  if (sub_len < needle_len) {
    str = realloc(*haystack, sizeof **haystack * (haystack_len + 1));
    if (!str) goto exit;
    *haystack = str;
  }

exit:
  return str;
}
