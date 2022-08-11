/* date = July 15th 2022 4:23 pm */

#ifndef BASE_SYNCHRONIZATION_H
#define BASE_SYNCHRONIZATION_H

//~ NOTE(fakhri): prefere readers
typedef struct sync_reader_pref sync_reader_pref;
struct sync_reader_pref
{
    u32 ReadersCount;
    semaphore_handle ReadersMutex;
    semaphore_handle ResourceSemaphore;
};

internal void Sync_MakeSynchronizationObject(sync_reader_pref *Sync);
internal void Begin_SyncSection_Read(sync_reader_pref *Sync);
internal void End_SyncSection_Read(sync_reader_pref *Sync);
internal void Begin_SyncSection_Write(sync_reader_pref *Sync);
internal void End_SyncSection_Write(sync_reader_pref *Sync);

#endif //BASE_SYNCHRONIZATION_H
