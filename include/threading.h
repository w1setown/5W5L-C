#ifndef THREADING_H
#define THREADING_H

// Threading functions
int detect_thread_count(void);
void* worker(void* arg);

#endif // THREADING_H