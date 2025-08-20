#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h> // Atomic operations for thread safety. Atomic operations are used to safely update shared variables across threads. It does this by ensuring that the updates are visible to all threads and that they occur without interference from other threads.
#include <windows.h>

#define MAX_WORDS 400000
#define WORDLEN   5
#define THREADS   8   // adjust to CPU cores
#define MEMORY_SAFETY_MARGIN 0.2  // Keep 20% memory free
#define MIN_MEMORY_MB 100 // Minimum memory limit in MB
#define MAX_MEMORY_PERCENTAGE 0.75  // Use max 75% of available memory | It is recommended to keep this value low to avoid memory pressure, if you decide to change this, be cautious.

typedef struct {
    char word[WORDLEN+1];
    unsigned int mask;
} Word;

Word* words;
int wordCount = 0;

atomic_ulong totalCount = 0; // atomic_ulong is an unsigned long integer type that is safe to use in concurrent environments. This means it can be safely updated by multiple threads without additional synchronization.

typedef struct { // Thread arguments for each worker
    int start, end;
} ThreadArg;

unsigned int word_to_mask(const char *w) {
    unsigned int mask = 0;
    for (int i = 0; i < WORDLEN; i++) {
        int bit = w[i] - 'a';
        if (mask & (1u << bit)) return 0; // duplicate inside word
        mask |= 1u << bit;
    }
    return mask;
}

// Thread function for processing word combinations
void *worker(void *arg) {
    ThreadArg *t = (ThreadArg *)arg; // Cast the argument to the correct type
    unsigned long localCount = 0;

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

                    for (int m = l+1; m < wordCount; m++) {
                        unsigned int m5 = words[m].mask;
                        if (!m5 || (m1234 & m5)) continue;
                        localCount++;
                    }
                }
            }
        }
    }

    atomic_fetch_add(&totalCount, localCount);
    return NULL;
}

/*
 * Worker Function Explanation:
 * --------------------------
 * This function implements a nested loop algorithm to find valid 5-word combinations
 * where no letters repeat across all words. It uses bit masks for efficient letter checking.
 *
 * Algorithm:
 * 1. Takes a segment of words (start to end) assigned to this thread
 * 2. Uses nested loops to try all possible 5-word combinations
 * 3. For each word combination:
 *    - m1: mask for first word
 *    - m12: combined mask for first two words
 *    - m123: combined mask for first three words
 *    - m1234: combined mask for first four words
 *    - m5: mask for fifth word
 */

 // Get available system memory in MB
size_t getAvailableSystemMemoryMB() {
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return (size_t)(status.ullAvailPhys / (1024 * 1024));  // Fixed member name
}


// Calculate safe memory limit
size_t calculateSafeMemoryLimit() {
    size_t availableMB = getAvailableSystemMemoryMB();
    size_t safeLimitMB = (size_t)(availableMB * MAX_MEMORY_PERCENTAGE);
    
    if (safeLimitMB < MIN_MEMORY_MB) {
        return MIN_MEMORY_MB;
    }
    return safeLimitMB;
}

size_t calculateMaxWords(size_t memoryLimitMB) {
    size_t totalBytes = memoryLimitMB * 1024 * 1024;
    size_t baseMemory = sizeof(Word) * THREADS;  // Thread overhead
    size_t availableBytes = totalBytes - baseMemory;
    return availableBytes / sizeof(Word);
}

int main() {
    // Calculate safe memory limits
    size_t memoryLimit = calculateSafeMemoryLimit();
    size_t maxWords = calculateMaxWords(memoryLimit);
    
    if (maxWords > MAX_WORDS) {
        maxWords = MAX_WORDS;
    }

    // Print system memory information
    printf("System Memory Info:\n");
    printf("Available Memory: %zu MB\n", getAvailableSystemMemoryMB());
    printf("Using Memory Limit: %zu MB\n", memoryLimit);
    printf("Max Words Capacity: %zu\n", maxWords);
    
    // Modify the word array definition to use calculated size
    words = malloc(sizeof(Word) * maxWords);
    if (!words) {
        fprintf(stderr, "Failed to allocate memory for words\n");
        return 1;
    }

    FILE *f = fopen("prototype/lists/unique_words.txt", "r"); // Open the file for reading | Change this path as needed
    if (!f) { perror("open"); return 1; }

    // Read words from file
    char buf[64];
    while (fscanf(f, "%s", buf) == 1) {
        if (strlen(buf) != WORDLEN) continue;
        unsigned int mask = word_to_mask(buf);
        if (mask) {
            strcpy(words[wordCount].word, buf);
            words[wordCount].mask = mask;
            wordCount++;
        }
    }
    fclose(f); // Close the file

    printf("Loaded %d valid words\n", wordCount);

    pthread_t threads[THREADS];
    ThreadArg args[THREADS];

    // Divide work into chunks for each thread
    int chunk = wordCount / THREADS;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int t = 0; t < THREADS; t++) {
        args[t].start = t * chunk;
        args[t].end = (t == THREADS-1) ? wordCount : (t+1)*chunk;
        pthread_create(&threads[t], NULL, worker, &args[t]);
    }
    // Wait for all threads to finish
    for (int t = 0; t < THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double seconds = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec)/1e9;

    printf("Total valid 5-word combinations: %lu\n", totalCount);
    printf("Execution time: %.3f seconds\n", seconds);

    free(words); // Free allocated memory for words
    return 0;
}
