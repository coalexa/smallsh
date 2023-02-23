#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>

// TODO: possibly move getenv functions outside of loops into their own function?

void prompt_function(); //function to print PS1 parameter
//size_t split_str(char **split_arr, char *line);
char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub);
//void parse_arr(char ***split_arr, size_t count, char **outfile, char **infile, int *background);
void exec_cmd(char **split_arr); //probably change name here, add more parameters for files/signals

char *smallsh_pid = NULL;   //pointer for $$
char *fg_exit = "0";   //pointer for $?
char *bg_pid = "";   //pointer for $!

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

    if (feof(stdin)) {
      fprintf(stderr, "\nexit\n");
      exit(0);
    }
    // have to clear error otherwise 

    // SPLITTING START
    char *token = NULL;
    char *dup_token = NULL;
    size_t count = 0;

    for (size_t i = 0; i >= 0; i++) {
      char *IFS = getenv("IFS");
      if (IFS == NULL) IFS = " \t\n";

      if (i == 0) {
        token = strtok(line, IFS);
      } else {
        token = strtok(NULL, IFS);
      }
      if (token == NULL) {
        split_arr[i] = NULL;
        break;
      }
      dup_token = strdup(token);

      count++;
      split_arr = realloc(split_arr, sizeof *split_arr * count + 1);
      split_arr[i] = dup_token;
    }
    // SPLITTING END
    //size_t count = split_str(&split_arr, line);
    //printf("%s", split_arr[0]);

    // EXPANSION START
    for (size_t i = 0; i < count; i++) {
      // ~ expansion
      if (strncmp(split_arr[i], "~/", 2) == 0) {
        char *home_param = getenv("HOME");
        if (home_param == NULL) home_param = "";
        split_arr[i] = str_gsub(&split_arr[i], "~", home_param);
      }
      // $$ expansion
      split_arr[i] = str_gsub(&split_arr[i], "$$", smallsh_pid);
      //$? expansion
      split_arr[i] = str_gsub(&split_arr[i], "$?", fg_exit);
      //$! expansion
      split_arr[i] = str_gsub(&split_arr[i], "$!", bg_pid);
      //printf("%s \n", split_arr[i]);
    }
    // EXPANSION END

    // PARSING START
    int background = 0;
    char *outfile = NULL;
    char *infile = NULL;
    if (count < 3) goto exit;  // change to jump to exit instead of continue

    // case for when there are comments
    for(size_t i = 0; i < count; i++) {
      if (strcmp(split_arr[i], "#") == 0) {
        free(split_arr[i]);
        split_arr[i] = NULL;
        
        if ((i - 1) > 0 && (strcmp(split_arr[i - 1], "&") == 0)) {
          background = 1;
          free(split_arr[i - 1]);
          split_arr[i - 1] = NULL;
        
          // case when there's a # and & is last word
          if ((i - 3) > 0) {
            if (strcmp(split_arr[i - 3], "<") == 0) {
              infile = split_arr[i - 2];
              outfile = split_arr[i - 4];
              free(split_arr[i - 3]);
              split_arr[i - 3] = NULL;

              if ((i - 5) > 0 && (strcmp(split_arr[i - 5], ">") == 0)) {
                free(split_arr[i - 5]);
                split_arr[i - 5] = NULL;
              }

            } else if (strcmp(split_arr[i - 3], ">") == 0) {
              outfile = split_arr[i - 2];
              infile = split_arr[i - 4];
              free(split_arr[i - 3]);
              split_arr[i - 3] = NULL;

              if ((i - 5) > 0 && (strcmp(split_arr[i - 5], "<") == 0)) {
                free(split_arr[i - 5]);
                split_arr[i - 5] = NULL;
              }
            }
          }
        }
        // case when there's a # and & is not the last word
        if ((i - 2) > 0 && (strcmp(split_arr[i - 2], "<") == 0)) {
          infile = split_arr[i - 1];
          outfile = split_arr[i - 3];
          free(split_arr[i - 2]);
          split_arr[i - 2] = NULL;

          if ((i - 4) > 0 && (strcmp(split_arr[i - 4], ">") == 0)) {
            free(split_arr[i - 4]);
            split_arr[i - 4] = NULL;
          }
        } else if ((i - 2) > 0 && (strcmp(split_arr[i - 2], ">") == 0)) {
          outfile = split_arr[i - 1];
          infile = split_arr[i - 3];
          free(split_arr[i - 2]);
          split_arr[i - 2] = NULL;

          if ((i - 4) > 0 && (strcmp(split_arr[i - 4], "<") == 0)) {
            free(split_arr[i - 4]);
            split_arr[i - 4] = NULL;
          }
        }
      }
    }
    // case when & is the last word and no # is present
    if (strcmp(split_arr[count - 1], "&") == 0) {
      background = 1;
      free(split_arr[count - 1]);
      split_arr[count - 1] = NULL;

      if ((count - 3) > 0) {
        if (strcmp(split_arr[count - 3], "<") == 0) {
          infile = split_arr[count - 2];
          outfile = split_arr[count - 4];
          free(split_arr[count - 3]);
          split_arr[count - 3] = NULL;

          if ((count - 5) > 0 && (strcmp(split_arr[count - 5], ">") == 0)) {
            free(split_arr[count - 5]);
            split_arr[count - 5] = NULL;
          }

        } else if (strcmp(split_arr[count - 3], ">") == 0) {
          outfile = split_arr[count - 2];
          infile = split_arr[count - 4];
          free(split_arr[count - 3]);
          split_arr[count - 3] = NULL;

          if ((count - 5) > 0 && (strcmp(split_arr[count - 5], "<") == 0)) {
            free(split_arr[count - 5]);
            split_arr[count - 5] = NULL;
          }
        }
      }
    }

    // case when there is no & and #
    if (strcmp(split_arr[count - 2], "<") == 0) {
      infile = split_arr[count - 1];
      outfile = split_arr[count - 3];
      free(split_arr[count - 2]);
      split_arr[count - 2] = NULL;
      
      if ((count - 4) > 0 && (strcmp(split_arr[count - 4], ">") == 0)) {
        free(split_arr[count - 4]);
        split_arr[count - 4] = NULL;
      }

    } else if (strcmp(split_arr[count - 2], ">") == 0) {
      outfile = split_arr[count - 1];
      infile = split_arr[count - 3];
      free(split_arr[count - 2]);
      split_arr[count - 2] = NULL;

      if ((count - 4) > 0 && (strcmp(split_arr[count - 4], "<") == 0)) {
        free(split_arr[count - 4]);
        split_arr[count - 4] = NULL;
      }
    }
    //printf("%s\n", outfile);
    //printf("%s\n", infile);
    //printf("%d\n", background);
    // PARSING END

    // {TODO: Add exit built-in command}
    // BUILT-INS START
    // EXIT BUILT-IN
exit:
    if (strcmp(split_arr[0], "exit") == 0) {
      if (split_arr[2]) {
        fprintf(stderr, "Too many arguments\n");
        goto free_stuff; //idk maybe don't continue?? if continue change to free stuff i guess
      }
      if (split_arr[1]) {
        printf("%s", split_arr[1]);
        for(int i = 0; i < strlen(split_arr[1]); i++) {
          if (isdigit(split_arr[1][i]) == 0) {
            fprintf(stderr, "Argument must be a number\n");
            goto free_stuff; //again probably change this
          }
        }
        fprintf(stderr, "\nexit\n");
        exit(atoi(split_arr[1]));
      }
      fprintf(stderr, "\nexit\n");
      exit(atoi(fg_exit));
    }

    // CD BUILT-IN
    if (!split_arr[0]) goto free_stuff; //will likely need to change continue to a jump to free stuff
    
    if (strcmp(split_arr[0], "cd") == 0) {
      if (split_arr[2]) {
        fprintf(stderr, "Too many arguments\n");
        goto free_stuff; //CHANGE THIS
      } 
      if (split_arr[1]) {
        if (chdir(split_arr[1]) == -1) {
          fprintf(stderr, "Invalid directory\n");
          goto free_stuff; //CHANGE THIS
        } else {
          chdir(split_arr[1]);
        }
      } else {
        chdir(getenv("HOME"));
      }
    }
    // BUILT-INS END
    else {
      exec_cmd(split_arr);
    }

free_stuff:
    //continue;
    // probably keep these 3 things to be freed inside main loop
    for (int i = 0; i < count; i++) {
      free(split_arr[i]);
    }
    free(split_arr);
    //free(line);
  }
  free(line);
  free(smallsh_pid);
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

/*
// function for splitting line into separate words
size_t split_str(char **split_arr, char *line) {
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
*/

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

/*
// function for parsing
void parse_arr(char ***split_arr, size_t count, char **outfile, char **infile, int *background) {
  if (count < 3) return;

  // case for when there are comments
  for(size_t i = 0; i < count; i++) {
    if (strcmp(split_arr[i], "#") == 0) {
      free(split_arr[i]);
      split_arr[i] = NULL;
      
      if ((i - 1) > 0 && (strcmp(split_arr[i - 1], "&") == 0)) {
        *background = 1;
        free(split_arr[i - 1]);
        split_arr[i - 1] = NULL;
      }

      if ((i - 3) > 0) {
        if (strcmp(split_arr[i - 3], "<") == 0) {
          infile = split_arr[i - 2];
          outfile = split_arr[i - 4];
          free(split_arr[i - 3]);
          split_arr[i - 3] = NULL;

          if ((i - 5) > 0 && (strcmp(split_arr[i - 5], ">") == 0)) {
            free(split_arr[i - 5]);
            split_arr[i - 5] = NULL;
          }

        } else if (strcmp(split_arr[i - 3], ">") == 0) {
          outfile = split_arr[i - 2];
          infile = split_arr[i - 4];
          free(split_arr[i - 3]);
          split_arr[i - 3] = NULL;

          if ((i - 5) > 0 && (strcmp(split_arr[i - 5], "<") == 0)) {
           free(split_arr[i - 5]);
            split_arr[i - 5] = NULL;
          }
        }
      }
    }
  }
  // case where process should be run in background
  if (strcmp(split_arr[count - 1], "&") == 0) {
    *background = 1;
    free(split_arr[count - 1]);
    split_arr[count - 1] = NULL;
  }

  // case where there is no & and #
  if (strcmp(split_arr[count - 2], "<") == 0) {
    infile = split_arr[count - 1];
    outfile = split_arr[count - 3];
    free(split_arr[count - 2]);
    split_arr[count - 2] = NULL;
    
    if ((count - 4) > 0 && (strcmp(split_arr[count - 4], ">") == 0)) {
      free(split_arr[count - 4]);
      split_arr[count - 4] = NULL;
    }

  } else if (strcmp(split_arr[count - 2], ">") == 0) {
    outfile = split_arr[count - 1];
    infile = split_arr[count - 3];
    free(split_arr[count - 2]);
    split_arr[count - 2] = NULL;

    if ((count - 4) > 0 && (strcmp(split_arr[count - 4], "<") == 0)) {
      free(split_arr[count - 4]);
      split_arr[count - 4] = NULL;
    }
  }
}
*/
void exec_cmd(char **split_arr) {
   int childStatus;

   pid_t spawnPid = fork();

   switch(spawnPid){
     case -1:
       perror("fork()\n");
       exit(1);
       break;
     case 0:
       // child process
       //fprintf(stderr, "CHILD(%d) running %s command\n", getpid(), split_arr[0]);
       execvp(split_arr[0], split_arr);
       // exec only returns if there is an error
       perror("execvp");
       exit(2);
       break;
     default:
       // in parent process
       // wait for child's termination
       spawnPid = waitpid(spawnPid, &childStatus, 0);
       //fprintf(stderr, "PARENT(%d): child(%d) terminated. Exiting\n", getpid(), spawnPid);
       break;
   }
}
