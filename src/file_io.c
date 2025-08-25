#include "file_io.h"
#include "word_types.h"
#include "word_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int load_words_from_file(const char* inputPath, size_t maxWords)
{
    FILE* f = fopen(inputPath, "r");
    if(!f) {
        perror("Failed to open input file");
        return -1;
    }

    char buf[64];
    int totalRead = 0, validWords = 0;
    
    while(wordCount < (int)maxWords && fscanf(f, "%63s", buf) == 1) {
        totalRead++;
        if((int)strlen(buf) != WORDLEN)
            continue;
        
        unsigned int mask = word_to_mask(buf);
        if(mask && bit_count_u32(mask) == WORDLEN) {
            memcpy(words[wordCount].word, buf, WORDLEN);
            words[wordCount].word[WORDLEN] = '\0';
            words[wordCount].mask = mask;
            wordCount++;
            validWords++;
        }
    }
    fclose(f);

    printf("Read %d total words from file\n", totalRead);
    printf("Found %d valid %d-letter words with unique letters\n", validWords, WORDLEN);
    
    return validWords;
}