#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>

#define MAX_WORDS 400000
#define WORDLEN   5
#define THREADS   8   // adjust to CPU cores

typedef struct {
    char word[WORDLEN+1];
    unsigned int mask;
} Word;

Word words[MAX_WORDS];
int wordCount = 0;

atomic_ulong totalCount = 0;

typedef struct {
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

void *worker(void *arg) {
    ThreadArg *t = (ThreadArg *)arg;
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

int main() {
    FILE *f = fopen("prototype/lists/unique_words.txt", "r");
    if (!f) { perror("open"); return 1; }

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
    fclose(f);

    printf("Loaded %d valid words\n", wordCount);

    pthread_t threads[THREADS];
    ThreadArg args[THREADS];

    int chunk = wordCount / THREADS;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int t = 0; t < THREADS; t++) {
        args[t].start = t * chunk;
        args[t].end = (t == THREADS-1) ? wordCount : (t+1)*chunk;
        pthread_create(&threads[t], NULL, worker, &args[t]);
    }

    for (int t = 0; t < THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double seconds = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec)/1e9;

    printf("Total valid 5-word combinations: %lu\n", totalCount);
    printf("Execution time: %.3f seconds\n", seconds);

    return 0;
}
