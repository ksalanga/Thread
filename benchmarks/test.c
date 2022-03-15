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
int i = 870;

int main(int argc, char **argv)
{
	worker_t t;
	worker_create(&t, NULL, &foo, NULL);
	printf("Good evening");
	return 0;
}

void *foo(void *arg) {
	printf("I did something\n");
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