#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_ARG_COUNT 100
#define ERROR_MSG "An error has occurred\n"

char *resolve_path(char *command) {
    // Get the PATH environment variable
    char *path_env = getenv("PATH");
    // If $PATH is not set, we can't find the command
    if (path_env == NULL) {
        return NULL;
    }

    // Copy path so we don't change the original
    char *path_copy = strdup(path_env);
    if (path_copy == NULL) {
        // duplication fail
        write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG)); 
        return NULL;
    }

    char *dir = strtok(path_copy, ":");

    while (dir != NULL) {
        //Allocate memory for path to return
        // +2 for "/"" and "\0"
        size_t path_size = strlen(dir) + strlen(command) + 2;  
        char *full_path = malloc(path_size);
        if (full_path == NULL) {
            free(path_copy);
            return NULL;
        }

        // Construct path
        snprintf(full_path, path_size, "%s/%s", dir, command);

        // Check if the file exists and is executable
        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return full_path;
        }

        free(full_path);
        // get next value
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}

int main(int argc, char *argv[]) {

    // char error_message[30] = "";
    char *input = NULL;
    size_t len = 0;

    while (1) {
        // Print prompt, flush the buffer manually to ensure printing is not delayed as we don't print a \n
        printf("wish> ");
        fflush(stdout);

        if (getline(&input, &len, stdin) == -1) {
            break; // Exit on EOF or error
        }

        // Remove the newline character
        input[strcspn(input, "\n")] = '\0';

        // Check if the user wants to exit
        if (strcmp(input, "exit") == 0) {
            break;
        }

        char *args[MAX_PID_COUNT];
        int argCount = 0;
        char *token = strtok(input, " ");

        while (token != NULL && argCount < MAX_PID_COUNT - 1) {
            args[argCount++] = token;
            token = strtok(NULL, " ");
        }

        args[argCount] = NULL;  // Null-terminate the argument list

        if (argCount == 0) {
            continue;
        }

        /*
        // Test print the args
        for (int i = 0; args[i] != NULL; i++) {
            printf("args[%d]: %s\n", i, args[i]);
        }
        */

        char *full_path = resolve_path(args[0]);
        //printf("path: %s\n", full_path);
        if (full_path == NULL) {
            printf("command not found: %s\n", args[0]);
            continue;
        }

        // Fork child process
        pid_t pid = fork();
        if (pid < 0) {
            // fork fail
            free(full_path);
            write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
            continue;
        } else if (pid == 0) {
            execv(full_path, args);
            // execv fail
            // write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG)); 
            // exit(1);
        } else {
            int status;
            pid_t wpid = waitpid(pid, &status, 0);
            free(full_path);
            if (wpid == -1) {
                //waitpid fail
                write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG)); 
            } else {
                if (WIFEXITED(status)) {
                    // printf("exited status %d\n", WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    // printf("terminated signal\n");
                } else if (WIFSTOPPED(status)) {
                    // printf("stopped signal\n");
                }
            }

        }

    }

    free(input);

    return 0;
}