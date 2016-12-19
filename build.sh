
FILES="code/devrant_cli.c"

OUTPUT=build/terminal_devrant

CC=clang

COMPILE_FLAGS="-ggdb"
LINKING_FLAGS="`curl-config --libs` -lm -lsqlite3"

$CC $FILES $COMPILE_FLAGS $LINKING_FLAGS -o $OUTPUT
