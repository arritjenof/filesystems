TARGETS = memfs

CC ?= gcc
CFLAGS_OSXFUSE = -D_FILE_OFFSET_BITS=64 -I/usr/local/include/osxfuse/fuse
CFLAGS_EXTRA = -g $(CFLAGS)
LIBS = -losxfuse
#-Wall
.c:
	$(CC) $(CFLAGS_OSXFUSE) $(CFLAGS_EXTRA) -o $@ $< $(LIBS)

all: info

memfs: memfs.c

info: $(TARGETS)
	@echo
	@echo Compiled. The following is a typical way to run the memfs file system. In
	@echo this example, /tmp/dir is an existing directory whose contents will become
	@echo available in the existing mount point /Volumes/loop:
	@echo
	@echo "./memfs /Volumes/memfs -omodules=threadid:subdir,subdir=/tmp/dir -oallow_other,native_xattr,volname=MEMFS"
	@echo

clean:
	rm -fr $(TARGETS) *.o *.dSYM
