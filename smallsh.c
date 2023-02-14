#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char *argv[]){
  char *line = NULL;
  size_t n = 0;
  for(;;) {
    ssize_t line_length = getline(&line, &n, stdin);

    if(line_length == -1) {
      exit(1);
    } else {
      printf("it works?");
      exit(0);
    }
  }
}
