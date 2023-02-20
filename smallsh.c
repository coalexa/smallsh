#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h> //might not need? someone said for ssize_t

void prompt_function(); //function to print PS1 parameter
char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub);

char *tilda = NULL; //pointer for ~/
char *dd = NULL;   //pointer for $$
char *dq = NULL;   //pointer for $?
char *de = NULL;   //pointer for $!

int main(int argc, char *argv[]){
  char *line = NULL;
  size_t n = 0;
  char **split_arr = NULL;
  for (;;) {
    prompt_function();
    split_arr = malloc(sizeof *split_arr);
    ssize_t line_length = getline(&line, &n, stdin);

    char *IFS = getenv("IFS");
    if (IFS == NULL) IFS = " \t\n";

    char *token = NULL;
    char *dup_token = NULL;
    size_t count = 0;
    for (size_t i = 0; i >= 0; i++) {
      if (i == 0) {
        token = strtok(line, IFS);
      } else {
        token = strtok(NULL, IFS);
      }
      if (token == NULL) break;
      dup_token = strdup(token);

      count++;
      split_arr = realloc(split_arr, sizeof *split_arr * count + 1);
      split_arr[i] = dup_token;
    }
    dd = malloc(8);
    sprintf(dd, "%d", getpid());
    printf("%s \n", dd);

    for (size_t i = 0; i < count; i++) {
      printf("%s \n", split_arr[i]);
    }

    free(dd);
    //things declared in loops go in/out of scope on each iteration, we should free/realloc on each loop
    for (int i = 0; i < count; i++) {
      free(split_arr[i]);
    }
    free(split_arr);
    //maybe free this outside of main for loop
    free(line);

    exit(0);
  }
}

void prompt_function(){
  char *ps_parameter = getenv("PS1");
  if (ps_parameter == NULL) {
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
