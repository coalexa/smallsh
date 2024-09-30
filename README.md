# SMALLSH

`smallsh` is a simple shell written in C. 

This program does the following:
- Prints an interactive input prompt
- Parses command line input into semantic tokens
- Implements parameter expansion
  - Shell special parameters `$$`, `$?`, and `$!`
  - Tilde (~) expansion
- Implements two shell built-in commands: `exit` and `cd`
- Executes non-built-in commands using the appropriate `EXEC(3)` function
   - Implements redirection operators '<' and '>'
   - Implements the '&' operator to run commands in the background
- Implements custom behavior for `SIGINT` and `SIGSTP` signals

## How to Run
Use the included makefile then type `make` to compile

Run program using `./smallsh`
