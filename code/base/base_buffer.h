/* date = June 22nd 2022 4:00 pm */

#ifndef BASE_BUFFER_H
#define BASE_BUFFER_H

struct buffer
{
    buffer *Next;
    buffer *Prev;
    
    string8 Content;
    u64 Capacity;
    u32 Log2Capacity; // NOTE(fakhri): index in the buffer_pool 
};

struct buffer_list
{
    buffer *First;
    buffer *Last;
    u32 NodeCount;
};

// NOTE(fakhri): allocate and free power of two sized buffers
struct buffer_pool
{
    // NOTE(fakhri): buffers in Buffers[i] list has 
    // a capacity of (1 << i)
    buffer_list Buffers[32];
};


internal buffer *MakeBuffer(m_arena *Arena, u64 Capacity);
internal b32     BufferWriteBytes(buffer *Buffer, string8 Data);
internal void    BufferListPushBuffer(buffer_list *BufferList, buffer *Buffer);
internal buffer *BufferListPopBuffer(buffer_list *BufferList);
internal void    BufferPoolAddBuffer(buffer_pool *Pool, buffer *Buffer);
// NOTE(fakhri): returns a buffer of a power of two capacity
// large enough to hold Size bytes
internal buffer *BufferPoolGetBuffer(m_arena *Arena, buffer_pool *Pool, u64 Size);

#endif //BASE_BUFFER_H
