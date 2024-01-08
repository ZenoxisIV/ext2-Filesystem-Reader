
# 	CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
# 	Ivan Cassidy Cadiang (2021-12575)
# 	Diego Montenejo (2020-05984)
# 	Rohan Solas (2021-07292)

# Very basic Makefile. May be improved once i get the hang of it.

SRC_DIR := ext2reader

OBJS := main.o extract.o enum.o

ext2: $(OBJS)
	gcc $^ -o $@

%.o: $(SRC_DIR)/%.c $(SRC_DIR)/defs.h
	@gcc -c $<

.PHONY: clean

clean:
	@rm -f *.o ext2