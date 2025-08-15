#define _POSIX_C_SOURCE 200809L  // For strdup
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_WORD_LENGTH 6
#define MAX_WORDS 10000
#define REQUIRED_LENGTH 5
#define COMBINATION_SIZE 5

typedef struct {
    char letters[MAX_WORD_LENGTH];
} Word;

bool isValidWord(const char* word) {
    if (strlen(word) != REQUIRED_LENGTH) return false;
    
    bool used[26] = {false};
    int unique_count = 0;
    
    for (size_t i = 0; word[i]; i++) {
        if (!isalpha((unsigned char)word[i])) return false;
        int index = tolower((unsigned char)word[i]) - 'a';
        if (!used[index]) {
            used[index] = true;
            unique_count++;
        }
    }
    
    return unique_count == REQUIRED_LENGTH;
}

bool isValidCombination(char** words, int count) {
    bool used[26] = {false};
    int total_unique = 0;
    
    for (int i = 0; i < count; i++) {
        for (int j = 0; words[i][j]; j++) {
            int index = tolower(words[i][j]) - 'a';
            if (!used[index]) {
                used[index] = true;
                total_unique++;
            }
        }
    }
    
    return total_unique == 25;
}

void findCombinations(char** words, int wordCount, char** current, int depth, int startIndex) {
    if (depth == COMBINATION_SIZE) {
        if (isValidCombination(current, COMBINATION_SIZE)) {
            for (int i = 0; i < COMBINATION_SIZE; i++) {
                printf("%s ", current[i]);
            }
            printf("\n");
        }
        return;
    }
    
    for (int i = startIndex; i < wordCount; i++) {
        current[depth] = words[i];
        findCombinations(words, wordCount, current, depth + 1, i + 1);
    }
}

int main() {
    clock_t start = clock();
    
    FILE* fptr = fopen("prototype/lists/words_beta.txt", "r");
    if (!fptr) {
        printf("Error: Could not open file. Make sure the file is in the same directory.\n");
        return 1;
    }

    char** words = malloc(MAX_WORDS * sizeof(char*));
    if (!words) {
        printf("Error: Memory allocation failed for words array.\n");
        fclose(fptr);
        return 1;
    }

    int wordCount = 0;
    char buffer[MAX_WORD_LENGTH];

    printf("Reading words from file...\n");

    while (fgets(buffer, sizeof(buffer), fptr) && wordCount < MAX_WORDS) {
        buffer[strcspn(buffer, "\n")] = 0;  // Remove newline
        if (isValidWord(buffer)) {
            // Replace strdup with manual memory allocation and strcpy
            size_t len = strlen(buffer) + 1;
            words[wordCount] = malloc(len);
            if (!words[wordCount]) {
                printf("Error: Memory allocation failed for word %d\n", wordCount);
                // Cleanup previously allocated memory
                for (int i = 0; i < wordCount; i++) {
                    free(words[i]);
                }
                free(words);
                fclose(fptr);
                return 1;
            }
            strcpy(words[wordCount], buffer);
            wordCount++;
        }
    }
    fclose(fptr);

    printf("Successfully read %d valid words\n", wordCount);

    if (wordCount == 0) {
        printf("No valid words found in file.\n");
        free(words);
        return 1;
    }

    char** current = malloc(COMBINATION_SIZE * sizeof(char*));
    if (!current) {
        printf("Error: Memory allocation failed for current array.\n");
        for (int i = 0; i < wordCount; i++) {
            free(words[i]);
        }
        free(words);
        return 1;
    }

    printf("Searching for combinations...\n");
    findCombinations(words, wordCount, current, 0, 0);

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time taken: %.2f seconds\n", time_spent);

    // Cleanup
    for (int i = 0; i < wordCount; i++) {
        free(words[i]);
    }
    free(words);
    free(current);

    printf("Program completed successfully.\n");
    return 0;
}
