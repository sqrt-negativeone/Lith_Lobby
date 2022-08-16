
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
    
    m_arena *AppArena = M_ArenaAlloc(Megabytes(100));
    app_context *App = MakeAppContext(AppArena);
    
    int number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    u32 WorkerThreadsCount = Max(number_of_processors - 1, 1);
    WorkQueue_SetupQueue(&LinuxWorkQueue, WorkerThreadsCount);
    // NOTE(fakhri): launch worker threads
    for (u32 ThreadIndex = 0;
         ThreadIndex < WorkerThreadsCount;
         ++ThreadIndex)
    {
        struct worker_info *WorkerInfo = PushStructZero(AppArena, struct worker_info);
        
        WorkerInfo->WorkQueue = &LinuxWorkQueue;
        WorkerInfo->WorkerID = ThreadIndex;
        
        pthread_t ThreadID;
        pthread_create(&ThreadID, 0, WorkerThreadMain, WorkerInfo);
        pthread_detach(ThreadID);
    }
    
    socket_handle PlayerListenSocket = OS_OpenListenSocket(PLAYER_PORT);
    socket_handle HostListenSocket = OS_OpenListenSocket(HOST_PORT);
    
    int EpollFD = epoll_create1(0);
    if (EpollFD != -1)
    {
        epoll_context PlayerEpollContext;
        epoll_context HostEpollContext;
        
        {
            PlayerEpollContext.Origin = epoll_origin_PlayerListen;
            PlayerEpollContext.Socket = PlayerListenSocket;
            
            struct epoll_event PlayerEpollEvent;
            PlayerEpollEvent.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
            PlayerEpollEvent.data.ptr = &PlayerEpollContext;
            Assert(!epoll_ctl(EpollFD, EPOLL_CTL_ADD, PlayerListenSocket, &PlayerEpollEvent));
        }
        
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
                        Input->EpollFD = EpollFD;
                        
                        Input->EpollContext = PushStructZero(Arena, epoll_context);
                        Input->EpollContext->Origin = epoll_origin_HostCloseSignal;
                        Input->EpollContext->Socket = HostSocket;
                        Input->EpollContext->Data = Input;
                        
                        WorkQueue_PushEntrySP(&LinuxWorkQueue, ProcessHostRequest, Input);
                    }
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


