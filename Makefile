
# 	CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
# 	Ivan Cassidy Cadiang (2021-12575)
# 	Diego Montenejo (2020-05984)
# 	Rohan Solas (2021-07292)

# A very basic Makefile.

CC = gcc
CFLAGS = -Wall -g

SRCS = main.c enum.c read.c pathParser.c error.c extract.c
OBJS = $(SRCS:.c=.o)
EXEC = ext2op

$(EXEC): $(OBJS) defs.h
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

%.o: %.c defs.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)