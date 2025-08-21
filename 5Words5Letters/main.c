#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>
#include <windows.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif

#define MAX_WORDS 400000
#define WORDLEN   5
#define COMBO_LEN 5 // Change this for nth-word combos
#define MAX_MEMORY_PERCENTAGE 0.75
#define MIN_MEMORY_MB 100

// Letter-frequency based position remap (0 = rarest, 25 = most common)
static const int LETTER_FREQ_POS[26] = {
    16, 9, 23, 25, 22, 10, 21, 5, 24, 1, 7, 12, 15, 6, 20, 3, 2, 11, 14, 19, 13, 17, 0, 8, 18, 4
};

// Rarity score for each letter (lower = rarer)
static const int RARITY_SCORE[26] = {
    22, 9, 16, 15, 25, 7, 11, 10, 23, 1, 4, 17, 13, 20, 18, 14, 0, 21, 24, 19, 14, 5, 21, 22, 8, 3
};

typedef struct {
    char word[WORDLEN+1];
    unsigned int mask;
    int rarest_letter_score;
} Word;

Word* words;
int wordCount = 0;
atomic_ullong totalCount = 0;
static atomic_uint g_processed_i = 0;

typedef struct {
    int start, end, thread_id;
} ThreadArg;

// Count set bits in a 32-bit integer
static inline int bit_count_u32(unsigned int x) {
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

// Calculate the rarest letter score for a word
int calculate_rarest_letter_score(const char *w) {
    int rarest_score = 999;
    for (int i = 0; i < WORDLEN; i++) {
        char c = (char)(w[i] | 32);
        if (c < 'a' || c > 'z') return 999;
        int freq_pos = LETTER_FREQ_POS[c - 'a'];
        int rarity_score = RARITY_SCORE[freq_pos];
        if (rarity_score < rarest_score) rarest_score = rarity_score;
    }
    return rarest_score;
}

// Convert word to bitmask (remapped bits)
unsigned int word_to_mask(const char *w) {
    unsigned int mask = 0;
    for (int i = 0; i < WORDLEN; i++) {
        char c = (char)(w[i] | 32);
        if (c < 'a' || c > 'z') return 0;
        int bit = LETTER_FREQ_POS[c - 'a'];
        if (mask & (1u << bit)) return 0; // duplicate letter
        mask |= 1u << bit;
    }
    return mask;
}

// Get available system memory in MB
size_t getAvailableSystemMemoryMB() {
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return (size_t)(status.ullAvailPhys / (1024 * 1024));
}

// Calculate safe memory limit in MB
size_t calculateSafeMemoryLimit() {
    size_t availableMB = getAvailableSystemMemoryMB();
    size_t safeLimitMB = (size_t)(availableMB * MAX_MEMORY_PERCENTAGE);
    return (safeLimitMB < MIN_MEMORY_MB) ? MIN_MEMORY_MB : safeLimitMB;
}

// Calculate max number of words that fit in memory
size_t calculateMaxWords(size_t memoryLimitMB) {
    size_t totalBytes = memoryLimitMB * 1024 * 1024;
    return totalBytes / sizeof(Word);
}

// Detect logical CPU count
static int detect_thread_count(void) {
    DWORD n = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    return (n > 0) ? (int)n : 8;
}

// Sort by rarest letter score, then mask
static int compare_words_by_rarity(const void* a, const void* b) {
    const Word* wa = (const Word*)a;
    const Word* wb = (const Word*)b;
    if (wa->rarest_letter_score != wb->rarest_letter_score)
        return wa->rarest_letter_score - wb->rarest_letter_score;
    if (wa->mask < wb->mask) return -1;
    if (wa->mask > wb->mask) return 1;
    return 0;
}

// Sort by mask (for deduplication)
static int compare_words_by_mask(const void* a, const void* b) {
    const Word* wa = (const Word*)a;
    const Word* wb = (const Word*)b;
    if (wa->mask < wb->mask) return -1;
    if (wa->mask > wb->mask) return 1;
    return 0;
}

// Remove anagrams (words with identical letter masks)
static void dedupe_by_mask_after_sort(void) {
    if (wordCount <= 1) return;
    printf("Before dedup: %d words\n", wordCount);
    int originalCount = wordCount;
    printf("First 10 masks before dedup:\n");
    for (int i = 0; i < 10 && i < wordCount; i++)
        printf("  %s: mask=0x%08x\n", words[i].word, words[i].mask);
    int writePos = 0, duplicatesFound = 0;
    for (int readPos = 0; readPos < wordCount; readPos++) {
        if (readPos == 0 || words[readPos].mask != words[readPos-1].mask) {
            if (writePos != readPos) words[writePos] = words[readPos];
            writePos++;
        } else {
            duplicatesFound++;
            if (duplicatesFound <= 5)
                printf("  Duplicate found: %s (mask=0x%08x) same as %s\n",
                    words[readPos].word, words[readPos].mask, words[readPos-1].word);
        }
    }
    wordCount = writePos;
    printf("After dedup: %d words (removed %d duplicates)\n", wordCount, originalCount - wordCount);
}

// Find all valid 5-word combinations using nested loops and bitmasks
void *worker(void *arg) {
    ThreadArg *t = (ThreadArg *)arg;
    unsigned long long localCount = 0;
    int lastPercent = -1;
    const int required_bits = WORDLEN * COMBO_LEN;

    for (int i = t->start; i < t->end; i++) {
        unsigned int m1 = words[i].mask;
        if (!m1) continue;
        for (int j = i+1; j < wordCount; j++) {
            unsigned int m2 = words[j].mask;
            if (!m2 || (m1 & m2)) continue;
            unsigned int m12 = m1 | m2;
            for (int k = j+1; k < wordCount; k++) {
                unsigned int m3 = words[k].mask;
                if (!m3 || (m12 & m3)) continue;
                unsigned int m123 = m12 | m3;
                for (int l = k+1; l < wordCount; l++) {
                    unsigned int m4 = words[l].mask;
                    if (!m4 || (m123 & m4)) continue;
                    unsigned int m1234 = m123 | m4;
                    int bits_so_far = bit_count_u32(m1234);
                    if (bits_so_far > required_bits) continue;
                    int remaining_bits = required_bits - bits_so_far;
                    for (int m = l+1; m < wordCount; m++) {
                        unsigned int m5 = words[m].mask;
                        if (!m5 || (m1234 & m5)) continue;
                        if (bit_count_u32(m5) == remaining_bits) {
                            localCount++;
                        }
                    }
                }
            }
        }
        // Progress reporting (only from thread 0)
        if (t->thread_id == 0) {
            int processed = atomic_load(&g_processed_i);
            int percent = (processed * 100) / wordCount;
            if (percent > lastPercent && percent <= 100) {
                fprintf(stderr, "Progress: %d%% (processed %d/%d)\r", percent, processed, wordCount);
                lastPercent = percent;
            }
        }
        atomic_fetch_add(&g_processed_i, 1);
    }
    atomic_fetch_add(&totalCount, localCount);
    return NULL;
}

void search_recursive(int depth, int start, unsigned int used, int combo[], unsigned long long* count) {
    if (depth == COMBO_LEN) {
        (*count)++;
        // Optionally print/store combo here
        return;
    }
    for (int i = start; i < wordCount; i++) {
        if ((used & words[i].mask) == 0) {
            combo[depth] = i;
            search_recursive(depth + 1, i + 1, used | words[i].mask, combo, count);
        }
    }
}

#define MAX_BUCKETS 26
typedef struct {
    int* indices;
    int count;
    int capacity;
} Bucket;

Bucket letterBuckets[MAX_BUCKETS];

void init_buckets() {
    for (int i = 0; i < MAX_BUCKETS; i++) {
        letterBuckets[i].indices = malloc(sizeof(int) * 1024);
        letterBuckets[i].count = 0;
        letterBuckets[i].capacity = 1024;
    }
    for (int i = 0; i < wordCount; i++) {
        int rare = words[i].rarest_letter_score;
        if (letterBuckets[rare].count == letterBuckets[rare].capacity) {
            letterBuckets[rare].capacity *= 2;
            letterBuckets[rare].indices = realloc(letterBuckets[rare].indices, sizeof(int) * letterBuckets[rare].capacity);
        }
        letterBuckets[rare].indices[letterBuckets[rare].count++] = i;
    }
}

// Free allocated memory for buckets
void free_buckets() {
    for (int i = 0; i < MAX_BUCKETS; i++) {
        free(letterBuckets[i].indices);
        letterBuckets[i].indices = NULL;
        letterBuckets[i].count = 0;
        letterBuckets[i].capacity = 0;
    }
}

void search_recursive_buckets(int depth, unsigned int used, int combo[], unsigned long long* count, int skips) {
    if (depth == COMBO_LEN) {
        (*count)++;
        return;
    }
    for (int b = 0, s = 0; b < MAX_BUCKETS && s < 2; b++) {
        // Skip bucket if rarest letter already used
        int letter_bit = 1 << b;
        if (used & letter_bit) { s++; continue; }
        for (int i = 0; i < letterBuckets[b].count; i++) {
            int idx = letterBuckets[b].indices[i];
            if ((used & words[idx].mask) == 0) {
                combo[depth] = idx;
                search_recursive_buckets(depth + 1, used | words[idx].mask, combo, count, skips);
            }
        }
    }
}

typedef struct {
    int word_idx;
    unsigned long long local_count;
} RecThreadArg;

void* recursive_thread_worker(void* arg) {
    RecThreadArg* a = (RecThreadArg*)arg;
    int combo[COMBO_LEN];
    combo[0] = a->word_idx;
    unsigned int used = words[a->word_idx].mask;
    a->local_count = 0;
    search_recursive(1, a->word_idx + 1, used, combo, &a->local_count);
    return NULL;
}

int main(int argc, char** argv) {
    // CLI: -i <input>, -t <threads>
    const char* inputPath = "5Words5Letters/lists/unique_words.txt";
    int threadsWanted = 0;
    for (int a = 1; a < argc; a++) {
        if (!strcmp(argv[a], "-i") && a+1 < argc) inputPath = argv[++a];
        else if (!strcmp(argv[a], "-t") && a+1 < argc) threadsWanted = atoi(argv[++a]);
    }

    size_t memoryLimit = calculateSafeMemoryLimit();
    size_t maxWords = calculateMaxWords(memoryLimit);
    if (maxWords > MAX_WORDS) maxWords = MAX_WORDS;

    printf("System Memory Info:\n");
    printf("Available Memory: %zu MB\n", getAvailableSystemMemoryMB());
    printf("Using Memory Limit: %zu MB\n", memoryLimit);
    printf("Max Words Capacity: %zu\n", maxWords);

    words = (Word*)malloc(sizeof(Word) * maxWords);
    if (!words) {
        fprintf(stderr, "Failed to allocate memory for words\n");
        return 1;
    }

    FILE *f = fopen(inputPath, "r");
    if (!f) {
        perror("Failed to open input file");
        free(words);
        return 1;
    }

    char buf[64];
    int totalRead = 0, validWords = 0;
    while (wordCount < (int)maxWords && fscanf(f, "%63s", buf) == 1) {
        totalRead++;
        if ((int)strlen(buf) != WORDLEN) continue;
        unsigned int mask = word_to_mask(buf);
        if (mask && bit_count_u32(mask) == WORDLEN) {
            strncpy(words[wordCount].word, buf, WORDLEN);
            words[wordCount].word[WORDLEN] = '\0';
            words[wordCount].mask = mask;
            words[wordCount].rarest_letter_score = calculate_rarest_letter_score(buf);
            wordCount++;
            validWords++;
        }
    }
    fclose(f);

    printf("Read %d total words from file\n", totalRead);
    printf("Found %d valid %d-letter words with unique letters\n", validWords, WORDLEN);

    if (wordCount < COMBO_LEN) {
        printf("Need at least %d words to find %d-word combinations\n", COMBO_LEN, COMBO_LEN);
        free(words);
        return 0;
    }

    // Sort by mask for deduplication
    qsort(words, wordCount, sizeof(Word), compare_words_by_mask);
    dedupe_by_mask_after_sort();

    printf("Final word count after removing anagrams: %d\n", wordCount);

    // Sort by letter rarity for optimal pruning
    printf("\nSorting words by letter rarity for optimal pruning...\n");
    qsort(words, wordCount, sizeof(Word), compare_words_by_rarity);

    printf("\nFirst 10 words after rarity sorting (rarest letters first):\n");
    for (int i = 0; i < 10 && i < wordCount; i++)
        printf("  %s (rarity score: %d, mask: 0x%08x)\n",
            words[i].word, words[i].rarest_letter_score, words[i].mask);

    // Set up threading
    int hw = detect_thread_count();
    int T = threadsWanted > 0 ? threadsWanted : hw;
    if (T > wordCount) T = wordCount;
    if (T < 1) T = 1;
    printf("\nUsing %d threads\n", T);

    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * T);
    ThreadArg* args = (ThreadArg*)malloc(sizeof(ThreadArg) * T);

    int chunk = wordCount / T;
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        fprintf(stderr, "Warning: Failed to get start time\n");

    atomic_store(&g_processed_i, 0);
    atomic_store(&totalCount, 0);

    printf("Starting search for %d-word combinations using all %d letters...\n", COMBO_LEN, WORDLEN * COMBO_LEN);

    for (int t = 0; t < T; t++) {
        args[t].start = t * chunk;
        args[t].end = (t == T-1) ? wordCount : (t+1)*chunk;
        args[t].thread_id = t;
        pthread_create(&threads[t], NULL, worker, &args[t]);
    }
    for (int t = 0; t < T; t++) pthread_join(threads[t], NULL);

    if (clock_gettime(CLOCK_MONOTONIC, &end) == 0) {
        double seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1e9;
        printf("\nExecution time: %.3f seconds\n", seconds);
    }

    fprintf(stderr, "\n");
    unsigned long long totalCount = 0;
    int combo[COMBO_LEN];
    search_recursive(0, 0, 0, combo, &totalCount);
    printf("Total valid %d-word combinations: %llu\n", COMBO_LEN, totalCount);

    // Parallelize the recursive search by launching a thread for each word in the rarest bucket
    init_buckets();
    int rarest_bucket = 0;
    while (rarest_bucket < MAX_BUCKETS && letterBuckets[rarest_bucket].count == 0) rarest_bucket++;
    int N = letterBuckets[rarest_bucket].count;
    pthread_t* rec_threads = malloc(sizeof(pthread_t) * N);
    RecThreadArg* rec_args = malloc(sizeof(RecThreadArg) * N);

    for (int i = 0; i < N; i++) {
        rec_args[i].word_idx = letterBuckets[rarest_bucket].indices[i];
        rec_args[i].local_count = 0;
        pthread_create(&rec_threads[i], NULL, recursive_thread_worker, &rec_args[i]);
    }
    unsigned long long total_recursive_count = 0;
    for (int i = 0; i < N; i++) {
        pthread_join(rec_threads[i], NULL);
        total_recursive_count += rec_args[i].local_count;
    }
    printf("Total valid %d-word combinations (parallel recursive): %llu\n", COMBO_LEN, total_recursive_count);
    free(rec_threads);
    free(rec_args);

    free(args);
    free(threads);
    free(words);
    free_buckets();
    return 0;
}