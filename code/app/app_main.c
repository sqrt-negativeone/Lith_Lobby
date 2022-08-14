
internal app_context *
MakeAppContext(m_arena *Arena)
{
    app_context *App = PushStructZero(Arena, app_context);
    Sync_MakeSynchronizationObject(&App->Sync);
    App->Arena = Arena;
    App->ExistingGameIDs = MakeHashtable(Arena);
    App->Hosts = MakeHashtable(Arena);
    App->GameIDsEntroy = MakeLinearRandomGenerator(1234);
    App->GeneralEntropy = MakeLinearRandomGenerator(5678);
    return App;
}

internal
WORK_TASK_SIG(ProcessPlayerRequest)
{
    player_input *PlayerInput = (player_input *)Input;
    app_context *App = PlayerInput->App;
    string8 GameID;
    u32 LocalAddress = 0;
    u16 LocalPort = 0;
    OS_NetworkReceiveAllBytes(PlayerInput->Socket, &LocalAddress, sizeof(LocalAddress));
    OS_NetworkReceiveAllBytes(PlayerInput->Socket, &LocalPort, sizeof(LocalPort));
    Log("Player local endpoint is %d:%d", LocalAddress, LocalPort);
    network_result ReceiveResult = OS_NetworkReceiveAllBytes(PlayerInput->Socket, &GameID.size, sizeof(GameID.size));
    if (!ReceiveResult.Error)
    {
        GameID.str = PushArray(PlayerInput->Arena, u8, GameID.size);
        ReceiveResult = OS_NetworkReceiveAllBytes(PlayerInput->Socket, GameID.str, (i32)GameID.size);
        
        if (!ReceiveResult.Error)
        {
            Begin_SyncSection_Read(&App->Sync);
            host *Host = HostStorage_FindHost(&App->Hosts, GameID);
            if (Host)
            {
                // NOTE(fakhri): send player endpoints to host
                OS_NetworkSendAllBytes(Host->Socket, &PlayerInput->PublicAddress, sizeof(PlayerInput->PublicAddress));
                OS_NetworkSendAllBytes(Host->Socket, &PlayerInput->PublicPort, sizeof(PlayerInput->PublicPort));
                OS_NetworkSendAllBytes(Host->Socket, &LocalAddress, sizeof(LocalAddress));
                OS_NetworkSendAllBytes(Host->Socket, &LocalPort, sizeof(LocalPort));
                
                // NOTE(fakhri): send host endpoints to player
                OS_NetworkSendAllBytes(PlayerInput->Socket, &Host->PublicAddress, sizeof(Host->PublicAddress));
                OS_NetworkSendAllBytes(PlayerInput->Socket, &Host->PublicPort, sizeof(Host->PublicPort));
                OS_NetworkSendAllBytes(PlayerInput->Socket, &Host->LocalAddress, sizeof(Host->LocalAddress));
                OS_NetworkSendAllBytes(PlayerInput->Socket, &Host->LocalPort, sizeof(Host->LocalPort));
            }
            End_SyncSection_Read(&App->Sync);
        }
        
    }
    
    OS_CloseSocket(PlayerInput->Socket);
    M_ArenaRelease(PlayerInput->Arena);
}

internal
WORK_TASK_SIG(ProcessHostRequest)
{
    host_context *HostContext = (host_context *)Input;
    app_context *App = HostContext->App;
    
    Begin_SyncSection_Write(&App->Sync);
    string8 GameID;
    
    // NOTE(fakhri): generate a game id
    {
        GameID.str = PushArray(HostContext->Arena, u8, GameIDLength);
        GameID.size = GameIDLength;
        
        // NOTE(fakhri): make a base64 string of StringLength digits
        for(u32 Index = 0;
            Index < GameIDLength;
            ++Index)
        {
            u32 RandomNumber = NextRandomNumberMinMax(&App->GameIDsEntroy, 0, 64);
            GameID.str[Index] = Base64Encoding[RandomNumber];
        }
        
        // TODO(fakhri): should we change the entropy if this case happenes?
        Assert(!Hashtable_Find(&App->ExistingGameIDs, MakeHashtableKey(GameID)));
        Hashtable_Insert(App->Arena, &App->ExistingGameIDs, MakeHashtableKey(GameID), 0);
    }
    
    Log("generated game id: %.*s", Str8Expand(GameID));
    host Host;
    Host.Socket = HostContext->Socket;
    Host.PublicAddress = HostContext->PublicAddress;
    Host.PublicPort = HostContext->PublicPort;
    OS_NetworkReceiveAllBytes(HostContext->Socket, &Host.LocalAddress, sizeof(Host.LocalAddress));
    OS_NetworkReceiveAllBytes(HostContext->Socket, &Host.LocalPort, sizeof(Host.LocalPort));
    HostContext->GameIDHashSlot = HostStorage_InsertHost(App->Arena, &App->Hosts, GameID, &Host);
    End_SyncSection_Write(&App->Sync);
    
    // NOTE(fakhri): let the host know what is his game id
    OS_NetworkSendAllBytes(HostContext->Socket, &GameID.size, sizeof(GameID.size));
    OS_NetworkSendAllBytes(HostContext->Socket, GameID.str, (u32)GameID.size);
    
    // NOTE(fakhri): register the host socket in the completion port to know when the host 
    // is no longer active
    
    struct epoll_event EpollEvent;
    EpollEvent.events = EPOLLIN | EPOLLET;
    EpollEvent.data.ptr = HostContext->EpollContext;
    Assert(!epoll_ctl(HostContext->EpollFD, EPOLL_CTL_ADD, HostContext->Socket, &EpollEvent));
    //M_ArenaRelease(HostContext->Arena);
}
