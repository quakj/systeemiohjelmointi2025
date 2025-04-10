#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 

int main(int argc, char *argv[]) {
    // If no files, exit
    if (argc < 2) {
        printf("my-zip: file1 [file2 ...]\n");
        return 1;
    }

    FILE *file;
    bool first_char = true;
    char prev = '\0';
    char current = '\0';
    size_t count = 0;

    for (int i = 1; i < argc; i++) {
        
        file = fopen(argv[i], "r");
        if (file == NULL) {
            printf("my-zip: cannot open file\n");
            exit(1);
        }

        while ((current = fgetc(file)) != EOF) {
            if (first_char) {
                prev = current;
                count = 1;
                first_char = false;
            } else {
                // Current character is the same as previous
                if (current == prev) {
                    count++;
                } else {
                    // Write 4-byte integer and the character.
                    fwrite(&count, sizeof(int), 1, stdout);
                    fwrite(&prev, sizeof(char), 1, stdout);
                    count = 1;
                    prev = current;
                }
            }
        }

        fclose(file);

    }

    if (!first_char) {
        fwrite(&count, sizeof(int), 1, stdout);
        fwrite(&prev, sizeof(char), 1, stdout);
    }

    return 0;
}