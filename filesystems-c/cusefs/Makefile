TARGETS = cusefs
CC ?= gcc
CFLAGS_OSXFUSE = -D_FILE_OFFSET_BITS=64 -I/usr/local/include/osxfuse/fuse
CFLAGS_EXTRA = -g $(CFLAGS)
LDFLAGS=-L/usr/local/lib -losxfuse

all: info

cusefs.o: cusefs.c
	$(CC) -c $(CFLAGS_OSXFUSE) $(CFLAGS_EXTRA) -o $@ $<

cusefs: cusefs.o
	$(CC) $(CFLAGS_OSXFUSE) $(CFLAGS_EXTRA) -o $@ $^ $(LDFLAGS)

info: $(TARGETS)
	@echo
	@echo Ready
	@echo

clean:
	rm -fr $(TARGETS) *.o *.dSYM