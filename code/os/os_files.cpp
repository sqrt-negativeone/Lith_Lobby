
internal b32
OS_MakeDirectory(string8 Path)
{
    b32 Result = 0;
#if defined(IMPL_MakeDirectory)
    Result = IMPL_MakeDirectory(Path);
#else
# error provide implemenation for this
#endif
    return Result;
}

internal string8
OS_LoadEntireFile(m_arena *Arena, string8 Filename)
{
    string8 Result = ZERO_STRUCT;
#if defined(IMPL_LoadEntireFile)
    Result = IMPL_LoadEntireFile(Arena, Filename);
#else
# error provide implemenation for this
#endif
    
    return Result;
}

internal b32
OS_SaveToFile(string8 Filename, string8_list Stream)
{
    b32 Result = 0;
#if defined(IMPL_SaveToFile)
    Result = IMPL_SaveToFile(Filename, Stream);
#else
# error provide implemenation for this
#endif
    return Result;
}

internal b32
OS_SaveStringToFile(string8 Filename, string8 String)
{
    string8_list Stream = ZERO_STRUCT;
    m_temp Scratch = GetScratch(0, 0);
    Str8ListPush(Scratch.Arena, &Stream, String);
    b32 Result = OS_SaveToFile(Filename, Stream);
    ReleaseScratch(Scratch);
    return Result;
}

internal b32
OS_AppendToFile(string8 Path, string8_list Stream)
{
    b32 Result = 0;
#if defined(IMPL_AppendToFile)
    Result = IMPL_AppendToFile(Path, Stream);
#else
# error provide implemenation for this
#endif
    return Result;
}

internal file_info
OS_GetFileInfo(string8 FilePath)
{
    file_info Result = ZERO_STRUCT;
#if defined(IMPL_GetFileInfo)
    Result = IMPL_GetFileInfo(FilePath);
#else
    UnusedVariable(FilePath);
#endif
    return Result;
}

internal void
OS_DeleteFile(string8 Filename)
{
#if defined(IMPL_DeleteFile)
    IMPL_DeleteFile(Filename);
#else
# error provide implemenation for this
#endif
}

internal b32
OS_DoesFileExist(string8 Path)
{
    b32 Result = 0;
#if defined(IMPL_DoesFileExist)
    Result = IMPL_DoesFileExist(Path);
#else
# error provide implemenation for this
#endif
}

internal b32
OS_DoesDirectoryExist(string8 Path)
{
    b32 Result = 0;
#if defined(IMPL_DoesDirectoryExist)
    Result = IMPL_DoesDirectoryExist(Path);
#else
# error provide implemenation for this
#endif
}

internal b32
OS_FileIterBegin(file_iterator *Iterator, string8 Path)
{
    b32 Result = 0;
#if defined(IMPL_FileIterBegin)
    Result = IMPL_FileIterBegin(Iterator, Path);
#else
# error provide implemenation for this
#endif
    return Result;
}

internal file_info
OS_FileIterNext(m_arena *Arena, file_iterator *Iterator)
{
#if defined(IMPL_FileIterNext)
    return IMPL_FileIterNext(Arena, Iterator);
#else
# error provide implemenation for this
#endif
}

internal void
OS_FileIterEnd(file_iterator *Iterator)
{
#if defined(IMPL_FileIterEnd)
    IMPL_FileIterEnd(Iterator);
#else
# error provide implemenation for this
#endif
}

internal string8
OS_GetSystemPath(m_arena *Arena, system_path SystemPath)
{
    string8 Result = ZERO_STRUCT;
#if defined(IMPL_GetSystemPath)
    Result = IMPL_GetSystemPath(Arena, SystemPath);
#else
# error provide implemenation for this
#endif
    return Result;
}
