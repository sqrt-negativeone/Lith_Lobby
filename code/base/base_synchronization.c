
// NOTE(fakhri): https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem

//~ NOTE(fakhri): prefer readers

internal void 
Sync_MakeSynchronizationObject(sync_reader_pref *Sync)
{
    OS_CreateSemaphore(&Sync->ReadersMutex     , 1, 1);
    OS_CreateSemaphore(&Sync->ResourceSemaphore, 1, 1);
}

internal void
Begin_SyncSection_Read(sync_reader_pref *Sync)
{
    OS_AcquireSemaphore(&Sync->ReadersMutex);
    ++Sync->ReadersCount;
    if (Sync->ReadersCount == 1)
    {
        // NOTE(fakhri): prevent writers from change the storage
        OS_AcquireSemaphore(&Sync->ResourceSemaphore);
    }
    OS_ReleaseSemaphore(&Sync->ReadersMutex);
}

internal void
End_SyncSection_Read(sync_reader_pref *Sync)
{
    OS_AcquireSemaphore(&Sync->ReadersMutex);
    --Sync->ReadersCount;
    if (Sync->ReadersCount == 0)
    {
        // NOTE(fakhri): all write operations are done, allow writing again
        OS_ReleaseSemaphore(&Sync->ResourceSemaphore);
    }
    OS_ReleaseSemaphore(&Sync->ReadersMutex);
}

internal void
Begin_SyncSection_Write(sync_reader_pref *Sync)
{
    OS_AcquireSemaphore(&Sync->ResourceSemaphore);
}

internal void
End_SyncSection_Write(sync_reader_pref *Sync)
{
    OS_ReleaseSemaphore(&Sync->ResourceSemaphore);
}


