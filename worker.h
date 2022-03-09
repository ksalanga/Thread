// File:	worker_t.h

// List all group member's name:
// username of iLab:
// iLab Server:

#ifndef WORKER_T_H
#define WORKER_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_WORKERS macro */
#define USE_WORKERS 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/time.h>

typedef uint worker_t;

enum status
{
	READY,
	RUNNING,
	BLOCKED
};

enum lock_status
{
	LOCKED,
	UNLOCKED
};

typedef struct TCB
{
	// thread Id
	worker_t id;

	// thread status
	enum status status;

	// thread context
	ucontext_t *t_ctxt;

	// thread priority for MLFQ
	int priority;
} tcb;

/* mutex struct definition */
typedef struct worker_mutex_t
{
	enum lock_status lock;
} worker_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// Queue of TCBs
typedef struct Queue
{
	int front, rear, size;
	unsigned capacity;
	tcb **array;
} queue;

struct Queue *createQueue(unsigned capacity);

void resizeQueue(struct Queue *queue);

int isFull(struct Queue *queue);

int isEmpty(struct Queue *queue);

void enqueue(struct Queue *queue, tcb *item);

tcb *dequeue(struct Queue *queue);

tcb *front(struct Queue *queue);

tcb *rear(struct Queue *queue);

// MLFQ
queue PriorityArray[4];

/* Function Declarations: */

/* create a new thread */
int worker_create(worker_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg);

/* give CPU pocession to other user level worker threads voluntarily */
int worker_yield();

/* terminate a thread */
void worker_exit(void *value_ptr);

/* wait for thread termination */
int worker_join(worker_t thread, void **value_ptr);

/* initial the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t
												 *mutexattr);

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex);

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex);

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex);

#ifdef USE_WORKERS
#define pthread_t worker_t
#define pthread_mutex_t worker_mutex_t
#define pthread_create worker_create
#define pthread_exit worker_exit
#define pthread_join worker_join
#define pthread_mutex_init worker_mutex_init
#define pthread_mutex_lock worker_mutex_lock
#define pthread_mutex_unlock worker_mutex_unlock
#define pthread_mutex_destroy worker_mutex_destroy
#endif

#endif
