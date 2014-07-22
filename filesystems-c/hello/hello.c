#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <fuse.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <uuid/uuid.h>

static const char  *file_path      = "/hello.txt";
static const char   file_content[] = "Hello World!\n";
static const size_t file_size      = sizeof(file_content)/sizeof(char) - 1;

static uid_t _uid = 0;
static gid_t _gid = 0;

// From /usr/include/stat.h
/*
// * Definitions of flags stored in file flags word.
// *
// * Super-user and owner changeable flags.
// */
//
//#define	UF_SETTABLE		0x0000ffff	/* mask of owner changeable flags */
//#define	UF_NODUMP		0x00000001	/* do not dump file */
//#define	UF_IMMUTABLE	0x00000002	/* file may not be changed */
//#define	UF_APPEND		0x00000004	/* writes to file may only append */
//#define UF_OPAQUE		0x00000008	/* directory is opaque wrt. union */
//
//*
// * The following bit is reserved for FreeBSD.  It is not implemented
// * in Mac OS X.
// */
//* #define UF_NOUNLINK	0x00000010 */	/* file may not be removed or renamed */
//#define UF_COMPRESSED	0x00000020	/* file is hfs-compressed */
//#define UF_TRACKED		0x00000040	/* file renames and deletes are tracked */
//
//* Bits 0x0080 through 0x4000 are currently undefined. */
//#define UF_HIDDEN		0x00008000	/* hint that this item should not be */
//
//* displayed in a GUI */
//
//*
// * Super-user changeable flags.
// */
//#define	SF_SETTABLE		0xffff0000	/* mask of superuser changeable flags */
//#define	SF_ARCHIVED		0x00010000	/* file is archived */
//#define	SF_IMMUTABLE	0x00020000	/* file may not be changed */
//#define	SF_APPEND		0x00040000	/* writes to file may only append */




static int
hello_getattr(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0)
	{ /* The root directory of our file system. */
        stbuf->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        stbuf->st_nlink = 3;
		
		stbuf->st_uid = (__uint16_t)_uid;
		stbuf->st_gid = (__uint16_t)_gid;
		
    } else if (strcmp(path, file_path) == 0) { /* The only file we have. */
		stbuf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
		// | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = file_size;
		//stbuf->st_flags = UF_IMMUTABLE;	 /* LOCKING */
		fprintf(stderr, "uid %x gid %x", _uid, _gid);
		fflush(stderr);
		stbuf->st_uid = (__uint16_t)_uid;
		stbuf->st_gid = (__uint16_t)_gid;

    } else /* We reject everything else. */
        return -ENOENT;

    return 0;
}

static int
hello_open(const char *path, struct fuse_file_info *fi)
{
    if (strcmp(path, file_path) != 0) /* We only recognize one file. */
        return -ENOENT;
	
    if ((fi->flags & O_ACCMODE) != O_RDONLY) /* Only reading allowed. */
		return -EACCES;

    return 0;
}

static int
hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
              off_t offset, struct fuse_file_info *fi)
{
    if (strcmp(path, "/") != 0) /* We only recognize the root directory. */
        return -ENOENT;

    filler(buf, ".", NULL, 0);           /* Current directory (.)  */
    filler(buf, "..", NULL, 0);          /* Parent directory (..)  */
    filler(buf, file_path + 1, NULL, 0); /* The only file we have. */

    return 0;
}

static int
hello_read(const char *path, char *buf, size_t size, off_t offset,
           struct fuse_file_info *fi)
{
    if (strcmp(path, file_path) != 0)
        return -ENOENT;

    if (offset >= file_size) /* Trying to read past the end of file. */
        return 0;

    if (offset + size > file_size) /* Trim the read to the file size. */
        size = file_size - offset;

    memcpy(buf, file_content + offset, size); /* Provide the content. */

    return size;
}

static struct fuse_operations hello_filesystem_operations = {
    .getattr = hello_getattr, /* To provide size, permissions, etc. */
    .open    = hello_open,    /* To enforce read-only access.       */
    .read    = hello_read,    /* To provide file content.           */
    .readdir = hello_readdir, /* To provide directory listing.      */
};

int
main(int argc, char **argv)
{
	//struct passwd * pwd = getpwent();
	//const char * name = pwd->pw_name;
	_uid = getuid();//pwd->pw_uid;
	_gid = getgid();//pwd->pw_gid;
	fprintf(stdout,"\n uid %u - gid %u \n", _uid,_gid);
    return fuse_main(argc, argv, &hello_filesystem_operations, NULL);
}
