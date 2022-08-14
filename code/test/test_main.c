

#include <signal.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/epoll.h>

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "app/app_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "app/app_inc.c"

#define WORKER_THREADS_INITIAL_COUNT 8

global thread_ctx MainThreadContext;
global work_queue LinuxWorkQueue;

global u16 HostUsedPort;
global socket_handle HostLobbySocket;
global string8 GameID;

internal
WORK_TASK_SIG(HostWork)
{
    u32 PlayerIP;
    u16 PlayerPort;
    network_result ReceiveResult = OS_NetworkReceiveAllBytes(HostLobbySocket, &PlayerIP, sizeof(PlayerIP));
    Assert(!ReceiveResult.Error);
    ReceiveResult = OS_NetworkReceiveAllBytes(HostLobbySocket, &PlayerPort, sizeof(PlayerPort));
    Assert(!ReceiveResult.Error);
    
    socket_handle PlayerSocket = Linux_MakeSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    const int OptionValue = 1;
    if (!setsockopt(PlayerSocket, SOL_SOCKET, SO_REUSEADDR, &OptionValue, sizeof(OptionValue)) &&
        !setsockopt(PlayerSocket, SOL_SOCKET, SO_REUSEPORT, &OptionValue, sizeof(OptionValue)))
    {
        struct sockaddr_in LocalAddr;
        MemoryZeroStruct(&LocalAddr);
        LocalAddr.sin_family = AF_INET;
        LocalAddr.sin_port = HostUsedPort;
        LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (!bind(PlayerSocket, (const struct sockaddr *)&LocalAddr, sizeof(LocalAddr)))
        {
            struct sockaddr_in SockAddr;
            MemoryZeroStruct(&SockAddr);
            SockAddr.sin_family = AF_INET;
            SockAddr.sin_port = PlayerPort;
            SockAddr.sin_addr.s_addr = PlayerIP;
            // TODO(fakhri): bind socket to the same port as 
            while(!Linux_ConnectSocket(PlayerSocket, (struct sockaddr *)&SockAddr, sizeof(SockAddr)))
            {
                Log("connect failed with error %d", errno);
            }
            
            Log("Connection Succeded with player!");
            u32 MessageLength;
            OS_NetworkReceiveAllBytes(PlayerSocket, &MessageLength, sizeof(MessageLength));
            char Message[100];
            OS_NetworkReceiveAllBytes(PlayerSocket, &Message, MessageLength);
            Log("Player sent: %s", Message);
        }
        else
        {
            Log("bind failed with error %d", errno);
        }
    }
    else
    {
        Log("setsockopt failed with error %d", errno);
    }
    
}

internal
WORK_TASK_SIG(PlayerWork)
{
    u16 PlayerUsedPort;
    socket_handle LobbySocket = OS_OpenClientSocket("0.0.0.0", PLAYER_PORT);
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(LobbySocket, (struct sockaddr *)&sin, &len) != -1)
    {
        PlayerUsedPort = sin.sin_port;
    }
    network_result SendResult = OS_NetworkSendAllBytes(LobbySocket, &GameID.size, sizeof(GameID.size));
    Assert(!SendResult.Error);
    SendResult = OS_NetworkSendAllBytes(LobbySocket, GameID.str, GameID.size);
    Assert(!SendResult.Error);
    u32 HostIP;
    network_result ReceiveResult = OS_NetworkReceiveAllBytes(LobbySocket, &HostIP, sizeof(HostIP));
    Assert(!ReceiveResult.Error);
    u16 HostPort;
    ReceiveResult = OS_NetworkReceiveAllBytes(LobbySocket, &HostPort, sizeof(HostPort));
    Assert(!ReceiveResult.Error);
    
    socket_handle HostSocket = Linux_MakeSocket(AF_INET, SOCK_STREAM, 0);
    const int OptionValue = 1;
    if (!setsockopt(HostSocket, SOL_SOCKET, SO_REUSEADDR, &OptionValue, sizeof(OptionValue)) &&
        !setsockopt(HostSocket, SOL_SOCKET, SO_REUSEPORT, &OptionValue, sizeof(OptionValue)))
    {
        struct sockaddr_in LocalAddr;
        MemoryZeroStruct(&LocalAddr);
        LocalAddr.sin_family = AF_INET;
        LocalAddr.sin_port = PlayerUsedPort;
        LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (!bind(HostSocket, (const struct sockaddr *)&LocalAddr, sizeof(LocalAddr)))
        {
            struct sockaddr_in SockAddr;
            MemoryZeroStruct(&SockAddr);
            SockAddr.sin_family = AF_INET;
            SockAddr.sin_port = HostPort;
            SockAddr.sin_addr.s_addr = HostIP;
            // TODO(fakhri): bind socket to the same port as 
            while(!Linux_ConnectSocket(HostSocket, (struct sockaddr *)&SockAddr, sizeof(SockAddr)))
            {
                Log("connect failed with error %d", errno);
            }
            Log("Connected to Host!!");
            char Message[] = "Hello Server!";
            u32 MessageLength = sizeof(Message);
            OS_NetworkSendAllBytes(HostSocket, &MessageLength, sizeof(MessageLength));
            OS_NetworkSendAllBytes(HostSocket, Message, MessageLength);
        }
        else
        {
            Log("bind failed with error %d", errno);
        }
    }
    else
    {
        Log("setsockopt failed with error %d", errno);
    }
}

int main()
{
    signal(SIGPIPE, SIG_IGN);
    m_arena *Arena = M_ArenaAllocDefault();
    MainThreadContext = MakeThreadContext();
    SetThreadContext(&MainThreadContext);
    
    int number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    u32 WorkerThreadsCount = Max(number_of_processors - 1, 1);
    WorkQueue_SetupQueue(&LinuxWorkQueue, WorkerThreadsCount);
    // NOTE(fakhri): launch worker threads
    for (u32 ThreadIndex = 0;
         ThreadIndex < WorkerThreadsCount;
         ++ThreadIndex)
    {
        struct worker_info *WorkerInfo = PushStructZero(Arena, struct worker_info);
        
        WorkerInfo->WorkQueue = &LinuxWorkQueue;
        WorkerInfo->WorkerID = ThreadIndex;
        
        pthread_t ThreadID;
        pthread_create(&ThreadID, 0, WorkerThreadMain, WorkerInfo);
        pthread_detach(ThreadID);
    }
    
    HostLobbySocket = OS_OpenClientSocket("0.0.0.0", HOST_PORT);
    Assert(HostLobbySocket);
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(HostLobbySocket, (struct sockaddr *)&sin, &len) != -1)
    {
        HostUsedPort = sin.sin_port;
    }
    
    Log("Host connected to lobby, using port %d", (i32)HostUsedPort);
    
    network_result ReceiveResult = OS_NetworkReceiveAllBytes(HostLobbySocket, &GameID.size, sizeof(GameID.size));
    Assert(!ReceiveResult.Error);
    GameID.str = PushArray(Arena, u8, GameID.size);
    ReceiveResult = OS_NetworkReceiveAllBytes(HostLobbySocket, GameID.str, GameID.size);
    Assert(!ReceiveResult.Error);
    WorkQueue_PushEntry(&LinuxWorkQueue, HostWork, 0);
    WorkQueue_PushEntry(&LinuxWorkQueue, PlayerWork, 0);
    
    getc(stdin);
    Log("Test Passed");
    return 0;
}


