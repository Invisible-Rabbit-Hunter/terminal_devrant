
FILES = code/linux_terminal_rant.cpp

OUTPUT = build/terminal_devrant

CC = clang

COMPILE_FLAGS = -ggdb -W -Wno-unused-parameter `curl-config --cflags` `pkg-config gumbo --cflags` `ncurses5-config --cflags`
LINKING_FLAGS = `curl-config --libs` `pkg-config gumbo --libs` `ncurses5-config --libs`

all: $(FILES)
		$(CC) $(FILES) $(COMPILE_FLAGS) $(LINKING_FLAGS) -o $(OUTPUT)
