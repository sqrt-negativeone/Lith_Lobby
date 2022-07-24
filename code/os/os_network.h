/* date = June 30th 2022 0:56 pm */

#ifndef OS_NETWORK_H
#define OS_NETWORK_H


typedef struct sockaddr socket_address;
typedef struct addrinfo addrinfo;

#if OS_LINUX
#define InvalidSocket (-1)
#elif OS_WINDOWS
#define InvalidSocket INVALID_SOCKET
#endif
#define LISTENQ 1000

#define IO_BUFFER_SIZE Kilobytes(8)
struct buffered_socket
{
    socket_handle Socket;
    u32 AvailableBytes;
    b8 *Buffer;
    u32 Capacity;
    u32 ReadOffest;
};

enum network_error
{
    network_error_None = 0,
    network_error_SocketNotConnnected = 1,
    network_error_NotSocket = 2,
    network_error_Timeout = 3,
    network_error_PeerClosed = 3,
    network_error_Unknown,
};

struct network_result
{
    network_error Error;
    u32 ProcessedBytes;
};

internal buffered_socket *OS_MakeBufferedSocket(m_arena *Arena, socket_handle Socket);
internal network_result OS_BufferedSocketReceive(buffered_socket *BufferedSocket, void *Buffer, u32 BytesToReceive);

internal socket_handle  OS_AcceptSocket(socket_handle Socket, socket_address *Addr, int *AddrLen);
internal socket_handle  OS_MakeSocket(int Domain, int Type, int Protocol);
internal socket_handle  OS_ConnectSocket(socket_handle Socket, const struct sockaddr *Addr, int AddrLen);
internal b32            OS_CloseSocket(socket_handle Socket);
internal socket_handle  OS_OpenClientSocket(char *Hostname, char *Port);
internal socket_handle  OS_OpenListenSocket(char *Port);

internal network_result OS_NetworkReceiveBuffer(socket_handle Socket, void *Buffer, i32 BytesToReceive);
internal network_result OS_NetworkSendBuffer(socket_handle Socket, void *Buffer, u32 BytesToSend);
internal network_result OS_NetworkSendBuffer(socket_handle Socket, string8_list Stream);
internal network_result OS_NetworkReceive(socket_handle Socket, void *Buffer, i32 BufferSize);

#endif //OS_NETWORK_H
