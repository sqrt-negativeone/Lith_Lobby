
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "windows.h"
#undef DeleteFile

// NOTE(fakhri): definitions
#include "base/base_inc.h"
#include "os/os_inc.h"

// NOTE(fakhri): global variables
global m_arena *W32_PermArena = 0;
global string8 W32_InitalPath = {0};
global work_queue W32_WorkQueue;
global thread_ctx W32ThreadContext;

// NOTE(fakhri): implementations
#include "base/base_inc.cpp"
#include "os/os_inc.cpp"

struct host
{
    socket_handle Socket;
    u32 Address;
};

struct host_storage
{
    string_hashset HashtableKeys;
    host Values[HashTableSize];
};

#define Hosts_GetValue(Hosts, GameID, Host) \
Hashtable_GetValue(Hosts, GameID, host, Host, ZERO_STRUCT)

#define Hosts_Insert(Hosts, GameIDNode, Host) \
Hashtable_InsertNode(Hosts, GameIDNode, Host)

struct app_context
{
    sync_reader_pref Sync;
    m_arena *Arena;
    string_hashset *ExistingGameIDs;
    random_generator GameIDsEntroy;
    random_generator GeneralEntropy;
    host_storage Hosts;
    
    string8_node *FreeNodes;
};

#define HOST_PORT   "1234"
#define PLAYER_PORT "1235"

struct player_input
{
    m_arena *Arena;
    socket_handle Socket;
    app_context *App;
};

struct host_input
{
    m_arena *Arena;
    socket_handle Socket;
    u32 Address;
    app_context *App;
};

typedef u32 host_message;
enum
{
    host_message_Connect = 0,
};

#define GameIDLength 10

internal void
ProcessPlayerRequest(void *Input)
{
    player_input *PlayerInput = (player_input *)Input;
    app_context *App = PlayerInput->App;
    string8 GameID;
    network_result ReceiveResult = OS_NetworkReceiveBuffer(PlayerInput->Socket, &GameID.size, sizeof(GameID.size));
    if (!ReceiveResult.Error)
    {
        GameID.str = PushArray(PlayerInput->Arena, u8, GameID.size);
        ReceiveResult = OS_NetworkReceiveBuffer(PlayerInput->Socket, GameID.str, (i32)GameID.size);
        
        if (!ReceiveResult.Error)
        {
            Begin_SyncSection_Read(&App->Sync);
            Hosts_GetValue(&App->Hosts, GameID, Host);
            End_SyncSection_Read(&App->Sync);
            if (Host.Address)
            {
                OS_NetworkSendBuffer(PlayerInput->Socket, &Host.Address, sizeof(Host.Address));
            }
        }
    }
    
    OS_CloseSocket(PlayerInput->Socket);
    M_ArenaRelease(PlayerInput->Arena);
}

internal void
ProcessHostRequest(void *Input)
{
    host_input *HostInput = (host_input *)Input;
    app_context *App = HostInput->App;
    
    Begin_SyncSection_Write(&App->Sync);
    string8_node *GameIDNode = 0;
    if (App->FreeNodes)
    {
        GameIDNode = App->FreeNodes;
        App->FreeNodes = GameIDNode->Next;
    }
    
    if (!GameIDNode)
    {
        GameIDNode = PushArrayZero(App->Arena, string8_node, 1);
        GameIDNode->String.str = PushArray(App->Arena, u8, GameIDLength);
        GameIDNode->String.size = GameIDLength;
    }
    
    {
        string8 TempGameID = CreateBase64StringNotInSet(HostInput->Arena, &App->GameIDsEntroy, App->ExistingGameIDs, GameIDLength);
        MemoryCopy(GameIDNode->String.str, TempGameID.str, GameIDLength);
    }
    
    string8 GameID = GameIDNode->String;
    Log("generated game id: %.*s", Str8Expand(GameIDNode->String));
    host Host;
    Host.Socket = HostInput->Socket;
    Host.Address = HostInput->Address;
    Hosts_Insert(&App->Hosts, GameIDNode, Host);
    End_SyncSection_Write(&App->Sync);
    
    // NOTE(fakhri): let the host know what is his game id
    OS_NetworkSendBuffer(HostInput->Socket, &GameID.size, sizeof(GameID.size));
    OS_NetworkSendBuffer(HostInput->Socket, GameID.str, (u32)GameID.size);
    
    // TODO(fakhri): register the host socket in the completion port to know when the host 
    // is no longer active
    
    M_ArenaRelease(HostInput->Arena);
}

internal DWORD WINAPI 
W32_WorkerThreadMain(void *Input)
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
}


int main()
{
    // NOTE(fakhri): main thread context
    {
        W32ThreadContext = MakeThreadContext();
        SetThreadContext(&W32ThreadContext);
    }
    
    // NOTE(fakhri): setup network
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) 
        {
            LogError("Couldn't init socket");
            PostQuitMessage(1);
        }
    }
    
    W32_PermArena  = M_ArenaAlloc(Gigabytes(1));
    W32_InitalPath = OS_GetSystemPath(W32_PermArena, SystemPath_Current);
    
    app_context *App = PushStructZero(W32_PermArena, app_context);
    Sync_MakeSynchronizationObject(&App->Sync);
    App->ExistingGameIDs = &App->Hosts.HashtableKeys;
    App->GameIDsEntroy = MakeLinearRandomGenerator(1234);
    App->GeneralEntropy = MakeLinearRandomGenerator(5678);
    App->Arena = M_ArenaAlloc(Megabytes(100));
    
#define WorkersCount 8
    WorkQueue_SetupQueue(&W32_WorkQueue, WorkersCount);
    
    // NOTE(fakhri): start worker threads
    for (u32 Index = 0;
         Index < WorkersCount;
         ++Index)
    {
        CloseHandle(CreateThread(0, 0, W32_WorkerThreadMain, &W32_WorkQueue, 0, 0));
    }
    
    Log("Started worker threads");
    
    socket_handle PlayerListenSocket = OS_OpenListenSocket(PLAYER_PORT);
    socket_handle HostListenSocket = OS_OpenListenSocket(HOST_PORT);
    
    fd_set ReadyFDs, AllFDs;
    
    FD_ZERO(&ReadyFDs);
    FD_ZERO(&AllFDs);
    
    FD_SET(PlayerListenSocket, &AllFDs);
    FD_SET(HostListenSocket, &AllFDs);
    
    
    for(;;)
    {
        ReadyFDs = AllFDs;
        if (select(0, &ReadyFDs, 0, 0, 0) != SOCKET_ERROR)
        {
            if (FD_ISSET(PlayerListenSocket, &ReadyFDs))
            {
                socket_handle PlayerSocket = OS_AcceptSocket(PlayerListenSocket, 0, 0);
                
                if (PlayerSocket != InvalidSocket)
                {
                    m_arena *Arena = M_ArenaAlloc(Megabytes(1));
                    player_input *Input = PushStructZero(Arena, player_input);
                    Input->Socket = PlayerSocket;
                    Input->Arena = Arena;
                    Input->App = App;
                    WorkQueue_PushEntrySP(&W32_WorkQueue, ProcessPlayerRequest, Input);
                }
            }
            if (FD_ISSET(HostListenSocket, &ReadyFDs))
            {
                sockaddr_in Addr;
                int AddrLen = sizeof(Addr);
                MemoryZeroStruct(&Addr);
                socket_handle HostSocket = OS_AcceptSocket(HostListenSocket, (socket_address *)&Addr, &AddrLen);
                Log("the ip of the host is %d", Addr.sin_addr.S_un.S_addr);
                if (HostSocket != InvalidSocket)
                {
                    m_arena *Arena = M_ArenaAlloc(Megabytes(1));
                    host_input *Input = PushStructZero(Arena, host_input);
                    Input->Socket = HostSocket;
                    Input->Arena = Arena;
                    Input->App = App;
                    Input->Address = Addr.sin_addr.S_un.S_addr;
                    WorkQueue_PushEntrySP(&W32_WorkQueue, ProcessHostRequest, Input);
                }
            }
        }
        else
        {
            Log("select failed");
            break;
        }
    }
    
}