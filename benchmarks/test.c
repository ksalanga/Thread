#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../worker.h"

/* A scratch program template on which to call and
 * test worker library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

void checkCtxt(struct Queue *q);
struct Queue *setupQueue();
void *foo(void *arg);
void *bar(void *arg);
void *func_foo(void *arg);
void *func_bar(void *arg);
void *trythis(void *arg);
void *trythislock(void *arg);
long long counter = 0;
worker_mutex_t lock;

int main(int argc, char **argv)
{
	void *ret1, *ret2;

	worker_t t1;
	worker_t t2;

	// worker_mutex_init(&lock, NULL);
	worker_create(&t1, NULL, &func_bar, NULL);
	worker_create(&t2, NULL, &func_foo, NULL);

	worker_join(t1, &ret1);
	worker_join(t2, &ret2);

	printf("Thread 1 exited with '%s'\n", (char *)ret1);
	printf("Thread 2 exited with '%s'\n", (char *)ret2);
}

void *trythis(void *arg)
{
	unsigned long i = 0;
	counter += 1;
	printf("\n Job %d has started\n", counter);

	for (i = 0; i < (0xFFFFFFFF); i++)
		;
	printf("\n Job %d has finished\n", counter);

	return NULL;
}

void *trythislock(void *arg)
{
	worker_mutex_lock(&lock);
	unsigned long i = 0;
	counter += 1;
	printf("\n Job %d has started\n", counter);

	for (i = 0; i < (0xFFFFFFFF); i++)
		;
	printf("\n Job %d has finished\n", counter);

	worker_mutex_unlock(&lock);

	return NULL;
}

void *foo(void *arg)
{
	int i = 0;
	int counter = 0;

	for (; i < 10; i++)
	{
		fprintf(stdout, "hey\n");
	}

	char *ret;

	if ((ret = (char *)malloc(23)) == NULL)
	{
		perror("malloc() error");
		exit(2);
	}
	strcpy(ret, "I am a programmer! :)");
	worker_exit(ret);
	// while (1)
	// {
	// 	if (i % 19 == 0 && i % 24 == 0 && i % 37 == 0 && i % 105 == 0) // && i % 2049 == 0
	// 	{
	// 		fprintf(stdout, "f:%d, %d\n", counter, i);
	// 		counter++;
	// 	}
	// 	i++;
	// }
}

void *bar(void *arg)
{
	int i = 0;
	int counter = 0;

	while (1)
	{
		if (i % 19 == 0 && i % 24 == 0 && i % 37 == 0 && i % 105 == 0) // && i % 2049 == 0
		{
			fprintf(stdout, "b:%d, %d\n", counter, i);
			counter++;
		}
		i++;
	}
}

void *func_foo(void *arg)
{
	// int thread_quasi_id = *((int *)arg);
	// fprintf(stdout, "Initialize Thread: %d\n", thread_quasi_id);

	// int stack_i = 0;
	// while (1)
	// {
	//     if (stack_i % 19 == 0 && stack_i % 24 == 0 && stack_i % 37 == 0 && stack_i % 105 == 0) // && i % 2049 == 0
	//         fprintf(stdout, "Thread %d: %d\n", thread_quasi_id, stack_i);
	//     stack_i++;
	// }

	for (int i = 0; i < 4; i++)
	{
		fprintf(stdout, "Thread 2\n");
	}

	char *ret;

	if ((ret = (char *)malloc(23)) == NULL)
	{
		perror("malloc() error");
		exit(2);
	}
	strcpy(ret, "Two has exited!");

	fprintf(stdout, "func_foo (two) closing..\n");
	worker_exit(ret);
	return NULL;
}

void *func_bar(void *arg)
{
	int i;
	for (i = 0; i < (0xFFFFFFFF); i++)
		;
	fprintf(stdout, "func_bar (one) closing...\n");

	char *ret;

	if ((ret = (char *)malloc(23)) == NULL)
	{
		perror("malloc() error");
		exit(2);
	}
	strcpy(ret, "One has exited!");
	worker_exit(ret);
}

struct Queue *setupQueue()
{
	struct Queue *q = createQueue(100);

	struct TCB *tcb1 = (struct TCB *)malloc(sizeof(struct TCB));
	tcb1->id = 0;
	tcb1->status = 0;
	tcb1->priority = 0;
	ucontext_t cctx, cctx2;

	void *stack = malloc(200);
	/* Setup context that we are going to use */
	cctx.uc_link = NULL;
	cctx.uc_stack.ss_sp = stack;
	cctx.uc_stack.ss_size = 200;
	cctx.uc_stack.ss_flags = 0;

	tcb1->t_ctxt = &cctx;
	printf("Setup context: %x\n", cctx);
	printf("TCB context: %x\n", tcb1->t_ctxt);
	enqueue(q, tcb1);

	return q;
}

void checkCtxt(struct Queue *q)
{
	struct TCB *tcbTwo = dequeue(q);
	printf("%x\n", tcbTwo->t_ctxt);
	printf("Id: %d\n", tcbTwo->id);
}