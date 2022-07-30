/* date = July 27th 2022 7:13 pm */

#ifndef BASE_WORK_QUEUE_H
#define BASE_WORK_QUEUE_H

typedef void worker_work(void *);

typedef struct work_entry work_entry;
struct work_entry
{
    worker_work *Work;
    void *Data;
};

typedef struct next_work next_work;
struct next_work
{
    work_entry WorkEntry;
    b32 IsValid;
};

typedef struct work_queue work_queue;
struct work_queue
{
    work_entry Entries[8192];
    volatile u64 Head;
    volatile u64 Tail;
    semaphore_handle WaitingThreadsSemaphore;
    semaphore_handle ProducersMutex;
};


//- NOTE(fakhri): work queue functions
internal void      WorkQueue_SetupQueue(work_queue *WorkQueue, u32 WorkersCount);
internal next_work WorkQueue_GetNextWorkToDo(work_queue *WorkQueue);
internal b32       WorkQueue_ProcessOneEntry(work_queue *WorkQueue);
internal b32       WorkQueue_PushEntrySP(work_queue *WorkQueue, worker_work *Work, void *Data);
internal b32       WorkQueue_PushEntry(work_queue *WorkQueue, worker_work *Work, void *Data);
internal b32       WorkQueue_IsEmpty(work_queue *WorkQueue);
internal b32       WorkQueue_IsFull(work_queue *WorkQueue);

#endif //BASE_WORK_QUEUE_H
