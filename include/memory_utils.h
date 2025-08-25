#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <stddef.h>

// Memory management functions
size_t getAvailableSystemMemoryMB(void);
size_t calculateSafeMemoryLimit(void);
size_t calculateMaxWords(size_t memoryLimitMB);
void printMemoryInfo(void);

#endif // MEMORY_UTILS_H