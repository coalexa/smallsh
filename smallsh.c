#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h> //might not need? someone said for ssize_t

void prompt_function(); //function for PS1 prompt

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
