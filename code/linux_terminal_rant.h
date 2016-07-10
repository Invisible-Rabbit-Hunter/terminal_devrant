#ifndef LINUX_TERMINAL_RANT_H
#define LINUX_TERMINAL_RANT_H

#define MOD(a,b) ((((a)%(b))+(b))%(b))

struct string
{
  char *pointer;
  size_t length;
};

struct Rant
{
    char URL[50];

    char Users[101][100];

    char Text[5000];

    char Comments[100][1000];
    int CommentCount;
};

#endif
