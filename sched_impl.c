//Joaquin Garcia

#include "scheduler.h"
#include "sched_impl.h"
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <assert.h>

// we have 3 the first one to controll the amount of threads in the queua at the time
// second one will be like out mutex witch will only allow one thread at a time
// Last one is like the control but it make sure the queue is empty
sem_t controlSem;
sem_t cpuSem;
sem_t emptySem;


/* Fill in your scheduler implementation code below: */
//First will implemnet the threat portion of the code
static void init_thread_info(thread_info_t *info, sched_queue_t *queue)
{
	/*...Code goes here...*/
	info->queue = queue->list;
    info->queueData = NULL;
}

static void destroy_thread_info(thread_info_t *info)
{
	/*...Code goes here...*/
	 free(info->queueData);
}

/*...More functions go here...*/

// This code will block the thread until it can enter the schedualer queue
static void enter_sched_queue(thread_info_t *info)
{
        sem_wait(&controlSem);
        info->queueData = (list_elem_t*)malloc(sizeof(list_elem_t));
        list_elem_init(info->queueData, (void*)info);
        list_insert_tail(info->queue, info->queueData);
		//list was previously empty notify wait_for_queue
        if(list_size(info->queue) == 1)
		{
                sem_post(&emptySem);
		}
        sem_init(&info->runWorker,0,0);
}
//removes the thread from the scheduler queue
static void leave_sched_queue(thread_info_t *info)
{
        list_remove_elem(info->queue, info->queueData);
        sem_post(&controlSem);
}
// when it's in the process of the scheduler queue it will block until the thread is in the schedual
static void wait_for_cpu(thread_info_t *info)
{
        sem_wait(&info->runWorker);
}
//it will stop using the CPU when the thread timeslice is over
static void release_cpu(thread_info_t *info)
{
        sem_post(&cpuSem);
        sched_yield();
}
//Know we will be worrking on the Sched part of code
static void init_sched_queue(sched_queue_t *queue, int queue_size)
{
	/*...Code goes here...*/
	if (queue_size <= 0) 
	{
                exit(-1);
    }
        queue->currentWorker = NULL;
        queue->nextWorker = NULL;
        queue->list = (list_t*) malloc(sizeof(list_t));
        list_init(queue->list);
        sem_init(&controlSem, 0, queue_size);
        sem_init(&cpuSem,0,0);
        sem_init(&emptySem,0,0);

}
//release anything in the sched_queue_t
static void destroy_sched_queue(sched_queue_t *queue)
{
	/*...Code goes here...*/
	 list_elem_t * temp;
        while ((temp = list_get_head(queue->list)) != NULL) 
		{
                list_remove_elem(queue->list, temp);
                free(temp);
        }
        free(queue->list);

}

/*...More functions go here...*/
// allow the thread to execute
static void wake_up_worker(thread_info_t *info)
{
        sem_post(&info->runWorker);
}

/* Block until the current worker thread relinquishes the CPU. */
static void wait_for_worker(sched_queue_t *queue)
{
        sem_wait(&cpuSem);
}

/* Select the next worker thread to execute in round-robin scheduling
 * Returns NULL if the scheduler queue is empty. */
static thread_info_t * next_worker_rr(sched_queue_t *queue)
{
        if(list_size(queue->list) == 0) {
                return NULL;
        }

        if(queue->currentWorker == NULL) {//queue was just empty and now has an item in it
                queue->currentWorker = list_get_head(queue->list);
        } else if (queue->nextWorker == NULL) {//the last currentWorker was the tail of the queue
                if (queue->currentWorker == list_get_tail(queue->list)) {//the previous working thread is still in the queue and is the tail
                        queue->currentWorker = list_get_head(queue->list);
                } else {
                        queue->currentWorker = list_get_tail(queue->list); //collect the new tail
                }
        } else {//next worker is a member of the list
                queue->currentWorker = queue->nextWorker;
        }

        queue->nextWorker = queue->currentWorker->next;
        return (thread_info_t*) queue->currentWorker->datum;
}

/* Select the next worker thread to execute in FIFO scheduling
 * Returns NULL if the scheduler queue is empty. */
static thread_info_t * next_worker_fifo(sched_queue_t *queue) {
        if(list_size(queue->list) == 0) {
                return NULL;
        }
        else {
                return (thread_info_t*) (list_get_head(queue->list))->datum;
        }
}

/* Block until at least one worker thread is in the scheduler queue. */
static void wait_for_queue(sched_queue_t *queue)
{
        sem_wait(&emptySem);
}



/* You need to statically initialize these structures: */
sched_impl_t sched_fifo = {
	{ init_thread_info, destroy_thread_info /*, ...etc... */ }, 
	{ init_sched_queue, destroy_sched_queue /*, ...etc... */ } },
sched_rr = {
	{ init_thread_info, destroy_thread_info /*, ...etc... */ }, 
	{ init_sched_queue, destroy_sched_queue /*, ...etc... */ } };
