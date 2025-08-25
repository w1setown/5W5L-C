#include "word_types.h"
#include <stddef.h>

// Letter-frequency based position remap (0 = rarest, 25 = most common)
const int LETTER_FREQ_POS[26] = { 16, 9,  23, 25, 22, 10, 21, 5,  24, 1, 7, 12, 15,
                                  6,  20, 3,  2,  11, 14, 19, 13, 17, 0, 8, 18, 4 };

// Global state
Word* words = NULL;
int wordCount = 0;
atomic_ullong totalCount = 0;
atomic_uint g_processed_i = 0;