<h2>wish — A Simple Unix Shell Implementation</h2>

Wish (short for Wisconsin Shell) is a simple educational Unix shell, or command-line interpreter (CLI) written in C. It supports basic shell functionality including command execution, output redirection, parallel command execution, and several built-in commands. This project was educational in nature and was made to familiarize myself with systems programming in the Linux environment and process creation and management, and other functions of shells.


<h3>Features</h3>

- Command execution: The shell is able to execute a command by the user in interactive mode
- Parallel command execution: Parallel execution of multiple commands is supported using the & operator 
- Batch mode: Read commands from a file and execute them
- Built-in-commands:
    - cd: Change the current working directory
    - path: Change the search path(s) for executables
    - exit: Terminate the shell
- Redirection: Using the > operator allows the redirection of stdout and stderr to a file
- Error handling: Supports multiple error messages


<h3> Usage</h3>
Interactive Mode
Launch wish with no arguments to enter interactive mode, runs in a REPL (Read-Eval-Print Loop) shell-like interface:
```
./wish
wish> ls
wish> cd ..
wish> exit
```

Batch Mode
Provide a batch file containing commands:
```
./wish script.txt
```

Redirection
Using the > operator redirect the output of a command to a file:
```
ls > output.txt
```
Assumptions:
- Output redirection > is only allowed at the end of a command
- Only one output redirection per command is supported


Parallel command execution
Using the & operator execute multiple commands in parallel:
```
ls & pwd & echo "done"
```


<h3>Error handling</h3>

All errors output atleast a generic message to stderr
```
An error has occurred
```

Other error messages include:
- Invalid syntax
- Memory allocation failure
- Redirection syntax issue
- Invalid amount of arguments


<h3>Known limitations</h3>

- No piping | or input redirection <
- No background execution with & after a command
- No environment variable expansion (e.g., $HOME)


<h3>Compilation</h3>

```
gcc -o wish wish.c
```