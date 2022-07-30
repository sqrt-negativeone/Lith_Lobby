
internal void
WorkQueue_SetupQueue(work_queue *WorkQueue, u32 WorkersCount)
{
    OS_CreateSemaphore(&WorkQueue->WaitingThreadsSemaphore, 0, WorkersCount);
    OS_CreateSemaphore(&WorkQueue->ProducersMutex, 1, 1);
}

internal next_work
WorkQueue_GetNextWorkToDo(work_queue *WorkQueue)
{
    next_work Result = ZERO_STRUCT;
    u64 OldHead = WorkQueue->Head;
    if (!WorkQueue_IsEmpty(WorkQueue))
    {
        u64 EntryIndex = OldHead % ArrayCount(WorkQueue->Entries);
        Result.WorkEntry = WorkQueue->Entries[EntryIndex];
        if (AtomicCompareAndExchange(&WorkQueue->Head, OldHead, OldHead + 1))
        {
            Result.IsValid = true;
        }
    }
    return Result;
}

internal b32
WorkQueue_ProcessOneEntry(work_queue *WorkQueue)
{
    b32 Result = false;
    while(!WorkQueue_IsEmpty(WorkQueue))
    {
        next_work NextWorkToDo = WorkQueue_GetNextWorkToDo(WorkQueue);
        if (NextWorkToDo.IsValid)
        {
            Result = true;
            work_entry WorkEntry = NextWorkToDo.WorkEntry;
            WorkEntry.Work(WorkEntry.Data);
            break;
        }
    }
    return Result;
}

internal b32
WorkQueue_PushEntrySP(work_queue *WorkQueue, worker_work *Work, void *Data)
{
    b32 Result = false;
    
    if (!WorkQueue_IsFull(WorkQueue))
    {
        Result = true;
        u64 EntryIndex = WorkQueue->Tail % ArrayCount(WorkQueue->Entries);
        
        WorkQueue->Entries[EntryIndex].Work = Work;
        WorkQueue->Entries[EntryIndex].Data = Data;
        
        CompletePreviousWritesBeforeFutureWrites();
        // NOTE(fakhri): make the changes visible
        ++ WorkQueue->Tail;
        // NOTE(fakhri): wake up any worker thread that is waiting for a work entry
        OS_ReleaseSemaphore(&WorkQueue->WaitingThreadsSemaphore);
    }
    
    return Result;
}

internal b32
WorkQueue_PushEntry(work_queue *WorkQueue, worker_work *Work, void *Data)
{
    b32 Result = false;
    OS_AcquireSemaphore(&WorkQueue->ProducersMutex);
    Result = WorkQueue_PushEntrySP(WorkQueue, Work, Data);
    OS_ReleaseSemaphore(&WorkQueue->ProducersMutex);
    return Result;
}

internal b32
WorkQueue_IsEmpty(work_queue *WorkQueue)
{
    b32 Result = (WorkQueue->Head == WorkQueue->Tail);
    return Result;
}

internal b32
WorkQueue_IsFull(work_queue *WorkQueue)
{
    b32 Result = (WorkQueue->Tail - WorkQueue->Head) == ArrayCount(WorkQueue->Entries);
    return Result;
}
