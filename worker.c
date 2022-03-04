// File:	worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "worker.h"
// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

/* create a new thread */
int worker_create(worker_t *thread, pthread_attr_t *attr,
				  void *(*function)(void *), void *arg)
{

	// - create Thread Control Block (TCB)
	// - create and initialize the context of this worker thread
	// - allocate space of stack for this thread to run
	// after everything is set, push this thread into run queue and
	// - make it ready for the execution.

	// YOUR CODE HERE

	return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield()
{

	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE

	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr){
	// - de-allocate any dynamic memory created when starting this thread

	// YOUR CODE HERE
};

/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr)
{

	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex,
					  const pthread_mutexattr_t *mutexattr)
{
	//- initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex)
{

	// - use the built-in test-and-set atomic function to test the mutex
	// - if the mutex is acquired successfully, enter the critical section
	// - if acquiring mutex fails, push current thread into block list and
	// context switch to the scheduler thread

	// YOUR CODE HERE
	return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex)
{
	// - release mutex and make it available again.
	// - put threads in block list to run queue
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex)
{
	// - de-allocate dynamic memory created in worker_mutex_init

	return 0;
};

/* scheduler */
static void schedule()
{
	// - every time a timer interrupt occurs, your worker thread library
	// should be contexted switched from a thread context to this
	// schedule() function

	// - invoke scheduling algorithms according to the policy (RR or MLFQ)

	// if (sched == RR)
	//		sched_rr();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// - schedule policy
#ifndef MLFQ
	// Choose RR
#else
	// Choose MLFQ
#endif
}

/* Round-robin (RR) scheduling algorithm */
static void sched_rr()
{
	// - your own implementation of RR
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq()
{
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

struct Queue *createQueue(unsigned capacity)
{
	struct Queue *queue = (struct Queue *)malloc(
		sizeof(struct Queue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;

	// This is important, see the enqueue
	queue->rear = capacity - 1;
	queue->array = (tcb **)malloc(
		queue->capacity * sizeof(tcb *));
	return queue;
}

void resizeQueue(struct Queue *queue)
{
	queue->capacity += 100;
	queue->array = (tcb **)realloc(&queue->array, queue->capacity * sizeof(tcb *));
}

int isFull(struct Queue *queue)
{
	return (queue->size == queue->capacity);
}

int isEmpty(struct Queue *queue)
{
	return queue->size == 0;
}

void enqueue(struct Queue *queue, tcb *item)
{
	if (isFull(queue))
	{
		resizeQueue(queue);
		return;
	}
	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->array[queue->rear] = item;
	queue->size = queue->size + 1;
}

tcb *dequeue(struct Queue *queue)
{
	if (isEmpty(queue))
		return NULL;
	tcb *item = queue->array[queue->front];
	queue->front = (queue->front + 1) % queue->capacity;
	queue->size = queue->size - 1;
	return item;
}

tcb *front(struct Queue *queue)
{
	if (isEmpty(queue))
		return NULL;
	return queue->array[queue->front];
}

tcb *rear(struct Queue *queue)
{
	if (isEmpty(queue))
		return NULL;
	return queue->array[queue->rear];
}
