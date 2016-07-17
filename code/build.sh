
FILES=code/linux_terminal_devrant.c

OUTPUT=build/terminal_devrant

CC=clang

COMPILE_FLAGS="-ggdb"
LINKING_FLAGS="`curl-config --libs` -lm"

echo $CC $FILES $COMPILE_FLAGS $LINKING_FLAGS -o $OUTPUT
$CC $FILES $COMPILE_FLAGS $LINKING_FLAGS -o $OUTPUT
