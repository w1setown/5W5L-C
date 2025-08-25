#ifndef WORD_TYPES_H
#define WORD_TYPES_H

#include <stdatomic.h>
#include <stddef.h>

// Configuration constants
#define MAX_WORDS 400000
#define WORDLEN 5
#define COMBO_LEN 5
#define MAX_MEMORY_PERCENTAGE 0.75
#define MIN_MEMORY_MB 100

// Letter-frequency based position remap (0 = rarest, 25 = most common)
extern const int LETTER_FREQ_POS[26];

// Core data structures
typedef struct {
    char word[WORDLEN + 1];
    unsigned int mask;
} Word;

typedef struct {
    int start, end, thread_id;
} ThreadArg;

// Global state
extern Word* words;
extern int wordCount;
extern atomic_ullong totalCount;
extern atomic_uint g_processed_i;

#endif // WORD_TYPES_H