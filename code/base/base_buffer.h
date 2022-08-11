/* date = June 22nd 2022 4:00 pm */

#ifndef BASE_BUFFER_H
#define BASE_BUFFER_H

// TODO(fakhri): hmmm isn't this just a bad way to do general purpose memory allocation?

typedef struct base_buffer base_buffer;
struct base_buffer
{
    base_buffer *Next;
    base_buffer *Prev;
    
    string8 Content;
    u64 Capacity;
    u32 Log2Capacity; // NOTE(fakhri): index in the buffer_pool 
};

typedef struct buffer_list buffer_list;
struct buffer_list
{
    base_buffer *First;
    base_buffer *Last;
    u32 NodeCount;
};

// NOTE(fakhri): allocate and free power of two sized buffers
typedef struct buffer_pool buffer_pool;
struct buffer_pool
{
    // NOTE(fakhri): buffers in Buffers[i] list has 
    // a capacity of (1 << i)
    buffer_list Buffers[32];
};


internal base_buffer *MakeBuffer(m_arena *Arena, u64 Capacity);
internal b32     BufferWriteBytes(base_buffer *Buffer, string8 Data);
internal void    BufferListPushBuffer(buffer_list *BufferList, base_buffer *Buffer);
internal base_buffer *BufferListPopBuffer(buffer_list *BufferList);
internal void    BufferPoolAddBuffer(buffer_pool *Pool, base_buffer *Buffer);
// NOTE(fakhri): returns a buffer of a power of two capacity
// large enough to hold Size bytes
internal base_buffer *BufferPoolGetBuffer(m_arena *Arena, buffer_pool *Pool, u64 Size);

#endif //BASE_BUFFER_H
