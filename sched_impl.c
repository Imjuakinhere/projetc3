//Joaquin Garcia

#include "scheduler.h"
#include "sched_impl.h"
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <assert.h>

// we have 3 the controlSem will control how many threads are in the queue while 
// cpuSem will only allow one thread to use the CPU witch we could say it's like our mutex
// The last emptySem will do the opposite of the first one but this time it makes sure it's not empty

sem_t controlSem; 
sem_t cpuSem; 
sem_t emptySem;

// Worker thread_operations
///////////////////////////////////////////////////////////////////////////
// we are going to initialize the thread_info 
// then we going to release the sources that are associted with thread_info 
// we will then block until we are able to enter the scheduler queue
// Then we will remove the thread from the scheduler queue 
// It will wait till the thread is schedualed
// Lastly the cpu will releas till it timeslice 

static void init_thread_info(thread_info_t *info, sched_queue_t *queue)
{
        info->queue = queue->list;
        info->queueData = NULL;
}
static void destroy_thread_info(thread_info_t *info)
{
        free(info->queueData);
}

static void enter_sched_queue(thread_info_t *info)
{
        sem_wait(&controlSem);
        info->queueData = (list_elem_t*)malloc(sizeof(list_elem_t));
        list_elem_init(info->queueData, (void*)info);
        list_insert_tail(info->queue, info->queueData);
        if(list_size(info->queue) == 1)
		{
			//list was previously empty notify wait_for_queue
            sem_post(&emptySem);
		}
        sem_init(&info->runWorker,0,0);
}
static void leave_sched_queue(thread_info_t *info)
{
        list_remove_elem(info->queue, info->queueData);
        sem_post(&controlSem);
}
static void wait_for_cpu(thread_info_t *info)
{
        sem_wait(&info->runWorker);
}
static void release_cpu(thread_info_t *info)
{
        sem_post(&cpuSem);
        sched_yield();
}
//Sched_operations
////////////////////////////////////////////////////////////////////////
// Initialize the sched_queue
// We will then release resources from sched_queue
// Then we would allow a worker thread to execute
// Block it once the worker thread realeases the CPU
// We will then execute the next working thread that will execute in a round robin schedualing (return NUll for empty)
// Then we will repeat the step before but instead we will be executing in FIFO scheduling (return NUll for empty)
// We will Block until at least one worker thread is in the schedualer queue.

// Don't forget Cworker is current worker and Nworker is the next worker
static void init_sched_queue(sched_queue_t *queue, int queue_size)
{
        if (queue_size <= 0)
		{
                exit(-1); //exit entire program if queue has a size of zero
        }
        queue->CWorker = NULL;
        queue->NWorker = NULL;
        queue->list = (list_t*) malloc(sizeof(list_t));
        list_init(queue->list);
        sem_init(&controlSem, 0, queue_size);
		//block on first call of wait_for_worker
        sem_init(&cpuSem,0,0);
		//block on first call of wait_for_queue
		sem_init(&emptySem,0,0);
}
static void destroy_sched_queue(sched_queue_t *queue)
{
        list_elem_t * temp;
        while ((temp = list_get_head(queue->list)) != NULL) 
		{
			//delete any remainign list elements
            list_remove_elem(queue->list, temp);
            free(temp);
        }
        free(queue->list);
}
static void wake_up_worker(thread_info_t *info)
{
        sem_post(&info->runWorker);
}
static void wait_for_worker(sched_queue_t *queue)
{
        sem_wait(&cpuSem);
}
static thread_info_t * next_worker_rr(sched_queue_t *queue)
{
        if(list_size(queue->list) == 0) 
		{
             return NULL;
        }
        if(queue->CWorker == NULL)
		{	//queue was empty and now has an item in it
            queue->CWorker = list_get_head(queue->list);
        } 
		else if (queue->NWorker == NULL) 
		{	//the last CWorker was the tail of the queue
            if (queue->CWorker == list_get_tail(queue->list)) 
			{
				//the previous working thread is still in the queue and is the tail
                 queue->CWorker = list_get_head(queue->list);
            } else 
			{	//collect the new tail
                queue->CWorker = list_get_tail(queue->list); 
            }
        } 
		else 
		{	//next worker is a member of the list
            queue->CWorker = queue->NWorker;
        }

        queue->NWorker = queue->CWorker->next;
        return (thread_info_t*) queue->CWorker->datum;
}
static thread_info_t * next_worker_fifo(sched_queue_t *queue) {
        if(list_size(queue->list) == 0) {
                return NULL;
        }
        else {
                return (thread_info_t*) (list_get_head(queue->list))->datum;
        }
}
static void wait_for_queue(sched_queue_t *queue)
{
        sem_wait(&emptySem);
}

// Including everything that was missing 
// sched_fifo
// enter_sched_queue,leave_sched_queue, wait_for_cpu, release_cpu}, 
// wake_up_worker, wait_for_worker, next_worker_fifo, wait_for_queue}
// sched_rr
// enter_sched_queue,leave_sched_queue, wait_for_cpu, release_cpu}, 
// wake_up_worker, wait_for_worker, next_worker_rr, wait_for_queue}

sched_impl_t sched_fifo = 
{
    { init_thread_info, destroy_thread_info, enter_sched_queue, leave_sched_queue, wait_for_cpu, release_cpu}, 
    { init_sched_queue, destroy_sched_queue, wake_up_worker, wait_for_worker, next_worker_fifo, wait_for_queue}
},
sched_rr = 
{
    { init_thread_info, destroy_thread_info, enter_sched_queue, leave_sched_queue, wait_for_cpu, release_cpu}, 
    { init_sched_queue, destroy_sched_queue, wake_up_worker, wait_for_worker, next_worker_rr, wait_for_queue} 
};
