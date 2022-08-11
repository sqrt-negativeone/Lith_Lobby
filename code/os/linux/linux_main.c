
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

enum epoll_origin
{
    epoll_origin_PlayerListen,
    epoll_origin_HostListen,
    epoll_origin_HostCloseSignal,
};


typedef struct epoll_context epoll_context;
struct epoll_context
{
    enum epoll_origin Origin;
    socket_handle Socket;
    void *Data;
};

int main()
{
    signal(SIGPIPE, SIG_IGN);
    
    MainThreadContext = MakeThreadContext();
    SetThreadContext(&MainThreadContext);
    
    WorkQueue_SetupQueue(&LinuxWorkQueue, WORKER_THREADS_INITIAL_COUNT);
    
    
    // NOTE(fakhri): launch worker threads
    for (u32 ThreadIndex = 0;
         ThreadIndex < WORKER_THREADS_INITIAL_COUNT;
         ++ThreadIndex)
    {
        pthread_t ThreadID;
        pthread_create(&ThreadID, 0, WorkerThreadMain, &LinuxWorkQueue);
        pthread_detach(ThreadID);
    }
    
    app_context *App = MakeAppContext(M_ArenaAlloc(Megabytes(100)));
    
    socket_handle PlayerListenSocket = OS_OpenListenSocket(PLAYER_PORT);
    socket_handle HostListenSocket = OS_OpenListenSocket(HOST_PORT);
    
    int EpollFD = epoll_create1(0);
    if (EpollFD != -1)
    {
        epoll_context PlayerEpollContext;
        epoll_context HostEpollContext;
        
        // NOTE(fakhri): register http server
        {
            PlayerEpollContext.Origin = epoll_origin_PlayerListen;
            PlayerEpollContext.Socket = PlayerListenSocket;
            
            struct epoll_event PlayerEpollEvent;
            PlayerEpollEvent.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
            PlayerEpollEvent.data.ptr = &PlayerEpollContext;
            Assert(!epoll_ctl(EpollFD, EPOLL_CTL_ADD, PlayerListenSocket, &PlayerEpollEvent));
        }
        
        // NOTE(fakhri): register rtmp server
        {
            HostEpollContext.Origin = epoll_origin_HostListen;
            HostEpollContext.Socket = HostListenSocket;
            
            struct epoll_event HostEpollEvent;
            HostEpollEvent.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
            HostEpollEvent.data.ptr = &HostEpollContext;
            Assert(!epoll_ctl(EpollFD, EPOLL_CTL_ADD, HostListenSocket, &HostEpollEvent));
        }
        
        for (;;)
        {
            struct epoll_event Events;
            int EpollResult = epoll_wait(EpollFD, &Events, 1, -1);
            if (EpollResult < 0 && errno == EINTR) continue;
            Assert(EpollResult > 0);
            epoll_context *EpollContext = (epoll_context *)Events.data.ptr;
            Assert(EpollContext);
            b32 ShouldResume = false;
            switch(EpollContext->Origin)
            {
                case epoll_origin_HostCloseSignal:
                {
                    host_context *HostContext = (host_context *)EpollContext->Data;
                    if (HostContext && HostContext->GameIDHashSlot)
                    {
                        Log("Removing GameID :%.*s", Str8Expand(HostContext->GameIDHashSlot->Key.Content));
                        Begin_SyncSection_Write(&App->Sync);
                        Hashtable_RemoveSlot(&App->Hosts, HostContext->GameIDHashSlot);
                        End_SyncSection_Write(&App->Sync);
                    }
                    Assert(!epoll_ctl(HostContext->EpollFD, EPOLL_CTL_DEL, HostContext->Socket, 0));
                    OS_CloseSocket(HostContext->Socket);
                    M_ArenaRelease(HostContext->Arena);
                } break;
                case epoll_origin_PlayerListen:
                {
                    socket_handle PlayerSocket = OS_AcceptSocket(PlayerListenSocket, 0, 0);
                    
                    if (PlayerSocket != InvalidSocket)
                    {
                        m_arena *Arena = M_ArenaAlloc(Megabytes(1));
                        player_input *Input = PushStructZero(Arena, player_input);
                        Input->Socket = PlayerSocket;
                        Input->Arena = Arena;
                        Input->App = App;
                        WorkQueue_PushEntrySP(&LinuxWorkQueue, ProcessPlayerRequest, Input);
                    }
                    ShouldResume = true;
                } break;
                case epoll_origin_HostListen:
                {
                    struct sockaddr_in Addr;
                    int AddrLen = sizeof(Addr);
                    MemoryZeroStruct(&Addr);
                    socket_handle HostSocket = OS_AcceptSocket(HostListenSocket, (socket_address *)&Addr, &AddrLen);
                    Log("the ip of the host is %d", Addr.sin_addr.s_addr);
                    if (HostSocket != InvalidSocket)
                    {
                        m_arena *Arena = M_ArenaAlloc(Megabytes(1));
                        host_context *Input = PushStructZero(Arena, host_context);
                        
                        Input->Socket = HostSocket;
                        Input->Arena = Arena;
                        Input->App = App;
                        Input->Address = Addr.sin_addr.s_addr;
                        Input->Port = Addr.sin_port;
                        Input->EpollFD = EpollFD;
                        Log("received connection from %d:%d", Input->Address, Input->Port);
                        Input->EpollContext = PushStructZero(Arena, epoll_context);
                        Input->EpollContext->Origin = epoll_origin_HostCloseSignal;
                        Input->EpollContext->Socket = HostSocket;
                        Input->EpollContext->Data = Input;
                        
                        WorkQueue_PushEntrySP(&LinuxWorkQueue, ProcessHostRequest, Input);
                    }
                    ShouldResume = true;
                } break;
                default: NotImplemented;
            }
            
            if (ShouldResume)
            {
                struct epoll_event ResumeEpollEvent;
                ResumeEpollEvent.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                ResumeEpollEvent.data.ptr = EpollContext;
                epoll_ctl(EpollFD, EPOLL_CTL_MOD, EpollContext->Socket, &ResumeEpollEvent);
            }
        }
    }
    
    return 0;
}


