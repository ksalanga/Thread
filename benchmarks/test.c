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
	worker_t t1;
	worker_t t2;
	worker_create(&t1, NULL, &foo, NULL);
	worker_create(&t2, NULL, &bar, NULL);
	puts("Good evening");
	return 0;
}

void *foo(void *arg) {
	for (int i = 0; i < 10; i++) {	
		puts("foo");
	}
}

void *bar(void *arg) {
	for (int i = 0; i < 10; i++) {	
		puts("bar");
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