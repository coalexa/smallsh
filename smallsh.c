#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

// TODO: possibly move getenv functions outside of loops into their own function?

void prompt_function(); //function to print PS1 parameter
int split_str(char **split_arr, char *line);
char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub);
void exec_nbc(char **split_arr); //probably change name here, add more parameters for files/signals

char *smallsh_pid = NULL;   //pointer for $$
char *fg_exit = NULL;   //pointer for $?
char *bg_pid = NULL;   //pointer for $!

int main(){
  char *line = NULL;
  size_t n = 0;
  char **split_arr = NULL;

  // convert PID to string for $$ expansion
  smallsh_pid = malloc(8);
  sprintf(smallsh_pid, "%d", getpid());

  for (;;) {
    prompt_function();
    split_arr = malloc(sizeof *split_arr);
    getline(&line, &n, stdin);

    size_t count = split_str(split_arr, line);

    // EXPANSION START
    for (size_t i = 0; i < count; i++) {
      if (!strncmp(split_arr[i], "~/", 2)) {
        char *home_param = getenv("HOME");
        if (home_param == NULL) home_param = "";
        split_arr[i] = str_gsub(&split_arr[i], "~", home_param);
      }
      split_arr[i] = str_gsub(&split_arr[i], "$$", smallsh_pid);
      //$? expansion
      //$! expansion
      printf("%s \n", split_arr[i]);
    }
    // EXPANSION END

    // {TODO: Add exit built-in command}
    // BUILT-INS START
    if (!split_arr[0]) continue; //will likely need to change continue to a jump to free stuff
    
    if (strcmp(split_arr[0], "cd") == 0) {
      if (split_arr[2]) {
        fprintf(stderr, "Too many arguments \n");
        continue; //CHANGE THIS
      } if (split_arr[1]) {
        if (chdir(split_arr[1]) == -1) {
          fprintf(stderr, "Invalid directory \n");
          continue; //CHANGE THIS
        } else {
          chdir(split_arr[1]);
          continue; //This can get removed after removing exit at the bottom of for loop
        }
      } else {
        chdir(getenv("HOME"));
        continue; //This can get removed after removing exit at the bottom of for loop
      }
    }
    // BUILT-INS END

    exec_nbc(split_arr);

    free(smallsh_pid);   //can likely be moved outside the for loop
    // probably keep these 3 things to be freed inside main loop
    for (int i = 0; i < count; i++) {
      free(split_arr[i]);
    }
    free(split_arr);
    free(line);

    exit(0);
  }
}

// function for PS1 prompt
void prompt_function(){
  char *ps_param = getenv("PS1");
  if (ps_param == NULL) {
    fprintf(stderr, "");
  } else {
    fprintf(stderr, "%s", ps_param);
  }
}

// function for splitting line into separate words
int split_str(char **split_arr, char *line) {
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
    return count;
}

// function for expansion
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

    if (strcmp(needle, "~") == 0) break;
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

void exec_nbc(char **split_arr) {
   int childStatus;

   pid_t spawnPid = fork();

   switch(spawnPid){
     case -1:
       perror("fork()\n");
       exit(1);
       break;
     case 0:
       // child process
       fprintf(stderr, "CHILD(%d) running %s command\n", getpid(), split_arr[0]);
       execvp(split_arr[0], split_arr);
       // exec only returns if there is an error
       perror("execvp");
       exit(2);
       break;
     default:
       // in parent process
       // wait for child's termination
       spawnPid = waitpid(spawnPid, &childStatus, 0);
       fprintf(stderr, "PARENT(%d): child(%d) terminated. Exiting\n", getpid(), spawnPid);
       break;
   }
}
