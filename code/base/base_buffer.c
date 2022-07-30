
internal base_buffer *
MakeBuffer(m_arena *Arena, u64 Capacity)
{
    base_buffer *Result = PushStructZero(Arena, base_buffer);
    Result->Content.str = PushArray(Arena, u8, Capacity);
    Result->Capacity = Capacity;
    return Result;
}

internal b32
BufferWriteBytes(base_buffer *Buffer, string8 Data)
{
    b32 Result = 0;
    if (Buffer->Capacity >= Data.size)
    {
        Result = 1;
        MemoryCopy(Buffer->Content.str, Data.str, Data.size);
        Buffer->Content.size = Data.size;
    }
    return Result;
}


internal void
BufferListPushBuffer(buffer_list *BufferList, base_buffer *Buffer)
{
    QueuePush(BufferList->First, BufferList->Last, Buffer);
    BufferList->NodeCount += 1;
}

internal base_buffer *
BufferListPopBuffer(buffer_list *BufferList)
{
    base_buffer *Result = BufferList->First;
    if (BufferList->First)
    {
        QueuePop(BufferList->First, BufferList->Last);
        BufferList->NodeCount -= 1;
    }
    return Result;
}

internal void
BufferPoolAddBuffer(buffer_pool *Pool, base_buffer *Buffer)
{
    Assert(Buffer->Capacity == (1u << Buffer->Log2Capacity));
    Assert(Buffer->Log2Capacity < ArrayCount(Pool->Buffers));
    buffer_list *List = Pool->Buffers + Buffer->Log2Capacity;
    BufferListPushBuffer(List, Buffer);
}

internal base_buffer *
BufferPoolGetBuffer(m_arena *Arena, buffer_pool *Pool, u64 Size)
{
    base_buffer *Result = 0;
    u32 Log2 = 0;
    
    // NOTE(fakhri): compute from what list we git the buffer
    while((1u << Log2) < Size) ++Log2;
    Assert(Log2 < ArrayCount(Pool->Buffers));
    u64 Capacity = (1u << Log2);
    buffer_list *List = Pool->Buffers + Log2;
    
    // NOTE(fakhri): grab a buffer from the chosen list
    Result = BufferListPopBuffer(List);
    if (!Result)
    {
        // NOTE(fakhri): if the list had no buffer then allocate one
        Result = MakeBuffer(Arena, Capacity);
        Result->Log2Capacity = Log2;
    }
    return Result;
}
