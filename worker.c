// File:	worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "worker.h"

#define STACK_SIZE SIGSTKSZ
struct TCB *currTCB;
ucontext_t *sched_ctx;
struct Queue *runqueue;

#define INTERVAL 1000 /* milliseconds */
#define STACK_SIZE SIGSTKSZ

/* create a new thread */
int worker_create(worker_t *thread, pthread_attr_t *attr,
				  void *(*function)(void *), void *arg)
{

	// - create Thread Control Block (TCB)
	// - create and initialize the context of this worker thread
	// - allocate space of stack for this thread to run
	// after everything is set, push this thread into run queue and
	// - make it ready for the execution.

	if (sched_ctx == NULL)
	{
		sched_ctx = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));
		void *sched_ctx_stack = malloc(STACK_SIZE);
		sched_ctx->uc_link = NULL;
		sched_ctx->uc_stack.ss_sp = sched_ctx_stack;
		sched_ctx->uc_stack.ss_size = STACK_SIZE;
		sched_ctx->uc_stack.ss_flags = 0;
		getcontext(sched_ctx);
		makecontext(sched_ctx, schedule, 0);
		runqueue = createQueue(100);
	}

	struct TCB *worker_tcb = (struct TCB *)malloc(sizeof(struct TCB));
	worker_tcb->id = 0;
	worker_tcb->status = READY;
	worker_tcb->priority = 0;
	worker_tcb->t_ctxt = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));

	void *worker_stack = malloc(STACK_SIZE);

	if (worker_stack == NULL)
	{
		perror("Failed to allocate worker_stack");
		exit(1);
	}

	/* Setup context that we are going to use */
	worker_tcb->t_ctxt->uc_link = sched_ctx;
	worker_tcb->t_ctxt->uc_stack.ss_sp = worker_stack;
	worker_tcb->t_ctxt->uc_stack.ss_size = STACK_SIZE;
	worker_tcb->t_ctxt->uc_stack.ss_flags = 0;

	// TODO
	// if RR put into runqueue, if MLFQ, put into top priority queue

	// Create Worker Thread Context
	getcontext(worker_tcb->t_ctxt);
	makecontext(worker_tcb->t_ctxt, (void *)&function, 1, arg);
	enqueue(runqueue, worker_tcb);

	// Create Main/Caller Thread Context
	if (currTCB == NULL)
	{
		struct TCB *main_tcb = (struct TCB *)malloc(sizeof(struct TCB));
		main_tcb->id = 0;
		main_tcb->status = READY;
		main_tcb->priority = 0;
		main_tcb->t_ctxt = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));

		void *main_stack = malloc(STACK_SIZE);

		if (main_stack == NULL)
		{
			perror("Failed to allocate main_stack");
			exit(1);
		}

		main_tcb->t_ctxt->uc_link = sched_ctx;
		main_tcb->t_ctxt->uc_stack.ss_sp = main_stack;
		main_tcb->t_ctxt->uc_stack.ss_size = STACK_SIZE;
		main_tcb->t_ctxt->uc_stack.ss_flags = 0;
		enqueue(runqueue, main_tcb);
		currTCB = main_tcb;
	}

	swapcontext(currTCB->t_ctxt, sched_ctx);

	return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield()
{

	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	currTCB->status = READY;
	// free past thread context?
	swapcontext(currTCB->t_ctxt, sched_ctx);

	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr)
{
	// - de-allocate any dynamic memory created when starting this thread
	// Modify Value ptr

	free(currTCB->t_ctxt->uc_stack.ss_sp);
	free(currTCB->t_ctxt);
	free(currTCB);

	currTCB = NULL;

	// Because currTCB is null, when we go back to the runqueue,
	// the running thread that called worker exit has already been dequequed,
	// just check in the scheduler if currTCB is NULL, enqueue next thread.
	// effectively removing the thread
};

/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr)
{

	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread

	// TODO:
	// If thread is not in queue, resume
	// else, swapcontext to sched
	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex,
					  const pthread_mutexattr_t *mutexattr)
{
	//- initialize data structures for this mutex

	mutex = (worker_mutex_t *)(malloc(sizeof(struct worker_mutex_t)));
	mutex->lock = UNLOCKED;

	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex)
{

	// - use the built-in test-and-set atomic function to test the mutex
	// - if the mutex is acquired successfully, enter the critical section
	// - if acquiring mutex fails, push current thread into block list and
	// context switch to the scheduler thread

	if (mutex->lock == UNLOCKED)
	{
		mutex->lock = LOCKED;
		currTCB->status = RUNNING; // either running or ready
	}
	else if (mutex->lock == LOCKED)
	{
		currTCB->status = BLOCKED;
	}

	return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex)
{
	// - release mutex and make it available again.
	// - put threads in block list to run queue
	// so that they could compete for mutex later.

	if (mutex->lock == LOCKED)
	{
		mutex->lock = UNLOCKED;
	}

	return 0;
};

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex)
{
	// - de-allocate dynamic memory created in worker_mutex_init
	free(mutex);
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

	struct sigaction sa;
	sa.sa_handler = schedule;
	sa.sa_flags = 0;
	sigfillset(&sa.sa_mask);
	sigdelset(&sa.sa_mask, SIGPROF);

	struct itimerval it_val; /* for setting itimer */
	struct itimerval temp;	 // for resetting timer

	/* Upon SIGALRM, call DoStuff().
	 * Set interval timer.  We want frequency in ms,
	 * but the setitimer call needs seconds and useconds. */
	if (sigaction(SIGPROF, &sa, NULL) == -1)
	{
		printf("Unable to catch SIGALRM");
		exit(1);
	}

	it_val.it_value.tv_sec = INTERVAL / 1000;
	it_val.it_value.tv_usec = (INTERVAL * 1000) % 1000000;
	it_val.it_interval = it_val.it_value;

	temp.it_value.tv_sec = INTERVAL / 1000;
	temp.it_value.tv_usec = (INTERVAL * 1000) % 1000000;
	temp.it_interval = temp.it_value;
	if (setitimer(ITIMER_PROF, &temp, &it_val) == -1)
	{
		printf("error calling setitimer()");
		exit(1);
	}

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
