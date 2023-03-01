#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>


void prompt_function(); //function to print PS1 parameter
char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub);
void exec_cmd(char **split_arr, char *infile, char *outfile, int background, struct sigaction *default_action);
void dummy_handler(int signo);

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
    // setup signal handling for SIGINT and SIGTSTP
    struct sigaction ignore_action = {0}, SIGINT_dummy = {0}, default_action = {0};
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &ignore_action, NULL);
    sigaction(SIGINT, &ignore_action, NULL);

    if (errno != 0) errno = 0;

    // checking for any un-waited-for background processes in the same process group ID
    pid_t bg_child;
    int child_status;
    while ((bg_child = waitpid(0, &child_status, WUNTRACED | WNOHANG)) > 0) {
      if (WIFEXITED(child_status)) {
        fprintf(stderr, "Child process %jd done. Exit status %d.\n", (intmax_t) bg_child, WEXITSTATUS(child_status));
      }
      if (WIFSIGNALED(child_status)) {
        fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t) bg_child, WTERMSIG(child_status));
      }
      if (WIFSTOPPED(child_status)) {
        kill(bg_child, SIGCONT);
        fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) bg_child);
      }
    }
    
    prompt_function();
    split_arr = malloc(sizeof *split_arr);

    // set SIGINT to dummy so system call gets interrupted and prompt can be reprinted
    SIGINT_dummy.sa_handler = dummy_handler;
    sigfillset(&SIGINT_dummy.sa_mask);
    SIGINT_dummy.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_dummy, &ignore_action);

    // get input
    ssize_t line_length = getline(&line, &n, stdin);

    // set SIGINT back to SIG_IGN
    sigaction(SIGINT, &ignore_action, NULL);

    // checking for EOF and getline error
    if (feof(stdin)) {
      fprintf(stderr, "\nexit\n");
      exit(0);
    }
    if (line_length == -1) {
      fprintf(stderr, "%c", '\n');
      clearerr(stdin);
      errno = 0;
      continue;
    }

    // SPLITTING START
    char *token = NULL;
    char *dup_token = NULL;
    size_t count = 0;

    for (size_t i = 0;; i++) {
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
    }
    // EXPANSION END

    // PARSING START
    int background = 0;
    char *outfile = NULL;
    char *infile = NULL;
    if (count < 3) goto built_ins;

    for(size_t i = 0; i < count; i++) {
      // start by checking for comments
      if ((strcmp(split_arr[i], "#") == 0) || (strncmp(split_arr[i], "#", 1) == 0)) {
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
              free(split_arr[i - 3]);
              split_arr[i - 3] = NULL;

              if ((i - 5) > 0 && (strcmp(split_arr[i - 5], ">") == 0)) {
                outfile = split_arr[i - 4];
                free(split_arr[i - 5]);
                split_arr[i - 5] = NULL;
              }
            } 
            else if (strcmp(split_arr[i - 3], ">") == 0) {
              outfile = split_arr[i - 2];
              free(split_arr[i - 3]);
              split_arr[i - 3] = NULL;

              if ((i - 5) > 0 && (strcmp(split_arr[i - 5], "<") == 0)) {
                infile = split_arr[i - 4];
                free(split_arr[i - 5]);
                split_arr[i - 5] = NULL;
              }
            }
          }
        }
        // case when there's a # and & is not the last word
        if ((i - 2) > 0 && (strcmp(split_arr[i - 2], "<") == 0)) {
          infile = split_arr[i - 1];
          free(split_arr[i - 2]);
          split_arr[i - 2] = NULL;

          if ((i - 4) > 0 && (strcmp(split_arr[i - 4], ">") == 0)) {
            outfile = split_arr[i - 3];
            free(split_arr[i - 4]);
            split_arr[i - 4] = NULL;
          }
        } 
        else if ((i - 2) > 0 && (strcmp(split_arr[i - 2], ">") == 0)) {
          outfile = split_arr[i - 1];
          free(split_arr[i - 2]);
          split_arr[i - 2] = NULL;

          if ((i - 4) > 0 && (strcmp(split_arr[i - 4], "<") == 0)) {
            infile = split_arr[i - 3];
            free(split_arr[i - 4]);
            split_arr[i - 4] = NULL;
          }
        }
        break; // exit out of for loop after parsing if # was found
      }
      // cases when there is no # and last word in array is found
      else if (i == count - 1) {
        // case when & is the last word
        if (strcmp(split_arr[i], "&") == 0) {
          background = 1;
          free(split_arr[i]);
          split_arr[i] = NULL;

          if ((i - 2) > 0) {
            if (strcmp(split_arr[i - 2], "<") == 0) {
              infile = split_arr[i - 1];
              free(split_arr[i - 2]);
              split_arr[i - 2] = NULL;

              if ((i - 4) > 0 && (strcmp(split_arr[i - 4], ">") == 0)) {
                outfile = split_arr[i - 3];
                free(split_arr[i - 4]);
                split_arr[i - 4] = NULL;
              }
            } 
            else if (strcmp(split_arr[i - 2], ">") == 0) {
              outfile = split_arr[i - 1];
              free(split_arr[i - 2]);
              split_arr[i - 2] = NULL;

              if ((i - 4) > 0 && (strcmp(split_arr[i - 4], "<") == 0)) {
                infile = split_arr[i - 3];
                free(split_arr[i - 4]);
                split_arr[i - 4] = NULL;
              }
            }
          }
        }
        // case when there is no # and &
        else if (strcmp(split_arr[i - 1], "<") == 0) {
          infile = split_arr[i];
          free(split_arr[i - 1]);
          split_arr[i - 1] = NULL;
          
          if ((i - 3) > 0 && (strcmp(split_arr[i - 3], ">") == 0)) {
            outfile = split_arr[i - 2];
            free(split_arr[i - 3]);
            split_arr[i - 3] = NULL;
          }

        } 
        else if (strcmp(split_arr[i - 1], ">") == 0) {
          outfile = split_arr[i];
          free(split_arr[i - 1]);
          split_arr[i - 1] = NULL;

          if ((i - 3) > 0 && (strcmp(split_arr[i - 3], "<") == 0)) {
            infile = split_arr[i - 2];
            free(split_arr[i - 3]);
            split_arr[i - 3] = NULL;
          }
        }
      }
    }
    // PARSING END

built_ins:
    // BUILT-INS START
    if (split_arr[0] == NULL) goto free_array;  // no command present

    // EXIT BUILT-IN
    if (strcmp(split_arr[0], "exit") == 0) {
      if (split_arr[2]) {
        fprintf(stderr, "Too many arguments\n");
        goto free_array;
      }
      if (split_arr[1]) {
        for(size_t i = 0; i < strlen(split_arr[1]); i++) {
          if (isdigit(split_arr[1][i]) == 0) {
            fprintf(stderr, "Argument must be a number\n");
            goto free_array;
          }
        }
        fprintf(stderr, "\nexit\n");
        kill(0, SIGINT);
        exit(atoi(split_arr[1]));
      }
      fprintf(stderr, "\nexit\n");
      kill(0, SIGINT);
      exit(atoi(fg_exit));
    }

    // CD BUILT-IN
    if (strcmp(split_arr[0], "cd") == 0) {
      if (split_arr[2]) {
        fprintf(stderr, "Too many arguments\n");
        goto free_array;
      } 
      if (split_arr[1]) {
        if (chdir(split_arr[1]) == -1) {
          fprintf(stderr, "Invalid directory\n");
          goto free_array;
        } else {
          chdir(split_arr[1]);
        }
      } else {
        chdir(getenv("HOME"));
      }
    }
    // BUILT-INS END
    else {
      exec_cmd(split_arr, infile, outfile, background, &default_action);
    }

free_array:
    for (size_t i = 0; i < count; i++) {
      free(split_arr[i]);
    }
    free(split_arr);
  }
  free(line);
  free(smallsh_pid);
  free(fg_exit);
  free(bg_pid);
}

// function for PS1 prompt
void prompt_function(){
  char *ps_param = getenv("PS1");
  if (ps_param == NULL) {
    ps_param = "";
    fprintf(stderr, "%s", ps_param);
  } else {
    fprintf(stderr, "%s", ps_param);
  }
}

// function from course materials
// function for expansion AKA search and replace
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

// function for executing non-built-in commands
void exec_cmd(char **split_arr, char *infile, char *outfile, int background, struct sigaction *default_action) {
   int child_status;
   int input_fd = 0;  //0 for stdin
   int output_fd = 1;  //1 for stdout

   pid_t spawn_pid = fork();

   switch(spawn_pid){
     case -1:
       fprintf(stderr, "Could not fork()\n");
       exit(1);
       break;
     case 0:
       // child process
       // reset SIGINT and SIGTSTP to default behavior
       sigaction(SIGINT, default_action, NULL);
       sigaction(SIGTSTP, default_action, NULL);

       if (infile != NULL) {
         close(input_fd);
         input_fd = open(infile, O_RDONLY);
         if (input_fd == -1) {
           fprintf(stderr, "Could not open %s for reading\n", infile);
           exit(errno);
         }
       }

       if (outfile != NULL) {
         close(output_fd);
         output_fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
         if (output_fd == -1) {
           fprintf(stderr, "Could not open or create %s for writing\n", outfile);
           exit(errno);
         }
       }
  
       execvp(split_arr[0], split_arr);
       // exec only returns if there is an error
       fprintf(stderr, "Could not execute command %s\n", split_arr[0]);
       exit(errno);
       break;
     default:
       // in parent process
       // check for background processes first
       if (background == 1) {
         waitpid(spawn_pid, &child_status, WUNTRACED | WNOHANG);
         bg_pid = malloc(8);
         sprintf(bg_pid, "%d", spawn_pid);
       }
       // otherwise process is a foreground process
       else {
         waitpid(spawn_pid, &child_status, 0);
         if (WIFEXITED(child_status)) {
           fg_exit = malloc(8);
           sprintf(fg_exit, "%d", WEXITSTATUS(child_status));
         }
         if (WIFSIGNALED(child_status)) {
           int num_sig = 128 + WTERMSIG(child_status);
           fg_exit = malloc(8);
           sprintf(fg_exit, "%d", num_sig);
         } 
         if (WIFSTOPPED(child_status)) {
           kill(spawn_pid, SIGCONT);
           fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) spawn_pid);
           waitpid(spawn_pid, &child_status, WUNTRACED | WNOHANG);
           bg_pid = malloc(8);
           sprintf(bg_pid, "%d", spawn_pid);
         }
       }
       break;
   }
}

// dummy handler for SIGINT signal handling
void dummy_handler(int signo) {

}
