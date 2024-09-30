# SMALLSH

`smallsh` is a simple shell written in C. 

This program does the following:
- Prints an interactive input prompt
- Prases command line input into semantic tokens
- Implements parament expansion
  - Shell special parameters `$$`, `$?`, and `$!`
  - Tilde (~) expansion
- Implement two shell built-in commands: `exit` and `cd`
- Execute non-built-in commands using the appropriate `EXEC(3)` function
   - Implement redirection operators '<' and '>'
   - Implement the '&' operator to run commands in the background
- Implement custom behavior for `SIGINT` and `SIGSTP` signals

## How to Run
Use the included makefile then type `make` to compile

Run program using `./smallsh`
