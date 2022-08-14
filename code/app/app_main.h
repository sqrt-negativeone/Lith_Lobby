/* date = July 30th 2022 1:29 pm */

#ifndef APP_MAIN_H
#define APP_MAIN_H

#define HOST_PORT   "42069"
#define PLAYER_PORT "6969"
#define GameIDLength 10

typedef struct host host;
struct host
{
    socket_handle Socket;
    
    u32 PublicAddress;
    u16 PublicPort;
    u32 LocalAddress;
    u16 LocalPort;
};

typedef hashtable host_storage;

typedef struct app_context app_context;
struct app_context
{
    sync_reader_pref Sync;
    m_arena *Arena;
    hashtable ExistingGameIDs;
    random_generator GameIDsEntroy;
    random_generator GeneralEntropy;
    host_storage Hosts;
};

typedef struct player_input player_input;
struct player_input
{
    m_arena *Arena;
    socket_handle Socket;
    u32 PublicAddress;
    u16 PublicPort;
    app_context *App;
};

typedef struct host_context host_context;
struct host_context
{
    m_arena *Arena;
    socket_handle Socket;
    u32 PublicAddress;
    u16 PublicPort;
    app_context *App;
    hashtable_slot *GameIDHashSlot;
    
    struct epoll_context *EpollContext;
    int EpollFD;
};

inline_internal hashtable_slot *
HostStorage_InsertHost(m_arena *Arena, host_storage *Hosts, string8 GameID, host *Host)
{
    hashtable_slot *Result = 0;
    Hashtable_InsertTypedValue(Arena, Hosts, MakeHashtableKey(GameID), host, Host, Result);
    return Result;
}

inline_internal host *
HostStorage_FindHost(host_storage *Hosts, string8 GameID)
{
    hashtable_slot *Slot = Hashtable_Find(Hosts, MakeHashtableKey(GameID));
    host *Result = (host*)(Slot? Slot->Value:0);
    return Result;
}

internal app_context *MakeAppContext(m_arena *Arena);

#endif //APP_MAIN_H
