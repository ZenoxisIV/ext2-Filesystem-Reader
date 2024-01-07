
# 	CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
# 	Ivan Cassidy Cadiang (2021-12575)
# 	Diego Montenejo (2020-05984)
# 	Rohan Solas (2021-07292)

# Very basic Makefile. May be improved once i get the hang of it.

OBJS = main.o extract.o enum.o

ext2: main.o extract.o enum.o
	gcc $(OBJS) -o ext2

main.o: ext2reader/main.c ext2reader/defs.h
	@gcc -c ext2reader/main.c

extract.o: ext2reader/extract.c ext2reader/defs.h
	@gcc -c ext2reader/extract.c

enum.o: ext2reader/enum.c ext2reader/defs.h
	@gcc -c ext2reader/enum.c

clean:
	@rm *.o