#include "memory_utils.h"
#include "word_types.h"
#include <windows.h>
#include <stdio.h>

size_t getAvailableSystemMemoryMB(void)
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return (size_t)(status.ullAvailPhys / (1024 * 1024));
}

size_t calculateSafeMemoryLimit(void)
{
    size_t availableMB = getAvailableSystemMemoryMB();
    size_t safeLimitMB = (size_t)(availableMB * MAX_MEMORY_PERCENTAGE);
    return (safeLimitMB < MIN_MEMORY_MB) ? MIN_MEMORY_MB : safeLimitMB;
}

size_t calculateMaxWords(size_t memoryLimitMB)
{
    size_t totalBytes = memoryLimitMB * 1024 * 1024;
    return totalBytes / sizeof(Word);
}

void printMemoryInfo(void)
{
    size_t memoryLimit = calculateSafeMemoryLimit();
    size_t maxWords = calculateMaxWords(memoryLimit);
    if(maxWords > MAX_WORDS)
        maxWords = MAX_WORDS;

    printf("System Memory Info:\n");
    printf("Available Memory: %zu MB\n", getAvailableSystemMemoryMB());
    printf("Using Memory Limit: %zu MB\n", memoryLimit);
    printf("Max Words Capacity: %zu\n", maxWords);
}