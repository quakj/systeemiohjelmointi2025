#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // If no files are specified, exit 0
    if (argc == 1) {
        return 0;
    }

    // Process files
    for (int i = 1; i < argc; i++) {
        FILE *infile = fopen(argv[i], "r");
        if (infile == NULL) {
            printf("my-cat: cannot open file\n");
            return 1;
        }

        // Read and print
        char *line = NULL;
        size_t len = 0;
        ssize_t c_count;

        while ((c_count = getline(&line, &len, infile)) != -1) {
            printf("%s", line);
        }

        free(line);
        fclose(infile);
    }
    
    return 0;
}