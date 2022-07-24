/* date = July 23rd 2022 0:05 am */

#ifndef BASE_HASHSET_H
#define BASE_HASHSET_H

// NOTE(fakhri): power of 2 for now, because programmers...
#define HashTableSize 4096

struct string_hashset
{
    string8 Keys[HashTableSize];
    string8_list PresentKeys;
};

internal u32 HashString(string8 String);
internal u32 StringHashset_InsertKey(m_arena *Arena, string_hashset *StringHashset, string8 Key);
internal u32 StringHashset_FindKey(string_hashset *StringHashset, string8 Key);
internal string8_list GetPresentKeys(string_hashset *HashtableKey);

#endif //BASE_HASHSET_H
