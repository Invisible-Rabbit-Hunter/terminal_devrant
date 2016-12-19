#include "API/devrant_api.c"

#define COMMAND(name) {#name, cmd_##name}
#define COMMAND_FUNC(name) int cmd_##name(int argc, char **argv)

struct command
{
    char *Name;
    int (*cmd)(int, char **);
};
