#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "word_types.h"
#include "memory_utils.h"
#include "word_utils.h"
#include "file_io.h"
#include "threading.h"

int main(int argc, char** argv)
{
    // CLI: -i <input>, -t <threads>
    const char* inputPath = "5Words5Letters/lists/unique_words.txt";
    int threadsWanted = 0;
    
    for(int a = 1; a < argc; a++) {
        if(!strcmp(argv[a], "-i") && a + 1 < argc)
            inputPath = argv[++a];
        else if(!strcmp(argv[a], "-t") && a + 1 < argc)
            threadsWanted = atoi(argv[++a]);
    }

    // Memory setup
    size_t memoryLimit = calculateSafeMemoryLimit();
    size_t maxWords = calculateMaxWords(memoryLimit);
    if(maxWords > MAX_WORDS)
        maxWords = MAX_WORDS;

    printMemoryInfo();

    words = (Word*)malloc(sizeof(Word) * maxWords);
    if(!words) {
        fprintf(stderr, "Failed to allocate memory for words\n");
        return 1;
    }

    // Load words from file
    int validWords = load_words_from_file(inputPath, maxWords);
    if(validWords < 0) {
        free(words);
        return 1;
    }

    if(wordCount < COMBO_LEN) {
        printf("Need at least %d words to find %d-word combinations\n", COMBO_LEN, COMBO_LEN);
        free(words);
        return 0;
    }

    // Sort by mask and remove duplicates
    qsort(words, wordCount, sizeof(Word), compare_words);
    dedupe_by_mask_after_sort();

    // Set up threading
    int hw = detect_thread_count();
    int T = threadsWanted > 0 ? threadsWanted : hw;
    if(T > wordCount)
        T = wordCount;
    if(T < 1)
        T = 1;
    printf("\nUsing %d threads\n", T);

    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * T);
    ThreadArg* args = (ThreadArg*)malloc(sizeof(ThreadArg) * T);

    int chunk = wordCount / T;
    struct timespec start, end;
    if(clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        fprintf(stderr, "Warning: Failed to get start time\n");

    atomic_store(&g_processed_i, 0);
    atomic_store(&totalCount, 0);

    printf("Starting search for %d-word combinations using all %d letters...\n", COMBO_LEN, WORDLEN * COMBO_LEN);

    // Create and run worker threads
    for(int t = 0; t < T; t++) {
        args[t].start = t * chunk;
        args[t].end = (t == T - 1) ? wordCount : (t + 1) * chunk;
        args[t].thread_id = t;
        pthread_create(&threads[t], NULL, worker, &args[t]);
    }
    
    for(int t = 0; t < T; t++)
        pthread_join(threads[t], NULL);

    // Report results
    if(clock_gettime(CLOCK_MONOTONIC, &end) == 0) {
        double seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("\nExecution time: %.3f seconds\n", seconds);
    }

    fprintf(stderr, "\n");
    printf("Total valid %d-word combinations: %llu\n", COMBO_LEN, atomic_load(&totalCount));

    // Clean up
    free(args);
    free(threads);
    free(words);
    return 0;
}