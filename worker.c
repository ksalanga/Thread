// File:	worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "worker.h"

#define STACK_SIZE SIGSTKSZ

struct TCB *currTCB;
ucontext_t *sched_ctx;
struct Queue *mlfqrunqueue[4]; // 0 IS TOP LEVEL, 3 IS BOTTOM
struct Queue *runqueue;
struct Queue *exitqueue;
worker_t t_id = 0;
int currPriority = 0; // might not be needed
int total_threads = 0;
int finished_threads = 0;

struct itimerval it_val;	/* for setting itimer */
struct itimerval reset_val; // reset timer for mlfq
suseconds_t total_turnaround_time_usec = 0;
suseconds_t total_response_time_usec = 0;

suseconds_t S = 0;

#define INTERVAL 10 /* milliseconds */
#define INTERVAL_USEC(i) (i * 1000) % 1000000
#define INTERVAL_SEC(i) i / 1000

#define r_INTERVAL 50 /* milliseconds */
#define r_INTERVAL_USEC (r_INTERVAL * 1000) % 1000000
#define r_INTERVAL_SEC r_INTERVAL / 1000

#define LEVELS 4
#define PRIORITY_0 10
#define PRIORITY_1 20
#define PRIORITY_2 30
#define PRIORITY_3 40

int priority_intervals[4] = {PRIORITY_0, PRIORITY_1, PRIORITY_2, PRIORITY_3};

#define min(X, Y) ((X) < (Y) ? (X) : (Y))

// TODO: delete later
static void printqueue();

/* create a new thread */
int worker_create(worker_t *thread, pthread_attr_t *attr,
				  void *(*function)(void *), void *arg)
{
	// - create Thread Control Block (TCB)
	// - create and initialize the context of this worker thread
	// - allocate space of stack for this thread to run
	// after everything is set, push this thread into run queue and
	// - make it ready for the execution.

	if (sched_ctx == NULL) // first thread creation
	{
		struct sigaction sa;

		sa.sa_handler = handler;
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

#ifndef MLFQ
		runqueue = createQueue();
#else
		for (int i = 0; i < LEVELS; i++)
		{
			mlfqrunqueue[i] = createQueue();
		}
#endif

		exitqueue = createQueue();

		// Run timer
		it_val.it_value.tv_sec = INTERVAL_SEC(INTERVAL);
		it_val.it_value.tv_usec = INTERVAL_USEC(INTERVAL);
		it_val.it_interval.tv_sec = 0;
		it_val.it_interval.tv_usec = 0;
		if (setitimer(ITIMER_PROF, &it_val, NULL) == -1)
		{
			printf("error calling setitimer()");
			exit(1);
		}

		// Create Main/Caller Thread Context
		struct TCB *main_tcb = (struct TCB *)malloc(sizeof(struct TCB));
		main_tcb->id = t_id;
		t_id++;
		main_tcb->status = RUNNING;
		main_tcb->priority = 0;
		main_tcb->quanta = 0;
		main_tcb->t_ctxt = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));

		void *main_stack = malloc(STACK_SIZE);

		if (main_stack == NULL)
		{
			perror("Failed to allocate main_stack");
			exit(1);
		}

		main_tcb->t_ctxt->uc_link = NULL;
		main_tcb->t_ctxt->uc_stack.ss_sp = main_stack;
		main_tcb->t_ctxt->uc_stack.ss_size = STACK_SIZE;
		main_tcb->t_ctxt->uc_stack.ss_flags = 0;

		// Record Arrival Time of Caller
		struct timeval caller_arrival_time;
		gettimeofday(&caller_arrival_time, NULL);
		main_tcb->arrival_time = caller_arrival_time;

		currTCB = main_tcb;
	}

	struct TCB *worker_tcb = (struct TCB *)malloc(sizeof(struct TCB));
	worker_tcb->id = t_id;
	*thread = t_id;
	t_id++;
	total_threads++;
	worker_tcb->status = READY;
	worker_tcb->priority = 0;
	worker_tcb->quanta = 0;
	worker_tcb->t_ctxt = (struct ucontext_t *)malloc(sizeof(struct ucontext_t));

	if (getcontext(worker_tcb->t_ctxt) < 0)
	{
		perror("getcontext");
		exit(1);
	}

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

	// Record Arrival Time of Worker
	struct timeval worker_arrival_time;
	gettimeofday(&worker_arrival_time, NULL);
	worker_tcb->arrival_time = worker_arrival_time;

	// Create Worker Thread Context
	makecontext(worker_tcb->t_ctxt, (void *)function, 1, arg);

#ifndef MLFQ
	enqueue(runqueue, worker_tcb);
#else
	enqueue(mlfqrunqueue[0], worker_tcb);
#endif

	return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield()
{

	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context
	currTCB->yield = 1;
	swapcontext(currTCB->t_ctxt, sched_ctx);

	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr)
{
	// - de-allocate any dynamic memory created when starting this thread
	// Modify Value ptr

	sigset_t set;
	blockSignalProf(&set);

	free(currTCB->t_ctxt->uc_stack.ss_sp);
	free(currTCB->t_ctxt);
	currTCB->status = EXIT;
	currTCB->value_ptr = value_ptr;
	enqueue(exitqueue, currTCB);

	struct timeval finished_time;
	gettimeofday(&finished_time, NULL);
	total_turnaround_time_usec += ((finished_time.tv_sec * 1000000 + finished_time.tv_usec) -
								   (currTCB->arrival_time.tv_sec * 1000000 + currTCB->arrival_time.tv_usec));

	currTCB = NULL;

	finished_threads++;

	if (finished_threads == total_threads)
	{
		// Calculate Average Response and Turnaround Time
		long double total_response_time_ms = (long double)total_response_time_usec / 1000;
		long double total_turnaround_time_ms = (long double)total_turnaround_time_usec / 1000;

		long double average_response_time_ms = (long double)total_response_time_ms / total_threads;
		long double average_turnaround_time_ms = (long double)total_turnaround_time_ms / total_threads;
		fprintf(stdout, "Average Response Time: %Lf ms\n", average_response_time_ms);
		fprintf(stdout, "Average Turnaround Time: %Lf ms\n", average_turnaround_time_ms);
	}

	unblockSignalProf(&set);
	setcontext(sched_ctx);
};

/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr)
{
	// find worker_t thread.
	struct TCB *wait_on_thread = NULL;

	sigset_t set;
	blockSignalProf(&set);

	struct QNode *q_ptr;

#ifndef MLFQ
	q_ptr = runqueue->front;
	while (q_ptr != NULL && q_ptr->tcb->id != thread)
	{
		q_ptr = q_ptr->next;
	}
	if (q_ptr == NULL)
	{
		q_ptr = exitqueue->front;
		while (q_ptr != NULL && q_ptr->tcb->id != thread)
		{
			q_ptr = q_ptr->next;
		}
	}
#else
	for (int i = 0; i < 4; i++)
	{
		q_ptr = mlfqrunqueue[i]->front;
		while (q_ptr != NULL && q_ptr->tcb->id != thread)
		{
			q_ptr = q_ptr->next;
		}
		if (q_ptr != NULL)
		{
			break;
		}
	}
	if (q_ptr == NULL)
	{
		q_ptr = exitqueue->front;
		while (q_ptr != NULL && q_ptr->tcb->id != thread)
		{
			q_ptr = q_ptr->next;
		}
	}
#endif

	if (q_ptr != NULL)
	{
		wait_on_thread = q_ptr->tcb;
	}

	unblockSignalProf(&set);

	if (wait_on_thread != NULL)
	{
		while (wait_on_thread->status != EXIT)
		{
			worker_yield();
		}
		if (value_ptr != NULL)
			*value_ptr = wait_on_thread->value_ptr;
	}
	// - de-allocate any dynamic memory created by the joining thread
	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex,
					  const pthread_mutexattr_t *mutexattr)
{
	//- initialize data structures for this mutex

	sigset_t set;
	blockSignalProf(&set);

	mutex->lock = UNLOCKED;
	mutex->blocked_queue = createQueue();

	unblockSignalProf(&set);
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex)
{

	// - use the built-in test-and-set atomic function to test the mutex
	// - if the mutex is acquired successfully, enter the critical section
	// - if acquiring mutex fails, push current thread into block list and
	// context switch to the scheduler thread

	sigset_t set;
	blockSignalProf(&set);
	if (mutex->lock == UNLOCKED)
	{
		mutex->lock = LOCKED;
	}
	else if (mutex->lock == LOCKED)
	{

		currTCB->status = BLOCKED;
		enqueue(mutex->blocked_queue, currTCB);

		unblockSignalProf(&set);
		swapcontext(currTCB->t_ctxt, sched_ctx);
	}
	unblockSignalProf(&set);

	return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex)
{
	// - release mutex and make it available again.
	// - put threads in block list to run queue
	// so that they could compete for mutex later.

	// dequeue each thread from blocked queue and allow each thread to access variable in mutex

	sigset_t set;
	blockSignalProf(&set);

	if (mutex->lock == LOCKED)
	{
		mutex->lock = UNLOCKED;

		struct TCB *unblocked_thread = dequeue(mutex->blocked_queue);
		if (unblocked_thread != NULL)
		{
			unblocked_thread->status = READY;

#ifndef MLFQ
			enqueue(runqueue, unblocked_thread);
#else
			enqueue(mlfqrunqueue[unblocked_thread->priority], unblocked_thread);
#endif
		}
	}
	unblockSignalProf(&set);

	return 0;
};

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex)
{
	// - de-allocate dynamic memory created in worker_mutex_init
	struct QNode *q_ptr = mutex->blocked_queue->front;
	struct QNode *prev;

	while (q_ptr != NULL)
	{
		prev = q_ptr;
		q_ptr = q_ptr->next;
		free(prev);
	}

	free(mutex->blocked_queue);
	return 0;
};

static void handler()
{
	if (currTCB != NULL)
		swapcontext(currTCB->t_ctxt, sched_ctx);
	else
		setcontext(sched_ctx);
}

/* scheduler */
static void schedule()
{

#ifndef MLFQ
	sched_rr(INTERVAL);
#else
	sched_mlfq();
#endif
}

/* Round-robin (RR) scheduling algorithm */
static void sched_rr(int interval_ms)
{
	getitimer(ITIMER_PROF, &it_val);
	int quantum_expired = it_val.it_value.tv_usec > 0 ? 0 : 1;

#if MLFQ
	struct itimerval remaining_time = it_val;
#endif

	stoptimer();

	if (currTCB != NULL && currTCB->status != BLOCKED)
	{
		if (!quantum_expired && !currTCB->yield) // Thread has finished, not requeued.
		{
			worker_exit(NULL);
		}
		else // Thread has more code to run either through Time Quantum Elapse or Yield
		{

			currTCB->yield = 0;
#ifndef MLFQ
			enqueue(runqueue, currTCB);
#else
			if (quantum_expired)
			{
				currTCB->priority = min(currTCB->priority + 1, 3);
			}
			enqueue(mlfqrunqueue[currTCB->priority], currTCB);
#endif
		}
	}

	currTCB = dequeue(runqueue);

	if (currTCB != NULL)
	{
		currTCB->status = RUNNING;
		if (currTCB->quanta == 0)
		{
			struct timeval initial_schedule_time;
			gettimeofday(&initial_schedule_time, NULL);
			total_response_time_usec = ((initial_schedule_time.tv_sec * 1000000 + initial_schedule_time.tv_usec) -
										(currTCB->arrival_time.tv_sec * 1000000 + currTCB->arrival_time.tv_usec));
		}

		// Thread has ran for one more quanta
		currTCB->quanta++;

#if MLFQ
		// If S has elapsed, put all threads to the top of the priority queue
		S += INTERVAL_USEC(interval_ms) - (remaining_time.it_value.tv_sec * 1000000 + remaining_time.it_value.tv_usec);
		if (S >= r_INTERVAL_USEC)
		{
			// fprintf(stdout, "Resetting Priorities\n");
			for (int i = 1; i < LEVELS; i++)
			{
				if (mlfqrunqueue[i]->front != NULL)
				{
					if (mlfqrunqueue[0]->front != NULL)
					{
						mlfqrunqueue[0]->rear->next = mlfqrunqueue[i]->front;
						mlfqrunqueue[0]->rear = mlfqrunqueue[i]->rear;
					}
					else
					{
						mlfqrunqueue[0]->front = mlfqrunqueue[i]->front;
						mlfqrunqueue[0]->rear = mlfqrunqueue[i]->rear;
					}
					mlfqrunqueue[i]->front = NULL;
					mlfqrunqueue[i]->rear = NULL;
				}
			}
			S = 0;
		}
#endif

		runtimer(interval_ms);

		setcontext(currTCB->t_ctxt);
	}
	else
	{
#if MLFQ
		for (int i = 0; i < LEVELS; i++)
		{
			struct QNode *q = mlfqrunqueue[i]->front;
			if (q != NULL)
			{
				currTCB = dequeue(mlfqrunqueue[i]);
				runtimer(priority_intervals[i]);
				setcontext(currTCB->t_ctxt);
				break;
			}
		}
#endif
	}
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq()
{
	int priority_level = 0;
	// Rule 1
	for (int i = 0; i < LEVELS; i++)
	{
		if (mlfqrunqueue[i]->front != NULL)
		{
			runqueue = mlfqrunqueue[i];
			priority_level = i;
			break;
		}
	}

	sched_rr(priority_intervals[priority_level]);
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
	if (tcb == NULL)
		return;

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

static void blockSignalProf(sigset_t *set)
{
	sigemptyset(set);
	sigaddset(set, SIGPROF);
	sigprocmask(SIG_BLOCK, set, NULL);
}
static void unblockSignalProf(sigset_t *set)
{
	sigprocmask(SIG_UNBLOCK, set, NULL);
}

static void stoptimer()
{
	// Stops timer
	it_val.it_value.tv_sec = 0;
	it_val.it_value.tv_usec = 0;
	if (setitimer(ITIMER_PROF, &it_val, NULL) == -1)
	{
		printf("error calling setitimer()");
		exit(1);
	}
}

static void runtimer(int interval_ms)
{
	it_val.it_value.tv_sec = INTERVAL_SEC(interval_ms);
	it_val.it_value.tv_usec = INTERVAL_USEC(interval_ms);
	it_val.it_interval.tv_sec = 0;
	it_val.it_interval.tv_usec = 0;
	if (setitimer(ITIMER_PROF, &it_val, NULL) == -1)
	{
		printf("error calling setitimer()");
		exit(1);
	}
}

// TODO: delete later
static void printqueue()
{
#if MLFQ
	fprintf(stdout, "********************\n");
#endif
	if (currTCB != NULL)
	{
		fprintf(stdout, "|%d|  ", currTCB->id);
	}
	else
	{
		fprintf(stdout, "| |  ");
	}

#ifndef MLFQ
	struct QNode *q = runqueue->front;
	while (q != NULL)
	{
		fprintf(stdout, "%d->", q->tcb->id);
		q = q->next;
	}
	fprintf(stdout, "/\n");
#else
	fprintf(stdout, "\n");
	for (int i = 0; i < LEVELS; i++)
	{
		fprintf(stdout, "%d: ", i);
		struct QNode *q;
		if (mlfqrunqueue[i]->front != NULL)
		{
			q = mlfqrunqueue[i]->front;
			while (q != NULL)
			{
				fprintf(stdout, "%d->", q->tcb->id);
				q = q->next;
			}
		}
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "********************\n\n");
#endif
}