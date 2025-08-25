#ifndef WORD_UTILS_H
#define WORD_UTILS_H

// Bit manipulation and word processing functions
int bit_count_u32(unsigned int x);
unsigned int word_to_mask(const char* w);
int compare_words(const void* a, const void* b);
void dedupe_by_mask_after_sort(void);

#endif // WORD_UTILS_H