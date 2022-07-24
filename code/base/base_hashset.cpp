internal u32
HashString(string8 String)
{
    u32 HashValue = 0;
    for (u32 Index = 0;
         Index < String.size;
         ++Index)
    {
        u8 Ch = String.str[Index];
        HashValue += (HashValue * 33) ^ Ch;
    }
    return HashValue;
}

internal u32
StringHashset_InsertKey(m_arena *Arena, string_hashset *StringHashset, string8 Key)
{
    u32 Result = HashTableSize;
    u32 HashValue = HashString(Key);
    for (u32 Offset = 0;
         Offset < ArrayCount(StringHashset->Keys);
         ++Offset)
    {
        u32 KeyIndex = (HashValue + Offset) % ArrayCount(StringHashset->Keys);
        string8 HashtableKey = StringHashset->Keys[KeyIndex];
        if (HashtableKey.size == 0 || Str8Match(Key, HashtableKey, 0))
        {
            // NOTE(fakhri): found
            Result = KeyIndex;
            break;
        }
    }
    
    if (Result < HashTableSize)
    {
        // NOTE(fakhri): insert the key only once
        if (StringHashset->Keys[Result].size == 0)
        {
            Str8ListPush(Arena, &StringHashset->PresentKeys, Key);
        }
        StringHashset->Keys[Result] = Key;
    }
    else
    {
        Assert(!"hashtable is full, try increase the size of the hashtable or change to an external chaining ");
    }
    return Result;
}

internal u32
StringHashset_FindKey(string_hashset *StringHashset, string8 Key)
{
    u32 Result = HashTableSize;
    u32 HashValue = HashString(Key);
    for (u32 Offset = 0;
         Offset < ArrayCount(StringHashset->Keys);
         ++Offset)
    {
        u32 KeyIndex = (HashValue + Offset) % ArrayCount(StringHashset->Keys);
        string8 HashtableKey = StringHashset->Keys[KeyIndex];
        if (HashtableKey.size == 0)
        {
            break;
        }
        if (Str8Match(Key, HashtableKey, 0))
        {
            // NOTE(fakhri): found
            Result = KeyIndex;
            break;
        }
    }
    
    return Result;
}

internal string8_list
GetPresentKeys(string_hashset *HashtableKey)
{
    string8_list Result = HashtableKey->PresentKeys;
    return Result;
}
