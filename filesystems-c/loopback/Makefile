TARGETS = loopback loopback.o hashtable.o #logfile.o
CC ?= gcc
CFLAGS_OSXFUSE = -mmacosx-version-min=10.7 -D_FILE_OFFSET_BITS=64 -I/usr/local/include/osxfuse/fuse
CFLAGS_EXTRA = -g $(CFLAGS)
#LIBS = -losxfuse
LDFLAGS=-L/usr/local/lib -losxfuse

#-Wall
#.c: $(CC) -c $(CFLAGS_OSXFUSE) $(CFLAGS_EXTRA) -o $@ $< $(LIBS)

all: info

loopback.o: loopback.c
	$(CC) -c $(CFLAGS_OSXFUSE) $(CFLAGS_EXTRA) -o $@ $<

hashtable.o: hashtable.c hashtable.h
	$(CC) -c $(CFLAGS_OSXFUSE) $(CFLAGS_EXTRA) -o $@ $<

#logfile.o: logfile.c logfile.h
#	$(CC) -c $(CFLAGS_OSXFUSE) $(CFLAGS_EXTRA) -o $@ $<

loopback: loopback.o hashtable.o
	$(CC) $(CFLAGS_OSXFUSE) $(CFLAGS_EXTRA) -o $@ $^ $(LDFLAGS)

info: $(TARGETS)
	@echo
	@echo DONE!

clean:
	rm -f $(TARGETS) *.o
