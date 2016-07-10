#ifndef LINUX_TERMINAL_RANT_H
#define LINUX_TERMINAL_RANT_H

#define MOD(a,b) ((((a)%(b))+(b))%(b))

struct string
{
  char *pointer;
  size_t length;
};

struct Memory
{
    void *Memory;
    size_t Size;
};

struct RantContent
{
    char *User;
    char *Text;
};

struct Rant
{
    char URL[50];

    struct RantContent Content;

    struct RantContent Comments[200];
    int CommentCount;

    bool Loaded;
};

#endif
