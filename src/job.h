#ifndef _JOB_H_
#define _JOB_H_

#include "common.h"

#define MAX_WORKER_THREADS 7

typedef struct ThreadJob {
    int (*run)(void *data);
    void *args;
} ThreadJob;

#define MAX_THREAD_JOBS 1024

void job_system_initialize(void);
void job_system_terminate(void);

void job_queue_begin(void);
void job_queue_end(void);

void push_job(ThreadJob job);

#endif // _JOB_H_
