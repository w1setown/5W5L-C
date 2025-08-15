#define _POSIX_C_SOURCE 200809L
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
#define TOTAL_LETTERS (COMBINATION_SIZE * REQUIRED_LENGTH)

typedef struct {
    char* word;
    unsigned int letter_mask;  // Bitmask for quick letter checking
} Word;

typedef struct {
    Word* words;
    size_t count;
    size_t capacity;
} WordList;

// Function prototypes
bool isValidWord(const char* word);
unsigned int createLetterMask(const char* word);
WordList* createWordList(size_t initial_capacity);
void freeWordList(WordList* list);
bool addWord(WordList* list, const char* word);
void printStats(const WordList* list);
bool isValidWordCombination(Word** words, int count);
void findWordCombinations(WordList* list);
void processCombination(Word** combination);

// Test functions
void runTests(void);
bool testWordValidation(void);
bool testLetterMask(void);

bool isValidWord(const char* word) {
    if (!word || strlen(word) != REQUIRED_LENGTH) 
        return false;
    
    unsigned int mask = 0;
    
    for (size_t i = 0; i < REQUIRED_LENGTH; i++) {
        if (!isalpha((unsigned char)word[i])) 
            return false;
        
        int bit = tolower((unsigned char)word[i]) - 'a';
        if (mask & (1u << bit)) // Check if letter already used
            return false;
        
        mask |= (1u << bit);
    }
    
    return true;
}

unsigned int createLetterMask(const char* word) {
    unsigned int mask = 0;
    for (size_t i = 0; word[i]; i++) {
        mask |= (1u << (tolower(word[i]) - 'a'));
    }
    return mask;
}

WordList* createWordList(size_t initial_capacity) {
    WordList* list = malloc(sizeof(WordList));
    if (!list) return NULL;
    
    list->words = malloc(initial_capacity * sizeof(Word));
    if (!list->words) {
        free(list);
        return NULL;
    }
    
    list->count = 0;
    list->capacity = initial_capacity;
    return list;
}

void freeWordList(WordList* list) {
    if (!list) return;
    
    for (size_t i = 0; i < list->count; i++) {
        free(list->words[i].word);
    }
    free(list->words);
    free(list);
}

bool addWord(WordList* list, const char* word) {
    if (!list || !word || list->count >= list->capacity) 
        return false;
    
    if (!isValidWord(word)) 
        return false;
    
    list->words[list->count].word = strdup(word);
    if (!list->words[list->count].word) 
        return false;
    
    list->words[list->count].letter_mask = createLetterMask(word);
    list->count++;
    return true;
}

void printStats(const WordList* list) {
    printf("Word List Statistics:\n");
    printf("Total valid words: %zu\n", list->count);
    printf("Memory usage: %zu bytes\n", 
           list->count * (sizeof(Word) + REQUIRED_LENGTH + 1));
}

bool isValidWordCombination(Word** words, int count) {
    unsigned int combined_mask = 0;
    
    for (int i = 0; i < count; i++) {
        // If any letters overlap, combination is invalid
        if (combined_mask & words[i]->letter_mask) {
            return false;
        }
        combined_mask |= words[i]->letter_mask;
    }
    
    // Count bits to ensure we have exactly 25 unique letters
    int letter_count = 0;
    unsigned int mask = combined_mask;
    while (mask) {
        if (mask & 1) letter_count++;
        mask >>= 1;
    }
    
    return letter_count == TOTAL_LETTERS;
}

void processCombination(Word** combination) {
    for (int i = 0; i < COMBINATION_SIZE; i++) {
        printf("%s ", combination[i]->word);
    }
    printf("\n");
}

void findCombinationsRecursive(WordList* list, Word** current, bool* used, int depth, size_t start) {
    if (depth == COMBINATION_SIZE) {
        if (isValidWordCombination(current, COMBINATION_SIZE)) {
            processCombination(current);
        }
        return;
    }

    // Early validation to prune invalid branches
    if (depth > 0 && !isValidWordCombination(current, depth)) {
        return;
    }

    for (size_t i = start; i < list->count; i++) {
        if (!used[i]) {
            used[i] = true;
            current[depth] = &list->words[i];
            findCombinationsRecursive(list, current, used, depth + 1, i + 1);
            used[i] = false;
        }
    }
}

void findWordCombinations(WordList* list) {
    Word** current = malloc(COMBINATION_SIZE * sizeof(Word*));
    bool* used = calloc(list->count, sizeof(bool));
    
    if (!current || !used) {
        fprintf(stderr, "Memory allocation failed\n");
        free(current);
        free(used);
        return;
    }

    printf("Searching for valid combinations...\n");
    findCombinationsRecursive(list, current, used, 0, 0);

    free(current);
    free(used);
}

int main(int argc, char* argv[]) {
    clock_t start = clock();
    
    // Run tests in debug mode
    #ifdef DEBUG
    runTests();
    #endif
    
    const char* filename = (argc > 1) ? argv[1] : "prototype/lists/words_beta.txt";
    FILE* fptr = fopen(filename, "r");
    if (!fptr) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return EXIT_FAILURE;
    }

    WordList* wordList = createWordList(MAX_WORDS);
    if (!wordList) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(fptr);
        return EXIT_FAILURE;
    }

    char buffer[MAX_WORD_LENGTH];
    size_t invalid_words = 0;
    size_t total_words = 0;

    printf("Reading words from file...\n");

    while (fgets(buffer, sizeof(buffer), fptr)) {
        buffer[strcspn(buffer, "\n")] = 0;
        total_words++;
        
        if (!addWord(wordList, buffer)) {
            invalid_words++;
        }
    }

    fclose(fptr);
    
    printf("\nFile processing complete:\n");
    printf("Total words processed: %zu\n", total_words);
    printf("Invalid words skipped: %zu\n", invalid_words);
    printStats(wordList);

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("\nTime taken: %.2f seconds\n", time_spent);

    findWordCombinations(wordList);
    freeWordList(wordList);
    return EXIT_SUCCESS;
}

// Test functions implementation
void runTests(void) {
    printf("Running tests...\n");
    bool all_passed = true;
    
    all_passed &= testWordValidation();
    all_passed &= testLetterMask();
    
    printf("Tests %s\n", all_passed ? "PASSED" : "FAILED");
}

bool testWordValidation(void) {
    const char* test_cases[] = {
        "SMART",  // valid
        "HELLO",  // invalid (repeated L)
        "CAT",    // invalid (too short)
        "HOUSES", // invalid (too long)
        "SM@RT",  // invalid (special char)
    };
    
    bool expected[] = {true, false, false, false, false};
    bool all_passed = true;
    
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        bool result = isValidWord(test_cases[i]);
        if (result != expected[i]) {
            printf("Test failed for word: %s\n", test_cases[i]);
            all_passed = false;
        }
    }
    
    return all_passed;
}

bool testLetterMask(void) {
    const char* word = "SMART";
    unsigned int mask = createLetterMask(word);
    unsigned int expected = 
        (1u << ('s'-'a')) |
        (1u << ('m'-'a')) |
        (1u << ('a'-'a')) |
        (1u << ('r'-'a')) |
        (1u << ('t'-'a'));
    
    return mask == expected;
}