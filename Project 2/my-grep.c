#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 

// Number of possible characters, this covers extended ASCII
#define ALPHABET_SIZE 256

// Implemented Boyer Moore Algorithm to better understand how the actual grep works
// https://www.geeksforgeeks.org/boyer-moore-algorithm-for-pattern-searching/

// Preprocess the bad character heuristic
void bad_char_heuristic(char *pattern, int pattern_len, int bad_char_table[ALPHABET_SIZE]) {
    // Initialize all occurrences as -1
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        bad_char_table[i] = -1;
    }

    // Fill the actual value of last occurrence of a character
    for (int i = 0; i < pattern_len; i++) {
        bad_char_table[(unsigned char) pattern[i]] = i;
    }
}


void boyer_moore_search(char *line, char *pattern) {
    
    int line_len = strlen(line);
    int pattern_len = strlen(pattern);
    bool found = false;

    int bad_char_table[ALPHABET_SIZE];
    bad_char_heuristic(pattern, pattern_len, bad_char_table);
    
    // Position in line
    int shift = 0; 

    // Run until remaining line is <= pattern, meaning all matches would've been found
    while (shift <= (line_len - pattern_len)) {
        // Start at the last character, matching is done from right to left
        int j = pattern_len - 1;
        // Loop as long as characters match, if a match is found j is -1
        while (j >= 0 && pattern[j] == line[shift + j]) {
            j--;
        }

        // Pattern found, print
        if (j < 0) {
            // Prevents printing the same line multiple times in case multiple matches
            if (!found) {
                printf("%s", line);
                found = true;
            }
            // If pattern is longer than line increment shift by 1 to break the while loop
            // Otherwise we shift to last occurance of next character
            shift += (shift + pattern_len < line_len) ? pattern_len - bad_char_table[(unsigned char)line[shift + pattern_len]] : 1;
        } else {
            // line[shift + j] mismatched character in the line, bad_char_table[...] is the last occurance of the character
            int bad_char_shift = j - bad_char_table[(unsigned char)line[shift + j]];
            // Shifting atleast by 1
            shift += (bad_char_shift > 1) ? bad_char_shift : 1;
        }
    }
}


int main(int argc, char *argv[]) {
    // Checking for search term
    if (argc < 2) {
        printf("my-grep: searchterm [file ...]\n");
        exit(1);
    }

    char *search = argv[1];

    // If search term is empty, match nothing and exit
    if (strlen(search) == 0) {
        exit(0);
    }

    char *line = NULL;
    size_t len = 0;
    FILE *file = NULL;

    // If only search term is provided, read from standard input
    if (argc == 2) {
        while(getline(&line, &len, stdin) != -1) {
            if (strstr(line, search) != NULL) {
                printf("%s", line);
            }
        }
    } else {
        // Process each file passed as an argument
        for (int i = 2; i < argc; i++) {
            file = fopen(argv[i], "r");
            if (file == NULL) {
                printf("my-grep: cannot open file\n");
                free(line);
                exit(1);
            }
            while(getline(&line, &len, file) != -1) {
                boyer_moore_search(line, search);

            }
            fclose(file);
        }
    }

    free(line);
    return 0;

}