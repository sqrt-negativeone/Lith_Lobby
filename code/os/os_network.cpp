
internal socket_handle
OS_AcceptSocket(socket_handle Socket, socket_address *Addr, int *AddrLen)
{
    socket_handle Result = InvalidSocket;
#if defined(IMPL_AcceptSocket)
    Result = IMPL_AcceptSocket(Socket, Addr, AddrLen);
#else
# error provide an implementation for this
#endif
    return Result;
}

internal socket_handle
OS_MakeSocket(int Domain, int Type, int Protocol)
{
    socket_handle Result = InvalidSocket;
#if defined(IMPL_MakeSocket)
    Result = IMPL_MakeSocket(Domain, Type, Protocol);
#else
# error provide an implementation for this
#endif
    return Result;
}


internal socket_handle
OS_ConnectSocket(socket_handle Socket, const struct sockaddr *Addr, int AddrLen)
{
    socket_handle Result = 0;
#if defined(IMPL_ConnectSocket)
    Result = IMPL_ConnectSocket(Socket, Addr, AddrLen);
#else
# error provide an implementation for this
#endif
    return Result;
}

internal b32
OS_CloseSocket(socket_handle Socket)
{
    b32 Result = 0;
#if defined(IMPL_CloseSocket)
    Result = IMPL_CloseSocket(Socket);
#else
# error provide an implementation for this
#endif
    return Result;
}

internal socket_handle 
OS_OpenClientSocket(char *Hostname, char *Port)
{
    socket_handle Result;
#if defined(IMPL_OpenClientSocket)
    Result = IMPL_OpenClientSocket(Hostname, Port);
#else
# error provide an implementation for this
#endif
    return Result;
}


internal socket_handle 
OS_OpenListenSocket(char *Port)
{
    socket_handle Result = InvalidSocket;
#if defined(IMPL_OpenListenSocket)
    Result = IMPL_OpenListenSocket(Port);
#else
# error provide an implementation for this
#endif
    return Result;
}

internal network_result
OS_NetworkReceiveBuffer(socket_handle Socket, void *Buffer, i32 BytesToReceive)
{
    network_result Result;
#if defined(IMPL_NetworkReceiveBuffer)
    Result = IMPL_NetworkReceiveBuffer(Socket, Buffer, BytesToReceive);
#else
# error provide an implementation for this
#endif
    return Result;
}

internal network_result
OS_NetworkSendBuffer(socket_handle Socket, void *Buffer, u32 BytesToSend)
{
    network_result Result;
#if defined(IMPL_NetworkSendBuffer)
    Result = IMPL_NetworkSendBuffer(Socket, Buffer, BytesToSend);
#else
# error provide an implementation for this
#endif
    return Result;
}

internal network_result
OS_NetworkSendBuffer(socket_handle Socket, string8_list Stream)
{
    network_result Result = ZERO_STRUCT;
    for (string8_node *Node = Stream.First;
         Node;
         Node = Node->Next)
    {
        network_result SendResult = OS_NetworkSendBuffer(Socket, Node->String.data, (u32)Node->String.size);
        if (SendResult.Error == network_error_None)
        {
            Result.ProcessedBytes += SendResult.ProcessedBytes;
        }
        else
        {
            Result.Error = SendResult.Error;
            break;
        }
    }
    
    return Result;
}

internal network_result
OS_NetworkReceive(socket_handle Socket, void *Buffer, i32 BufferSize)
{
    network_result Result;
#if defined(IMPL_NetworkReceive)
    Result = IMPL_NetworkReceive(Socket, Buffer, BufferSize);
#else
# error provide an implementation for this
#endif
    return Result;
}


internal buffered_socket *
OS_MakeBufferedSocket(m_arena *Arena, socket_handle Socket)
{
    buffered_socket *Result = (buffered_socket *)M_ArenaPushZero(Arena, sizeof(buffered_socket) + IO_BUFFER_SIZE);
    Result->Socket = Socket;
    Result->Buffer = (b8*)Result + sizeof(buffered_socket);
    Result->Capacity = IO_BUFFER_SIZE;
    return Result;
}

internal network_result
OS_BufferedSocketReceive(buffered_socket *BufferedSocket, void *Buffer, u32 BytesToReceive)
{
    network_result Result;
#if defined(IMPL_BufferedSocketReceive)
    Result = IMPL_BufferedSocketReceive(BufferedSocket, Buffer, BytesToReceive);
#else
# error provide an implementation for this
#endif
    return Result;
}
