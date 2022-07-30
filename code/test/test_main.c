

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

internal void *
WorkerThreadMain(void *Input)
{
    thread_ctx WorkerThreadContext = MakeThreadContext();
    SetThreadContext(&WorkerThreadContext);
    
    work_queue *WorkQueue = (work_queue *)Input;
    
    for(;;)
    {
        b32 DidWork = WorkQueue_ProcessOneEntry(WorkQueue);
        if (!DidWork)
        {
            // NOTE(fakhri): work queue was empty, call the os
            // to see what should we do about it
            OS_AcquireSemaphore(&WorkQueue->WaitingThreadsSemaphore);
        }
    }
    return 0;
}

int main()
{
    signal(SIGPIPE, SIG_IGN);
    m_arena *Arena = M_ArenaAllocDefault();
    MainThreadContext = MakeThreadContext();
    SetThreadContext(&MainThreadContext);
    
    socket_handle HostSocket = OS_OpenClientSocket("localhost", HOST_PORT);
    Assert(HostSocket);
    Log("Host connected to lobby");
    string8 GameID;
    network_result ReceiveResult = OS_NetworkReceiveAllBytes(HostSocket, &GameID.size, sizeof(GameID.size));
    Assert(!ReceiveResult.Error);
    GameID.str = PushArray(Arena, u8, GameID.size);
    ReceiveResult = OS_NetworkReceiveAllBytes(HostSocket, GameID.str, GameID.size);
    Assert(!ReceiveResult.Error);
    Log("Host received GameID: %.*s", Str8Expand(GameID));
    
    socket_handle PlayerSocket = OS_OpenClientSocket("localhost", PLAYER_PORT);
    Log("Player1 connect to lobby");
    network_result SendResult = OS_NetworkSendAllBytes(PlayerSocket, &GameID.size, sizeof(GameID.size));
    Assert(!SendResult.Error);
    SendResult = OS_NetworkSendAllBytes(PlayerSocket, GameID.str, GameID.size);
    Assert(!SendResult.Error);
    Log("Player1 asked for game id %.*s", Str8Expand(GameID));
    u32 HostAddres;
    ReceiveResult = OS_NetworkReceiveAllBytes(PlayerSocket, &HostAddres, sizeof(HostAddres));
    Log("Player1 received host address: %d", HostAddres);
    Assert(!ReceiveResult.Error);
    OS_CloseSocket(HostSocket);
    Log("Host disconnects");
    
    socket_handle PlayerSocket2 = OS_OpenClientSocket("localhost", PLAYER_PORT);
    Log("Player1 connect to lobby");
    SendResult = OS_NetworkSendAllBytes(PlayerSocket2, &GameID.size, sizeof(GameID.size));
    Assert(!SendResult.Error);
    SendResult = OS_NetworkSendAllBytes(PlayerSocket2, &GameID.str, GameID.size);
    Assert(!SendResult.Error);
    Log("Player1 asked for game id %.*s", Str8Expand(GameID));
    HostAddres;
    ReceiveResult = OS_NetworkReceiveAllBytes(PlayerSocket2, &HostAddres, sizeof(HostAddres));
    Assert(ReceiveResult.Error == network_error_PeerClosed);
    Log("Player1 didn't receive host address");
    Log("Test Passed");
    return 0;
}


