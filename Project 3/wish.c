#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#define MAX_PID_COUNT 100
#define ERROR_MSG "An error has occurred\n"
#define MEMORY_ERROR_MSG "Failed to allocate memory\n"
#define COMMAND_ERROR_MSG "Command not found\n"
#define ARGS_ERROR_MSG "Either too many or too few arguments\n"
#define FORK_ERROR_MSG "Process failed to fork\n"
#define PID_ERROR_MSG "Pid fail\n"
#define REDIRECT_ERROR_MSG "Redirection fail\n"
#define FILE_ERROR_MSG "Error writing or reading file\n"


typedef struct {
    char *command;              // Command name
    char **args;                // Array for tokenized representation of a command
    char *output_file;          // Output file for redirection (or NULL if none)
    int redirect;               // 0 for none, 1 for output redirection
    size_t arg_count;           // Argument counter (number of tokens)
} Command;

bool init_struct(Command *command, char *command_name, size_t arg_count, char **args, char *output_file);
void free_struct(Command *command);
bool update_path(Command *command, char ***paths, size_t *path_count);
char *resolve_path(char *command_name, char **paths, size_t path_count);
void parse_line(char *curr_line, char **paths, size_t *path_count);
Command *parse_command(char *command);
void execute_command(Command *command, char **paths, size_t path_count);
bool built_in_commands(Command *command, char ***paths, size_t *path_count, char *curr_line);
char *trim(char *str);
void parse_free(char **args, size_t args_count, char *command_dup, char *output_file);
bool parse_error(void *pointer, char **args, size_t args_count, char *command_dup, char *output_file);
void custom_write(int fd, const char *msg, size_t len);

// Initializes a Command struct with the given parameters
// Allocates memory for command name, argument list, and optional output file
// Returns true on success, false on memory allocation failure
bool init_struct(Command *command, char *command_name, size_t arg_count, char **args, char *output_file) {
    command->command = strdup(command_name); // Duplicate command name string
    if (command->command == NULL) {
        custom_write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
        return false;
    }

    command->arg_count = arg_count;

    command->args = malloc(sizeof(char *) * (arg_count)); // Allocate memory for argument list
    if (command->args == NULL) { // On failure to allocate memory free allocated memory, print error message and return gracefully
        custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
        free(command->command);
        return false;
    }
    for (int i = 0; i < arg_count; i++) { // Duplicate each argument string into the command struct
        command->args[i] = strdup(args[i]);
        if (command->args[i] == NULL) { // On failure to allocate memory free allocated memory, print error message and return gracefully
            custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
            for (int j = 0; j < i; j++) {
                free(command->args[j]);
            }
            free(command->args);
            free(command->command);
            return false;
        }
    }

    if (output_file != NULL) {
        command->redirect = 1;  // Set redirect flag
        command->output_file = strdup(output_file); // Duplicate output file string
        if (command->output_file == NULL) { // On failure to allocate memory free allocated memory, print error message and return gracefully
            custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
            for (int i = 0; i < arg_count; i++) {
                free(command->args[i]);
            }
            free(command->args);
            free(command->command);
            return false;
        }
    } else {
        command->redirect = 0;
        command->output_file = NULL;
    }
    return true;
}

// Frees all dynamically allocated memory associated with a Command struct
// This includes the command name, argument list, and output file if present
void free_struct(Command *command) {
    if (command == NULL)
        return; // Return on NULL value

    if (command->command != NULL) {
        free(command->command); // Free command value
    }

    if (command->args != NULL) {
        for (int i = 0; i < command->arg_count; i++) {
            free(command->args[i]); // Free each argument string
        }
        free(command->args); // Free argument list array
    }

    if (command->output_file != NULL) {
        free(command->output_file); // Free output file if applicable
    }

    free(command); // Free the struct
}

// Trims leading and trailing whitespace from a string
// Returns a newly allocated trimmed string
// Returns an empty string if the input is all whitespace
// On memory allocation failure, returns NULL and writes an error message to stderr
char *trim(char *str) {
    // Skip leading spaces
    while (isspace(*str)) str++;

    if (*str == '\0') {
        // Empty or all spaces
        return strdup("");
    }

    // Find end of string
    const char *end = str + strlen(str) - 1;

    // Skip trailing spaces
    while (end > str && isspace(*end)) {
        end--;
    }

    size_t len = end - str + 1;

    char *trimmed = malloc(len + 1); // Allocate space for trimmed string (+1 for null terminator)
    if (trimmed == NULL) {
        custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
        return NULL;
    }

    strncpy(trimmed, str, len);
    trimmed[len] = '\0';

    return trimmed;
}

// Function to update Paths
// Extracts new path values from a Command struct and updates the paths list accordingly
bool update_path(Command *command, char ***paths, size_t *path_count) {
    // Cleanup old path(s)
    size_t count = *path_count;
    for (int i = 0; i < count; i++) {
        free((*paths)[i]);
    }

    size_t new_count = command->arg_count - 1; // First arg is 'path', rest are actual paths
    *path_count = new_count;
    
    if (new_count == 0) {
        // No new paths provided
        return true;
    }

    free(*paths);

    // Allocate memory for new path(s)
    char **new_paths = malloc(sizeof(char *) * (new_count));
    if (new_paths == NULL) {
        custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
        return true;
    }  

    // Add the paths from command args
    for (int i = 0; i < new_count; i++) {
        new_paths[i] = strdup(command->args[i + 1]);
        if (new_paths[i] == NULL) {
            custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
            return true;
        }
    }

    *paths = new_paths; // Update the caller's pointer to the new paths

    return true;
}

// Function to resolve a path for non built in commands e.g., ls, pwd
// Tries to locate the full path of a command by checking executability in all known directories
// Returns the full path string if found, NULL otherwise
char *resolve_path(char *command_name, char **paths, size_t path_count) {
    // No path set or command provided
    if (paths == NULL || command_name == NULL || path_count == 0) {
        // Command not found
        custom_write(STDERR_FILENO, COMMAND_ERROR_MSG, strlen(COMMAND_ERROR_MSG));
        return NULL;
    }

    // Loop and check each path in the list, default is /bin
    for (int i = 0; i < path_count; i++) {
        size_t path_size = strlen(paths[i]) + strlen(command_name) + 2;
        char *full_path = malloc(path_size);
        if (full_path == NULL) {
            custom_write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
            return NULL;
        }

        // Form the full path
        snprintf(full_path, path_size, "%s/%s", paths[i], command_name);

        // Check if the file can be accessed for execution
        if (access(full_path, X_OK) == 0) {
            return full_path;
        }

        free(full_path);
    }

    return NULL; // Command not found in paths
}

// Function to parse an input line and execute it
void parse_line(char *curr_line, char **paths, size_t *path_count) {

    size_t pid_count = 0;       // Track child processes amount
    pid_t pids[MAX_PID_COUNT];  // Array to hold child process IDs

    if (strstr(curr_line, "&") != NULL) { // Parallel execution mode when & is present

        size_t command_array_size = 0;  // Number of parsed commands
        Command **command_array = malloc(sizeof(Command *) * 2); // Initial size for two commands
        if (command_array == NULL) {
            custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
            return;
        }

        // Split the input by & to different commands
        char *curr_command = NULL;
        char *saveptr = NULL;
        
        curr_command = strtok_r(curr_line, "&", &saveptr);

        while (curr_command != NULL) {

            // Parse the current command and add it to the array
            Command *command = parse_command(curr_command);
            if(command == NULL) { // On fail cleanup already parsed commands
                custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
                for (int i = 0; i < command_array_size; i++) {
                    free_struct(command_array[i]);  // Free each Command
                }
                free(command_array);  // Free the array of commands
                return;
            }
            
            // Allocate more memory for additional commands
            Command **temp_command = realloc(command_array, sizeof(Command *) * (command_array_size + 1));
            if (temp_command == NULL) {
                custom_write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
                for (int i = 0; i < command_array_size; i++) {
                    free_struct(command_array[i]);  // Free each Command
                }
                free(command_array);  // Free the array of commands
                free_struct(command);
                return;
            }
            command_array = temp_command;

            command_array[command_array_size] = command;
            command_array_size++;

            curr_command = strtok_r(NULL, "&", &saveptr); // Move to next parallel command
        }

        // Execute all parsed commands in parallel
        for (int i = 0; i < command_array_size; i++) {

            // Check if command is built-in and handle internally
            if (built_in_commands(command_array[i], &paths, path_count, curr_line)) {
                free_struct(command_array[i]);
                continue;
            }

            // Fork a child process to run the command
            pid_t pid = fork();
            if (pid != 0) {
                // Parent process
                pids[pid_count] = pid;
                pid_count++;
                
            } else if (pid == 0) {
                // Child process
                size_t count = *path_count;                
                execute_command(command_array[i], paths, count);
                exit(0);
            } else {
                // Fork fail
                custom_write(STDERR_FILENO, FORK_ERROR_MSG, strlen(FORK_ERROR_MSG));
            }

        }

        // Wait for all child processes to complete
        for (int i = 0; i < pid_count; i++) {
            pid_t wpid = waitpid(pids[i], NULL, 0);
            if (wpid == -1) {
                //waitpid fail
                custom_write(STDERR_FILENO, PID_ERROR_MSG, strlen(PID_ERROR_MSG)); 
            }
        }

        // Cleanup allocated memory
        for (int i = 0; i < command_array_size; i++) {
            free_struct(command_array[i]);  // Free each Command
        }
        free(command_array);  // Free the array of commands

    } else { // Single command execution

        // Parse the input into a Command struct
        Command *command = parse_command(curr_line);
        if(command == NULL) {
            return;
        }

        // Handle built-in commands internally
        if(built_in_commands(command, &paths, path_count, curr_line) == true) {
            free_struct(command);  // Free Command
            return;
        }

        // Fork a new process
        pid_t pid = fork();
            if (pid != 0) {
                // Parent process
                pids[pid_count++] = pid;

                int status;
                pid_t wpid = waitpid(pid, &status, 0);

                if (wpid == -1) {
                    //waitpid fail
                    custom_write(STDERR_FILENO, PID_ERROR_MSG, strlen(PID_ERROR_MSG)); 
                } else {
                    if (WIFEXITED(status)) {
                        // Child exited normally
                        // printf("exited status %d\n", WEXITSTATUS(status));
                    } else if (WIFSIGNALED(status)) {
                        // Child was terminated by a signal
                        // printf("terminated signal\n");
                    } else if (WIFSTOPPED(status)) {
                        // Child was stopped
                        // printf("stopped signal\n");
                    }
                }
                
            } else if (pid == 0) {
                // Child process
                size_t count = *path_count;  
                execute_command(command, paths, count);
                exit(0);
            } else {
                // Fork fail
                custom_write(STDERR_FILENO, FORK_ERROR_MSG, strlen(FORK_ERROR_MSG));
            }

        free_struct(command);  // Free Command
    }

}

// A helper function to free memory allocated during parse_command
void parse_free(char **args, size_t args_count, char *command_dup, char *output_file) {
    
    // Free each argument string
    if (args != NULL) {
        for (size_t i = 0; i < args_count; i++) {
            free(args[i]);
        }
        free(args); // Free args array
    }

    // Free the duplicated command string
    if (command_dup != NULL) {
        free(command_dup);
    }

    // Free the output file string if applicable
    if (output_file != NULL) {
        free(output_file);
    }
}

// A helper function to cleanup memory and write error message
bool parse_error(void *pointer, char **args, size_t args_count, char *command_dup, char *output_file) {
    if (pointer == NULL) {
        custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
        parse_free(args, args_count, command_dup, output_file);
        return true; // True indicates error
    }
    return false; // False, no error
}

// Write wrapper
void custom_write(int fd, const char *msg, size_t len) {
    if (write(fd, msg, len) == -1) {
        write(STDERR_FILENO, msg, len);
    }
}

// Parses a single command line string into a Command struct
// This function processes a raw command string (e.g., "ls -l > out.txt")
// And returns a fully initialized Command pointer
Command *parse_command(char *command) {

    char **args = NULL;         // Dynamic array to hold argument strings
    size_t args_count = 0;      // Counter for arguments
    char *output_file = NULL;   // Holds the output redirection file if one exists

    // Allocate memory for arguments array
    args = malloc(sizeof(char *));
    if (parse_error(args, args, args_count, NULL, output_file)) {
        return NULL;
    }

    // Duplicate the input
    char *command_dup = malloc(strlen(command) * 2 + 1); // worst case: space before and after every char
    if (command_dup == NULL) {
        custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
        return NULL;
    }

    // Add spaces around > to make tokenization consistent
    int j = 0;
    for (int i = 0; command[i] != '\0'; i++) {
        if (command[i] == '>') {
            command_dup[j++] = ' ';
            command_dup[j++] = '>';
            command_dup[j++] = ' ';
        } else {
            command_dup[j++] = command[i];
        }
    }
    command_dup[j] = '\0';
    
    // Trim leading and trailing whitespace
    char *temp = trim(command_dup);  
    free(command_dup);
    if (temp == NULL) {
        // Trim failed due to malloc error
        custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
        return NULL;
    }
    
    command_dup = temp;
    if (parse_error(command_dup, args, args_count, NULL, output_file)) {
        return NULL;
    }

    // Empty command after trimming
    if (strlen(command_dup) == 0) {
        free(command_dup);
        return NULL;
    }

    char *token = NULL;
    char *saveptr = NULL;
    
    // Tokenize the string
    token = strtok_r(command_dup, " ", &saveptr);

    // If strtok finds no tokens, return NULL
    if (token == NULL) {
        custom_write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
        free(command_dup);
        return NULL;
    }

    // Cannot start with redirection
    if (strcmp(token, ">") == 0) {
        custom_write(STDERR_FILENO, REDIRECT_ERROR_MSG, strlen(REDIRECT_ERROR_MSG));
        free(command_dup);
        return NULL;
    }

    // First argument is the command
    char *command_name = token;

    // Parse the tokens
    while (token != NULL) {

        // Check if redirection and handle it
        if (strcmp(token, ">") == 0) {
            // Get the next token to use as output filename, we're only expecting one file after >
            token = strtok_r(NULL, " ", &saveptr);

            if (token == NULL || strtok_r(NULL, " ", &saveptr) != NULL) {
                custom_write(STDERR_FILENO, REDIRECT_ERROR_MSG, strlen(REDIRECT_ERROR_MSG));
                parse_free(args, args_count, command_dup, output_file);
                return NULL;
            } else {
                char *trimmed_token = trim(token);
                output_file = strdup(trimmed_token);
                free(trimmed_token);
            }
            token = strtok_r(NULL, " ", &saveptr);
            continue;

        }

        // Append current token to the argument list
        char **temp_args = realloc(args, sizeof(char *) * (args_count + 1));
        if (parse_error(temp_args, args, args_count, command_dup, output_file)) {
            return NULL;
        }
        args = temp_args;

        // Trim token and store
        char *trimmed_token = trim(token);
        if (parse_error(trimmed_token, args, args_count, command_dup, output_file)) {
            return NULL;
        }

        args[args_count] = trimmed_token; 
        args_count++;

        token = strtok_r(NULL, " ", &saveptr); // Move to the next token
    }

    // Validate the output filename
    if (output_file != NULL && strchr(output_file, '>') != NULL) {
        custom_write(STDERR_FILENO, REDIRECT_ERROR_MSG, strlen(REDIRECT_ERROR_MSG));
        parse_free(args, args_count, command_dup, output_file);
        return NULL;
    }

    // Allocate memory for the final Command struct
    Command *cmd = malloc(sizeof(Command));
    if (parse_error(cmd, args, args_count, command_dup, output_file)) {
        return NULL;
    }

    // Initialize the Command struct with the parsed arguments
    if (!init_struct(cmd, command_name, args_count, args, output_file)) {
        free(cmd);
        parse_free(args, args_count, command_dup, output_file);
        return NULL;
    }

    // Cleanup
    parse_free(args, args_count, command_dup, output_file);

    return cmd;
}

// Function to execute a command after parsing
// This function handles the execution of non-built-in shell commands
void execute_command(Command *command, char **paths, size_t path_count) {
    // Create arguments list for execution
    char *args[command->arg_count];
    for (int i = 0; i < command->arg_count; i++) {
        args[i] = command->args[i];
    }
    args[command->arg_count] = NULL; // Null-terminate the argument list

    // Resolve path from default /bin or user provided paths
    char *full_path = resolve_path(command->command, paths, path_count);
    if (full_path == NULL) {
        return;
    }

    // Handle file redirection if enabled
    if (command->redirect == 1) {
        int fd = open(command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            custom_write(STDERR_FILENO, FILE_ERROR_MSG, strlen(FILE_ERROR_MSG));
            free(full_path);
            return;
        }

        // Redirect stdout and stderr to file using file descriptor
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    // Execute command
    if (execv(full_path, args) == -1) {
        // execv fail
        fprintf(stderr, "%s: %s\n", command->command, strerror(errno));
        exit(1);
    }

    free(full_path);
    return;
}

// Function to handle built-in shell commands (exit, cd, path)
// Returns true if the command was a built-in and has been handled, false if the command is not a built-in
bool built_in_commands(Command *command, char ***paths, size_t *path_count, char *curr_line) {
    size_t count = *path_count;
    // Check if the user wants to exit
    if (strcmp(command->command, "exit") == 0 && command->arg_count == 1) {
        // Free struct
        free_struct(command);
        // Free path(s)
        for (int i = 0; i < count; i++) {
            free((*paths)[i]);
        }
        free(*paths);
        free(curr_line);
        exit(0);
    } else if (strcmp(command->command, "exit") == 0 && command->arg_count > 1) {
        // Too many args
        custom_write(STDERR_FILENO, ARGS_ERROR_MSG, strlen(ARGS_ERROR_MSG));
        return true;
    }

    // Check if user wants to change working directory
    if (strcmp(command->command, "cd") == 0) {
        if (command->arg_count != 2) {
            custom_write(STDERR_FILENO, ARGS_ERROR_MSG, strlen(ARGS_ERROR_MSG));
        } else {
            if (chdir(command->args[1]) == -1) {
                custom_write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
            }
        }
        return true;
    }

    // Check if user wants to update paths
    if (strcmp(command->command, "path") == 0) {
        if (update_path(command, paths, path_count)) {
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[]) {

    // Allocate memory and set the default /bin path
    char **paths = NULL;
    size_t path_count = 1;
    paths = malloc(sizeof(char *));
    if (paths == NULL) {
        custom_write(STDERR_FILENO, MEMORY_ERROR_MSG, strlen(MEMORY_ERROR_MSG));
        exit(1);
    }
    paths[0] = strdup("/bin");

    char *input = NULL;
    size_t len = 0;
    
    // Determine execution mode
    // mode 0 = interactive, mode 1 = batch, mode -1 = invalid
    int mode = (argc == 1) ? 0 :
           (argc == 2) ? 1 : -1;


    switch (mode) {
        case 0: // Interactive mode
            while (1) {
                // Flush the buffer manually to ensure printing is not delayed as we don't print a newline
                printf("wish> ");
                fflush(stdout);
                if (getline(&input, &len, stdin) == -1) {
                    break;
                }

                // Remove the newline character
                input[strcspn(input, "\n")] = '\0';
    
                // Process the input line
                parse_line(input, paths, &path_count);
            }
            break;

        case 1: // Batch mode
            if (argc < 2) {
                custom_write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
                for (int i = 0; i < path_count; i++) {
                    free(paths[i]);
                }
                free(paths);
                exit(1);
            }

            // Open the specified batch file
            FILE *file = fopen(argv[1], "r");
            if (file == NULL) {
                custom_write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
                for (int i = 0; i < path_count; i++) {
                    free(paths[i]);
                }
                free(paths);
                exit(1);
            }

            // Read and process the batch file line by line
            char *line = NULL;
            size_t len = 0;
            while (getline(&line, &len, file) != -1) {
                line[strcspn(line, "\n")] = 0;  // Remove newline character
                parse_line(line, paths, &path_count);
            }
        
            free(line);
            fclose(file);

            break;

        default:
            // Handle unexpected mode value
            custom_write(STDERR_FILENO, ARGS_ERROR_MSG, strlen(ARGS_ERROR_MSG));
            free(input);
            for (int i = 0; i < path_count; i++) {
                free(paths[i]);
            }
            free(paths);
            exit(1);
    }

    // Cleanup
    free(input);
    for (int i = 0; i < path_count; i++) {
        free(paths[i]);
    }
    free(paths);

    return 0;
}