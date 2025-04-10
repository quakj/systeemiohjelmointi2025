#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // If no files, exit
    if (argc < 2) {
        printf("my-unzip: file1 [file2 ...]\n");
        return 1;
    }

    // Process each file given in the command line arguments.
    for (int i = 1; i < argc; i++) {

        FILE *file;
        
        file = fopen(argv[i], "rb");
        if (file == NULL) {
            printf("my-zip: cannot open file\n");
            exit(1);
        }

        int count;
        char character;

        // Read the compressed file, 4 bytes length, 1 byte character
        while (fread(&count, sizeof(int), 1, file) == 1 && fread(&character, sizeof(char), 1, file) == 1) {
            // Print the character
            for (int j = 0; j < count; j++) {
                printf("%c", character);
            }
        }

        fclose(file);
        
    }

    return 0;
}