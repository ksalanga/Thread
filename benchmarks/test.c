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
int i = 870;

int main(int argc, char **argv)
{
	void *ret;

	worker_t t1;
	worker_create(&t1, NULL, &foo, NULL);

	worker_join(t1, &ret);

	printf("thread exited with '%s'\n", ret);

	// int stack_i = 0;
	// while (1)
	// {
	// 	if (stack_i % 19 == 0 && stack_i % 24 == 0 && stack_i % 37 == 0 && stack_i % 105 == 0) // && i % 2049 == 0
	// 		fprintf(stdout, "main: %d\n", stack_i);
	// 	stack_i++;
	// }
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

struct Queue *setupQueue()
{
	struct Queue *q = createQueue(100);

	struct TCB *tcb1 = (struct TCB *)malloc(sizeof(struct TCB));
	tcb1->id = i;
	tcb1->status = 0;
	tcb1->priority = 0;
	ucontext_t cctx, cctx2;

	void *stack = malloc(i);
	/* Setup context that we are going to use */
	cctx.uc_link = NULL;
	cctx.uc_stack.ss_sp = stack;
	cctx.uc_stack.ss_size = i;
	cctx.uc_stack.ss_flags = 0;

	tcb1->t_ctxt = &cctx;
	printf("Setup context: %x\n", cctx);
	printf("TCB context: %x\n", tcb1->t_ctxt);
	enqueue(q, tcb1);

	i++;

	return q;
}

void checkCtxt(struct Queue *q)
{
	struct TCB *tcbTwo = dequeue(q);
	printf("%x\n", tcbTwo->t_ctxt);
	printf("Id: %d\n", tcbTwo->id);
}