#include "threading.h"
#include "word_types.h"
#include "word_utils.h"
#include <windows.h>
#include <stdio.h>

// Detect logical CPU count
int detect_thread_count(void)
{
    DWORD n = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    return (n > 0) ? (int)n : 8;
}

// Worker thread: Find all valid 5-word combinations using nested loops and bitmasks
void* worker(void* arg)
{
    ThreadArg* t = (ThreadArg*)arg;
    unsigned long long localCount = 0;
    int lastPercent = -1;
    const int required_bits = WORDLEN * COMBO_LEN;

    for(int i = t->start; i < t->end; i++) {
        unsigned int m1 = words[i].mask;
        if(!m1)
            continue;
        for(int j = i + 1; j < wordCount; j++) {
            unsigned int m2 = words[j].mask;
            if(!m2 || (m1 & m2))
                continue;
            unsigned int m12 = m1 | m2;
            for(int k = j + 1; k < wordCount; k++) {
                unsigned int m3 = words[k].mask;
                if(!m3 || (m12 & m3))
                    continue;
                unsigned int m123 = m12 | m3;
                for(int l = k + 1; l < wordCount; l++) {
                    unsigned int m4 = words[l].mask;
                    if(!m4 || (m123 & m4))
                        continue;
                    unsigned int m1234 = m123 | m4;
                    int bits_so_far = bit_count_u32(m1234);
                    if(bits_so_far > required_bits)
                        continue;
                    int remaining_bits = required_bits - bits_so_far;
                    for(int m = l + 1; m < wordCount; m++) {
                        unsigned int m5 = words[m].mask;
                        if(!m5 || (m1234 & m5))
                            continue;
                        if(bit_count_u32(m5) == remaining_bits) {
                            localCount++;
                        }
                    }
                }
            }
        }
        // Progress reporting (only from thread 0)
        if(t->thread_id == 0) {
            int processed = atomic_load(&g_processed_i);
            int percent = (processed * 100) / wordCount;
            if(percent > lastPercent && percent <= 100) {
                fprintf(stderr, "Progress: %d%% (processed %d/%d)\r", percent, processed, wordCount);
                lastPercent = percent;
            }
        }
        atomic_fetch_add(&g_processed_i, 1);
    }
    atomic_fetch_add(&totalCount, localCount);
    return NULL;
}