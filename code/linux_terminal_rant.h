#ifndef LINUX_TERMINAL_RANT_H
#define LINUX_TERMINAL_RANT_H

#define MOD(a,b) ((((a)%(b))+(b))%(b))
#define true 1
#define false 0

typedef struct string
{
    char *Text;
    size_t Length;
} string;

string
InitString()
{
    string Result = {};
    Result.Text = calloc(0, sizeof(char));
    Result.Length = 0;

    return(Result);
}

int32_t
Append(string *String, char *Text, size_t Size)
{
    String->Text = realloc(String->Text, String->Length + Size + 1);
    strncpy(String->Text + String->Length, Text, Size);
    String->Length += Size;
    return(Size);
}

typedef struct memory
{
    void *Pointer;
    uint32_t Length;
} memory;

memory
Allocate(int32_t size)
{
    memory Result = {};
    Result.Pointer = calloc(size, 1);
    Result.Length = size;

    return(Result);
}

void
Free(memory *Memory)
{
    free(Memory->Pointer);
    Memory->Length = 0;
}

struct rant
{
    string Text;
    struct rant *Comments;
    uint32_t CommentCount;

    struct rant *Next;

    memory Memory;
};

enum linked_list_type
{
    list_type_int,
    list_type_float,
    list_type_string,
    list_type_void_pointer,
    list_type_linked_list,
};

struct linked_list
{
    enum linked_list_type Type;
    union
    {
        int32_t Int;
        float Float;
        string String;
        void *VoidPointer;
        struct linked_list *LinkedList;
    };
};
#endif
