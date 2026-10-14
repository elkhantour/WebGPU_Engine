#ifndef _JOB_SYSTEM_H_
#define _JOB_SYSTEM_H_

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef void (*JobWorkFn)(void *data); // runs on a worker thread
typedef void (*JobDoneFn)(void *data); // runs on the main thread (may be NULL)

typedef struct Job {
  JobWorkFn work;
  JobDoneFn done;
  void *data;
  struct Job *next;
} Job;

typedef struct {
  Job *head, *tail;
} JobQueue;

typedef struct {
  pthread_t *threads;
  int thread_count;

  JobQueue pending;
  pthread_mutex_t pending_lock;
  pthread_cond_t pending_cv;

  JobQueue completed;
  pthread_mutex_t completed_lock;

  atomic_bool running;
  atomic_int in_flight;
} JobSystem;

void job_system_init(JobSystem *js, int thread_count); // 0 = auto
void job_system_destroy(JobSystem *js);
void job_system_submit(JobSystem *js, JobWorkFn work, JobDoneFn done,
                       void *data);
void job_system_draw_callback(void *data); // main thread, call per frame
void job_system_wait_idle(JobSystem *js);  // e.g. loading screen / shutdown

#endif
