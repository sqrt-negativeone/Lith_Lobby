/* date = July 8th 2022 0:21 pm */

#ifndef BASE_STRING_HASHTABLE_H
#define BASE_STRING_HASHTABLE_H

struct string_hashtable
{
    string_hashset HashtableKeys;
    string8 Values[HashTableSize];
};


#define Hashtable_DefaultInsertFunction(Hashtable, SlotIndex, Value) \
Statement((Hashtable)->Values[SlotIndex] = Value;)

#define Hashtable_InsertF(Arena, Hashtable, Key, Value, InsertFunction)                              \
Statement(u32 SlotIndex = StringHashset_InsertKey(Arena, &((Hashtable)->HashtableKeys), Key);      \
if (SlotIndex < HashTableSize)                                                           \
{                                                                                        \
InsertFunction(Hashtable, SlotIndex, Value);                                           \
})

#define Hashtable_Insert(Arena, Hashtable, Key, Value) \
Hashtable_InsertF(Arena, Hashtable, Key, Value, Hashtable_DefaultInsertFunction)

#define Hashtable_GetValue(Hashtable, Key, type, Value, NotFoundValue)                               \
type Value = NotFoundValue;                                                                        \
Statement(u32 SlotIndex = StringHashset_FindKey(&(Hashtable)->HashtableKeys, Key);                 \
if (SlotIndex < HashTableSize)                                                           \
{                                                                                        \
Value = (Hashtable)->Values[SlotIndex];                                                \
})

#define StringHashtable_GetValue(Hashtable, Key, Value) \
Hashtable_GetValue(Hashtable, Key, string8, Value, ZERO_STRUCT)


#endif //BASE_STRING_HASHTABLE_H
