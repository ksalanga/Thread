// File:	worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "worker.h"

#define STACK_SIZE SIGSTKSZ

struct TCB *currTCB;
ucontext_t *sched_ctx;
struct Queue *runqueue;
worker_t t_id = 0;

struct itimerval it_val; /* for setting itimer */
suseconds_t time_elapsed_usec = 0;
suseconds_t total_turnaround_time_usec = 0;
suseconds_t total_response_time_usec = 0;

#define INTERVAL 10 /* milliseconds */
#define INTERVAL_USEC (INTERVAL * 1000) % 1000000
#define INTERVAL_SEC INTERVAL / 1000

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
		// Create Signal for Timer
		struct sigaction sa;

		sa.sa_handler = schedule;
		sa.sa_flags = 0;
		sigfillset(&sa.sa_mask);
		sigdelset(&sa.sa_mask, SIGPROF);

		if (sigaction(SIGPROF, &sa, NULL) == -1)
		{
			printf("Unable to catch SIGALRM");
			exit(1);
		}

		// Initialize scheduler context
		sched_ctx = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));
		void *sched_ctx_stack = malloc(STACK_SIZE);
		sched_ctx->uc_link = NULL;
		sched_ctx->uc_stack.ss_sp = sched_ctx_stack;
		sched_ctx->uc_stack.ss_size = STACK_SIZE;
		sched_ctx->uc_stack.ss_flags = 0;
		getcontext(sched_ctx);
		makecontext(sched_ctx, schedule, 0);

		runqueue = createQueue();
	}

	// Create Main/Caller Thread Context
	if (currTCB == NULL)
	{
		struct TCB *main_tcb = (struct TCB *)malloc(sizeof(struct TCB));
		main_tcb->id = t_id;
		t_id++;
		main_tcb->status = READY;
		main_tcb->priority = 0;
		main_tcb->t_ctxt = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));

		void *main_stack = malloc(STACK_SIZE);

		if (main_stack == NULL)
		{
			perror("Failed to allocate main_stack");
			exit(1);
		}

		// TODO:
		// Temporarily make uc_link to null so that we can test if this stuff actually works.
		// In actual implementation, make uc_link = sched_ctx
		// main_tcb->t_ctxt->uc_link = sched_ctx;
		main_tcb->t_ctxt->uc_link = NULL;
		main_tcb->t_ctxt->uc_stack.ss_sp = main_stack;
		main_tcb->t_ctxt->uc_stack.ss_size = STACK_SIZE;
		main_tcb->t_ctxt->uc_stack.ss_flags = 0;
		currTCB = main_tcb;
	}

	struct TCB *worker_tcb = (struct TCB *)malloc(sizeof(struct TCB));
	worker_tcb->id = t_id;
	t_id++;
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

	// Temporarily Yield to the Scheduler Ctxt
	currTCB->yield = 1;

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

	// TODO:

#ifndef MLFQ
	sched_rr();
#else
	sched_mlfq();
#endif
}

/* Round-robin (RR) scheduling algorithm */
static void sched_rr()
{
	// - your own implementation of RR
	// (feel free to modify arguments and return types)

	// Records time
	getitimer(ITIMER_PROF, &it_val);

	// Might give a negative or overflow value because it_value w/ sigtimerprof ADDS extra time to the interval.
	suseconds_t thread_quantum_runtime_usec = INTERVAL_USEC - it_val.it_value.tv_usec;

	// If remaining time greater than 0, quantum did not expire.
	int quantum_expired = it_val.it_value.tv_usec > 0 ? 0 : 1;

	// Stops timer
	it_val.it_value.tv_sec = 0;
	it_val.it_value.tv_usec = 0;
	if (setitimer(ITIMER_PROF, &it_val, NULL) == -1)
	{
		printf("error calling setitimer()");
		exit(1);
	}

	time_elapsed_usec += thread_quantum_runtime_usec;

	if (!quantum_expired && !currTCB->yield)
	{
		worker_exit(NULL);
		total_turnaround_time_usec += (time_elapsed_usec - currTCB->arrival_time_usec);
	}
	else
	{
		currTCB->yield = 0;
		enqueue(runqueue, currTCB);
	}

	currTCB = dequeue(runqueue);

	if (currTCB != NULL)
	{
		if (currTCB->quanta == 0)
		{
			total_response_time_usec = time_elapsed_usec - currTCB->arrival_time_usec;
		}
	}
	else
	{
		// empty runqueue
		// check if blockqueue is empty as well, then stop process.
		// with MLFQ might want to check other priority levels as well.
	}

	it_val.it_value.tv_sec = INTERVAL_SEC;
	it_val.it_value.tv_usec = INTERVAL_USEC;
	it_val.it_interval = it_val.it_value;
	if (setitimer(ITIMER_PROF, &it_val, NULL) == -1)
	{
		printf("error calling setitimer()");
		exit(1);
	}
	setcontext(currTCB->t_ctxt);
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq()
{
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

struct QNode *newNode(tcb *tcb)
{
	struct QNode *temp = (struct QNode *)malloc(sizeof(struct QNode));
	temp->tcb = tcb;
	temp->next = NULL;
	return temp;
}

struct Queue *createQueue()
{
	struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
	q->front = q->rear = NULL;
	return q;
}

void enqueue(struct Queue *q, tcb *tcb)
{
	struct QNode *temp = newNode(tcb);

	if (q->rear == NULL)
	{
		q->front = q->rear = temp;
		return;
	}

	// Add the new node at the end of queue and change rear
	q->rear->next = temp;
	q->rear = temp;
}

tcb *dequeue(struct Queue *q)
{
	if (q->front == NULL)
		return NULL;

	struct QNode *node = q->front;
	tcb *tcb = node->tcb;

	q->front = q->front->next;

	if (q->front == NULL)
		q->rear = NULL;

	free(node);
	return tcb;
}
