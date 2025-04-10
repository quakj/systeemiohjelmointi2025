#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define INITIAL_CAPACITY 16

void file_error(const char *filename) {
    fprintf(stderr, "error: cannot open file '%s'\n", filename);
    exit(1);
}

void error_exit(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

int check_file_parity(const char *input, const char *output) {
    struct stat st_in, st_out;

    // No input causes error
    if (stat(input, &st_in) != 0) {
        file_error(input);
    }

    // But no output file is fine, means we can just create the file
    if (stat(output, &st_out) != 0) {
        return 0;
    }

    // Checking based on inode, strcmp would fail here if files are the same but in different paths
    if (st_in.st_ino == st_out.st_ino) {
        return 1;
    }

    return 0;
}


int main(int argc, char *argv[]) {
    
    FILE *infile = stdin;
    FILE *outfile = stdout;
    char *input_filename = NULL;
    char *output_filename = NULL;

    // Check command line arguments
    if (argc > 3) {
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(1);
    }

    // Open input file
    if (argc == 2 || argc == 3) {
        input_filename = argv[1];
        infile = fopen(input_filename, "r");
        if (!infile) {
            file_error(input_filename);
        }
    }

    // Test if the input file and output file are the same file
    if (argc == 3) {
        output_filename = argv[2];
        if (check_file_parity(input_filename, output_filename) != 0) {
            fprintf(stderr, "error: input and output file must differ\n");
            fclose(infile);
            exit(1);
        }
        outfile = fopen(output_filename, "w");
        if (!outfile) {
            fclose(infile);
            file_error(output_filename);
        }
    }

    // Dynamically allocate memory
    size_t capacity = INITIAL_CAPACITY;
    size_t count = 0;
    char **lines = malloc(capacity * sizeof(char *));
    if (!lines) {
        error_exit("malloc failed");
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t c_count;

    //Using getline(), which dynamically allocates memory because *line = NULL 
    while ((c_count = getline(&line, &len, infile)) != -1) {

        lines[count] = line;
        count++;

        //Allocate new buffer
        line = NULL;
        len = 0;
        
        //Reallocate more memory if needed
        if (count >= capacity) {
            capacity *= 2;
            char **temp = realloc(lines, capacity * sizeof(char *));
            if (temp == NULL) {
                //Cleanup in case error before exiting
                for (size_t i = 0; i < count; i++) {
                    free(lines[i]);
                }
                free(lines);
                error_exit("malloc failed");
            }
            lines = temp;
        }
    }

    free(line);

    //Write
    for (ssize_t i = count - 1; i >= 0; i--) {
        if (fputs(lines[i], outfile) == EOF) {
            fprintf(stderr, "error: write failed\n");
            for (size_t j = 0; j < count; j++) {
                free(lines[j]);
            }
            //Cleanup in case error before exiting
            free(lines);
            if (infile != stdin) fclose(infile);
            if (outfile != stdout) fclose(outfile);
            exit(1);
        }
        free(lines[i]);
    }


    // Free
    free(lines);

    // Close files
    if (infile != stdin)
        fclose(infile);
    if (outfile != stdout)
        fclose(outfile);

    return 0;
}