#include "os.h"
#include "job.h"

// NOTE: Multithreading job system --------------------------------------

SDL_Thread *thread_pool[MAX_WORKER_THREADS];
SDL_sem *semaphore;

ThreadJob jobs[MAX_THREAD_JOBS];
SDL_atomic_t jobs_pushed;
SDL_atomic_t jobs_done;
SDL_atomic_t next_job;

void job_queue_begin(void) {
    jobs_pushed.value = 0;
    jobs_done.value   = 0;
    next_job.value    = 0;
}

void job_queue_end(void) {
    while(jobs_done.value < jobs_pushed.value) {
        s32 job_index = next_job.value;
        if(job_index < jobs_pushed.value) {
            if(SDL_AtomicCAS(&next_job, job_index, job_index + 1)) {
                ThreadJob *job = jobs + job_index;
                job->run(job->args);
                SDL_AtomicIncRef(&jobs_done);
            }
        }
    }
}

void push_job(ThreadJob job) {
    jobs[jobs_pushed.value] = job;
    SDL_CompilerBarrier();
    SDL_AtomicIncRef(&jobs_pushed);
    SDL_SemPost(semaphore);
}

static int thread_do_jobs(void *data) {

    u8 *memory = (u8 *)data;
    unused(memory);

    for(;;) {
        s32 job_index = next_job.value;
        if(job_index < jobs_pushed.value) {
            if(SDL_AtomicCAS(&next_job, job_index, job_index + 1)) {
                ThreadJob *job = jobs + job_index;
                job->run(job->args);
                SDL_AtomicIncRef(&jobs_done);
            }
        } else {
            SDL_SemWait(semaphore);
        }
    }
    return 0;
}

void job_system_initialize(void) {
    semaphore = SDL_CreateSemaphore(0);

    for(u32 thread_index = 0; thread_index < MAX_WORKER_THREADS; ++thread_index) {
        SDL_Thread **thread = thread_pool + thread_index;
        *thread             = SDL_CreateThread(thread_do_jobs, NULL, NULL);
    }
}

void job_system_terminate(void) {
    SDL_DestroySemaphore(semaphore);
}

// ----------------------------------------------------------------------
