#include "word_utils.h"
#include "word_types.h"
#include <string.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif

// Count set bits in a 32-bit integer
int bit_count_u32(unsigned int x)
{
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_popcount(x);
#elif defined(_MSC_VER)
    return __popcnt(x);
#else
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    return ((x + (x >> 4)) & 0x0F0F0F0Fu) * 0x01010101u >> 24;
#endif
}

// Convert word to bitmask (remapped bits)
unsigned int word_to_mask(const char* w)
{
    unsigned int mask = 0;
    for(int i = 0; i < WORDLEN; i++) {
        char c = (char)(w[i] | 32);
        if(c < 'a' || c > 'z')
            return 0;
        int bit = LETTER_FREQ_POS[c - 'a'];
        if(mask & (1u << bit))
            return 0; // duplicate letter
        mask |= 1u << bit;
    }
    return mask;
}

// Sort by mask
int compare_words(const void* a, const void* b)
{
    const Word* wa = (const Word*)a;
    const Word* wb = (const Word*)b;
    
    // Compare bit counts first for better cache locality
    int bits_a = bit_count_u32(wa->mask);
    int bits_b = bit_count_u32(wb->mask);
    
    if(bits_a != bits_b)
        return bits_a - bits_b;

    // If bit counts equal, compare masks
    if(wa->mask != wb->mask)
        return (wa->mask > wb->mask) - (wa->mask < wb->mask);

    // If masks are equal, compare strings
    return strcmp(wa->word, wb->word);
}

// Remove anagrams (words with identical letter masks)
void dedupe_by_mask_after_sort(void)
{
    if(wordCount <= 1)
        return;
    int writePos = 0;
    for(int readPos = 0; readPos < wordCount; readPos++) {
        if(readPos == 0 || words[readPos].mask != words[readPos - 1].mask) {
            if(writePos != readPos)
                words[writePos] = words[readPos];
            writePos++;
        }
    }
    wordCount = writePos;
}