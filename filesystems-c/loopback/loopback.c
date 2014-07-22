/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

*/

/*
 * Loopback OSXFUSE file system in C. Uses the high-level FUSE API.
 * Based on the fusexmp_fh.c example from the Linux FUSE distribution.
 * Amit Singh <http://osxbook.com>
 */

#include <AvailabilityMacros.h>

#if !defined(AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER)
#error "This file system requires Leopard and above."
#endif

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 29
#endif

#define _GNU_SOURCE

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/xattr.h>
#include <sys/attr.h>
#include <sys/param.h>
#include <sys/vnode.h>
#include <pthread.h>

//#include "logfile.h"
#include "hashtable.h"

#if defined(_POSIX_C_SOURCE)
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;
typedef unsigned long  u_long;
#endif

#define G_PREFIX						"org"
#define G_KAUTH_FILESEC_XATTR G_PREFIX	".apple.system.Security"
#define A_PREFIX						"com"
#define A_KAUTH_FILESEC_XATTR A_PREFIX	".apple.system.Security"
#define XATTR_APPLE_PREFIX				"com.apple."

#define STDOUT_LOG_FILE_NAME			"loopback.log"
#define STDERR_LOG_FILE_NAME			"loopback.err.log"

// pthread_mutex_lock(&strlck);
// printf(fmt, ##__VA_ARGS__);
// pthread_mutex_unlock(&strlck);

// int logf=open("/var/log/mxfs.log", O_CREAT|O_WRONLY|O_APPEND);\
// dprintf(logf, "DEBUG >>>>>>> : " fmt "\n", ##__VA_ARGS__); \
// close(logf);\
// logf=0;\

// extern pthread_mutex_t strlck;
// pthread_mutex_t strlck = PTHREAD_MUTEX_INITIALIZER;
// 
// #if LBFS_DEBUG
// #define DBGLOG(fmt, ...) do {\
// 	pthread_mutex_lock(&strlck); \
// 	if(logf==0) logf=open("/var/log/mxfs.log", O_CREAT|O_WRONLY|O_APPEND);\
// 	dprintf(logf, "MXFS: %s " fmt "\n", gettime(), ##__VA_ARGS__); \
// 	pthread_mutex_unlock(&strlck); \
// } while (0);
// #else
// 	#define DBGLOG(...)
// #endif

int EMULATE_FLAGS = 1;
int FLAGS_SUPPORTED = 0;
int IS_EMULATING_FLAGS = 0;


#pragma mark -
#pragma mark TYPES, STRUCTURE

// directory-entry 
struct loopback_dirp
{
	DIR				*	dp;
	struct dirent	*	entry;
	off_t 				offset;
	uint32_t 			hashidx;
};


#pragma mark --- LOOPBACK implementation ---

static struct loopback_dirp *
get_dirp(struct fuse_file_info *fi)
{
	if(fi && fi->fh)
		return (struct loopback_dirp *)(uintptr_t)fi->fh;
	return NULL;
}




static int
reset_linkcount(const char * path, struct stat * stbuf, struct fuse_file_info * fi )
{
	
	int res;
	struct loopback_dirp *dp = get_dirp(fi);	
	
	if( (fi && fi->fh && (res=fstat(fi->fh,stbuf))>-1 )
	|| (         path && (res=lstat(path,  stbuf))>-1 ) )
	{	

		if( (stbuf->st_mode & S_IFDIR) == 0 ) stbuf->st_nlink = 1;
		
		// apply emulated flags if applicable
		if(IS_EMULATING_FLAGS)
		{
			filet * f = entryforfile(path);
			if(f!=NULL) stbuf->st_flags = f->flags;
		}
		return 0;
	}
   
	return -errno;	
}


static int
loopback_getattr(const char *path, struct stat *stbuf)
{
	int res;
	if ( (res = lstat(path, stbuf)) == -1 )
		return -errno;
	return reset_linkcount(path, stbuf, NULL ); 
}

static int
loopback_fgetattr(const char *path, struct stat *stbuf,
				  struct fuse_file_info *fi)
{
	int res;
	(void)path;
   
	if ( (res = fstat(fi->fh, stbuf)) == -1)
		return -errno;
	return reset_linkcount(path,stbuf,fi);
}


// static int loopback_access(const char *path, int mask)
// {
// 	int res;
// 	if( ( res = access(path, mask) ) != 0)
// 		return -errno;
// 	return 0;
// }


static int loopback_readlink(const char *path, char *buf, size_t size)
{
	int res = readlink(path, buf, size - 1);	
	if (res == -1) return -errno;
	buf[res] = '\0';
	return 0;
}



static int
loopback_opendir(const char *path, struct fuse_file_info *fi)
{
	int res;
	filet * f = NULL;
	
	struct loopback_dirp *d = malloc(sizeof(struct loopback_dirp));
	
	if (d == NULL) return -ENOMEM;
	
	d->dp = opendir(path);
	
	if (d->dp == NULL)
	{
		res = -errno;
		free(d);
		return res;
	}
	
	d->offset = 0;
	d->entry = NULL;
	fi->fh = (uintptr_t)d;
	return 0;
}


static int
loopback_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
				 off_t offset, struct fuse_file_info *fi)
{
	(void)path;
	struct loopback_dirp * d = get_dirp(fi);
	
	if (offset != d->offset)
	{
		seekdir(d->dp, offset);
		d->entry = NULL;
		d->offset = offset;
	}

	while (1)
	{
		struct stat st;
		off_t nextoff;
		
		// is it a directory ??
		if (!d->entry)
		{
			d->entry = readdir(d->dp);
			if (!d->entry) break;
		}
		
		memset(&st, 0, sizeof(st));
		st.st_ino = d->entry->d_ino;
		st.st_mode = d->entry->d_type << 12;
		
		
		if(IS_EMULATING_FLAGS)
		{
			uint32_t flags = flagsforfile(path);
			if( flags<UINT32_MAX && flags>0x0 )
				st.st_flags = flags;
		}
				
		nextoff = telldir(d->dp);
		if (filler(buf, d->entry->d_name, &st, nextoff)) break;
		d->entry = NULL;
		d->offset = nextoff;
	}

	return 0;
}

static int
loopback_releasedir(const char *path, struct fuse_file_info *fi)
{
	struct loopback_dirp *d = get_dirp(fi);
	(void)path;
	closedir(d->dp);
	free(d);
	return 0;
}

static int
loopback_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	if (S_ISFIFO(mode)) res = mkfifo(path, mode);
	else res = mknod(path, mode, rdev);
	if (res == -1) return -errno;
	return 0;
}

static int
loopback_mkdir(const char *path, mode_t mode)
{
	int res;
	res = mkdir(path, mode);
	if (res == -1) return -errno;
	return 0;
}

static int
loopback_unlink(const char *path)
{
	int res;
	res = unlink(path);
	if (res == -1) return -errno;
	return 0;
}

static int
loopback_rmdir(const char *path)
{
	int res;
	res = rmdir(path);
	if (res == -1) return -errno;
	return 0;
}

static int
loopback_symlink(const char *from, const char *to)
{
	int res;
	res = symlink(from, to);
	if (res == -1) return -errno;
	return 0;
}

static int
loopback_rename(const char *from, const char *to)
{
	int res;
	res = rename(from, to);
	if (res == -1) return -errno;
	return 0;
}

static int
loopback_exchange(const char *path1, const char *path2, unsigned long options)
{
	int res;
	res = exchangedata(path1, path2, options);
	if (res == -1) return -errno;
	return 0;
}

static int
loopback_link(const char *from, const char *to)
{
	int res;
	res = link(from, to);
	if (res == -1) return -errno;
	return 0;
}

static int
loopback_fsetattr_x(const char *path, struct setattr_x *attr,
					struct fuse_file_info *fi)
{
	int res;
	uid_t uid = -1;
	gid_t gid = -1;
	
	
	// MODE
	if (SETATTR_WANTS_MODE(attr))
	{
		res = lchmod(path, attr->mode);
		if (res == -1) return -errno;
	}
	
	// UID
	if (SETATTR_WANTS_UID(attr))
		uid = attr->uid;
	
	// GID
	if (SETATTR_WANTS_GID(attr))
		gid = attr->gid;
	
	if ((uid != -1) || (gid != -1))
	{
		res = lchown(path, uid, gid);
		if (res == -1) return -errno;
	}

	// SIZE
	if (SETATTR_WANTS_SIZE(attr)) {
		if (fi) res = ftruncate(fi->fh, attr->size);
		else res = truncate(path, attr->size);
		if (res == -1) return -errno;
	}
	
	// TIME-MODIFIED
	if (SETATTR_WANTS_MODTIME(attr))
	{
		struct timeval tv[2];
		if (!SETATTR_WANTS_ACCTIME(attr))
			gettimeofday(&tv[0], NULL);

		else
		{
			tv[0].tv_sec = attr->acctime.tv_sec;
			tv[0].tv_usec = attr->acctime.tv_nsec / 1000;
		}
		tv[1].tv_sec = attr->modtime.tv_sec;
		tv[1].tv_usec = attr->modtime.tv_nsec / 1000;
		
		res = utimes(path, tv);
		
		if (res == -1) return -errno;
	}

	// TIME-CREATED
	if (SETATTR_WANTS_CRTIME(attr))
	{
		struct attrlist attributes;
		attributes.bitmapcount	= ATTR_BIT_MAP_COUNT;
		attributes.reserved		= 0;
		attributes.commonattr	= ATTR_CMN_CRTIME;
		attributes.dirattr		= 0;
		attributes.fileattr		= 0;
		attributes.forkattr		= 0;
		attributes.volattr		= 0;
		
		res = setattrlist(path, &attributes, &attr->crtime, sizeof(struct timespec), FSOPT_NOFOLLOW);
		
		if (res == -1) return -errno;
	}

	if (SETATTR_WANTS_CHGTIME(attr))
	{
		struct attrlist attributes;

		attributes.bitmapcount	= ATTR_BIT_MAP_COUNT;
		attributes.reserved		= 0;
		attributes.commonattr	= ATTR_CMN_CHGTIME;
		attributes.dirattr		= 0;
		attributes.fileattr		= 0;
		attributes.forkattr		= 0;
		attributes.volattr		= 0;
		
		res = setattrlist(path, &attributes, &attr->chgtime, sizeof(struct timespec), FSOPT_NOFOLLOW);
		
		if (res == -1) return -errno;
	}
	
	// TIME-BACKUPED
	if (SETATTR_WANTS_BKUPTIME(attr))
	{
		struct attrlist attributes;

		attributes.bitmapcount	= ATTR_BIT_MAP_COUNT;
		attributes.reserved		= 0;
		attributes.commonattr	= ATTR_CMN_BKUPTIME;
		attributes.dirattr		= 0;
		attributes.fileattr		= 0;
		attributes.forkattr		= 0;
		attributes.volattr		= 0;
		
		res = setattrlist(path, &attributes, &attr->bkuptime, sizeof(struct timespec), FSOPT_NOFOLLOW);
		
		if (res == -1) return -errno;
	}

	// CHFLAGS
	if (SETATTR_WANTS_FLAGS(attr))
	{
		// try setting flags
		
		if( !EMULATE_FLAGS )
		{
			res = lchflags(path, attr->flags);
			return res;
		}
		
		if( FLAGS_SUPPORTED!=0 )
		{
			res = lchflags(path, attr->flags);
			FLAGS_SUPPORTED = (res==0);
			if(FLAGS_SUPPORTED) return res;
		}
		
		if(FLAGS_SUPPORTED==0) 
		{
			IS_EMULATING_FLAGS = 1;
			
			filet * f = NULL;
			if((f=entryforfile(path) )==NULL
				&& (f=addentryforfile(path))==NULL )
				return -errno;
			f->flags = attr->flags;
			if(fi)
				fsync(fi->fh);
			return 0;
		}
		
	}
	return 0;
}

static int
loopback_setattr_x(const char *path, struct setattr_x *attr)
{
	return loopback_fsetattr_x(path, attr, (struct fuse_file_info *)0);
}

static int
loopback_getxtimes(const char *path, struct timespec *bkuptime,
				   struct timespec *crtime)
{
	int res = 0;
	struct attrlist attributes;

	attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
	attributes.reserved	   = 0;
	attributes.commonattr  = 0;
	attributes.dirattr	   = 0;
	attributes.fileattr	   = 0;
	attributes.forkattr	   = 0;
	attributes.volattr	   = 0;
	
	struct xtimeattrbuf {
		uint32_t size;
		struct timespec xtime;
	} __attribute__ ((packed));
	
	struct xtimeattrbuf buf;
	
	attributes.commonattr = ATTR_CMN_BKUPTIME;
	res = getattrlist(path, &attributes, &buf, sizeof(buf), FSOPT_NOFOLLOW);
	
	if (res == 0)
		(void)memcpy(bkuptime, &(buf.xtime), sizeof(struct timespec));
	else
		(void)memset(bkuptime, 0, sizeof(struct timespec));

	attributes.commonattr = ATTR_CMN_CRTIME;
	
	res = getattrlist(path, &attributes, &buf, sizeof(buf), FSOPT_NOFOLLOW);
	
	if (res == 0)
		(void)memcpy(crtime, &(buf.xtime), sizeof(struct timespec));
	else
		(void)memset(crtime, 0, sizeof(struct timespec));

	return 0;
}

static int
loopback_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int fd = open(path, fi->flags, mode);
	if (fd == -1) return -errno;
	fi->fh = fd;
	return 0;
}

static int
loopback_open(const char *path, struct fuse_file_info *fi)
{
	int fd = open(path, fi->flags);
	if (fd == -1) return -errno;
	fi->fh = fd;
	return 0;
}

static int
loopback_read(const char *path, char *buf, size_t size, off_t offset,
			  struct fuse_file_info *fi)
{
	int res = 0;
	(void)path;
	if ( (res=pread(fi->fh, buf, size, offset)) == -1 )
		res = -errno;
	return res;
}

static int
loopback_write(const char *path, const char *buf, size_t size,
			   off_t offset, struct fuse_file_info *fi)
{
	int res;
	(void)path;
	res = pwrite(fi->fh, buf, size, offset);
	if (res == -1) res = -errno;
	return res;
}

static int
loopback_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	res = statvfs(path, stbuf);
	if (res == -1) return -errno;
	return 0;
}

static int
loopback_flush(const char *path, struct fuse_file_info *fi)
{
	int res;
	(void)path;
	res = close(dup(fi->fh));
	if (res == -1) return -errno;
	return 0;
}

static int
loopback_release(const char *path, struct fuse_file_info *fi)
{
	(void)path;
	close(fi->fh);
	return 0;
}

static int
loopback_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
	int res;
	(void)path;
	(void)isdatasync;
	res = fsync(fi->fh);
	if (res == -1) return -errno;
	return 0;
}

static int
loopback_setxattr(const char *path, const char *name, const char *value,
				  size_t size, int flags, uint32_t position)
{
	int res;
	if (!strncmp(name, XATTR_APPLE_PREFIX, sizeof(XATTR_APPLE_PREFIX) - 1))
		flags &= ~(XATTR_NOSECURITY);

	if (!strcmp(name, A_KAUTH_FILESEC_XATTR))
	{
		char new_name[MAXPATHLEN];
		memcpy(new_name, A_KAUTH_FILESEC_XATTR, sizeof(A_KAUTH_FILESEC_XATTR));
		memcpy(new_name, G_PREFIX, sizeof(G_PREFIX) - 1);
		res = setxattr(path, new_name, value, size, position, XATTR_NOFOLLOW);
	}
	else
		res = setxattr(path, name, value, size, position, XATTR_NOFOLLOW);
	
	if (res == -1)
		return -errno;

	return 0;
}

static int
loopback_getxattr(const char *path, const char *name, char *value, size_t size,
				  uint32_t position)
{
	int res;
	if (strcmp(name, A_KAUTH_FILESEC_XATTR) == 0)
	{
		char new_name[MAXPATHLEN];
		memcpy(new_name, A_KAUTH_FILESEC_XATTR, sizeof(A_KAUTH_FILESEC_XATTR));
		memcpy(new_name, G_PREFIX, sizeof(G_PREFIX) - 1);
		res = getxattr(path, new_name, value, size, position, XATTR_NOFOLLOW);
	}
	else
		res = getxattr(path, name, value, size, position, XATTR_NOFOLLOW);
	
	if (res == -1)
		return -errno;
	
	return res;
}

static int
loopback_listxattr(const char *path, char *list, size_t size)
{
	ssize_t res = listxattr(path, list, size, XATTR_NOFOLLOW);
	if (res > 0)
	{
		if (list)
		{
			size_t len = 0;
			char *curr = list;
			do
			{
				size_t thislen = strlen(curr) + 1;
				if (strcmp(curr, G_KAUTH_FILESEC_XATTR) == 0)
				{
					memmove(curr, curr + thislen, res - len - thislen);
					res -= thislen;
					break;
				}
				curr += thislen;
				len += thislen;
			}
			while (len < res);
		}
		else
		{
			/* WHY IS THIS COMMENTED-OUT?
			ssize_t res2 = getxattr(path, G_KAUTH_FILESEC_XATTR, NULL, 0, 0, XATTR_NOFOLLOW);
			if (res2 >= 0) res -= sizeof(G_KAUTH_FILESEC_XATTR);
			*/
		}
	}
	
	if (res == -1) return -errno;
	return res;
}

static int
loopback_removexattr(const char *path, const char *name)
{
	int res;
	if (strcmp(name, A_KAUTH_FILESEC_XATTR) == 0)
	{
		char new_name[MAXPATHLEN];
		memcpy(new_name, A_KAUTH_FILESEC_XATTR, sizeof(A_KAUTH_FILESEC_XATTR));
		memcpy(new_name, G_PREFIX, sizeof(G_PREFIX) - 1);
		res = removexattr(path, new_name, XATTR_NOFOLLOW);
	}
	else res = removexattr(path, name, XATTR_NOFOLLOW);
	if (res == -1) return -errno;
	return 0;
}

#if FUSE_VERSION >= 29

static int
loopback_fallocate(const char *path, int mode, off_t offset, off_t length,
				   struct fuse_file_info *fi)
{
	fstore_t fstore;
	if (!(mode & PREALLOCATE)) return -ENOTSUP;
	fstore.fst_flags = 0;
	
	if (mode & ALLOCATECONTIG)
		fstore.fst_flags |= F_ALLOCATECONTIG;

	if (mode & ALLOCATEALL)
		fstore.fst_flags |= F_ALLOCATEALL;

	if (mode & ALLOCATEFROMPEOF) fstore.fst_posmode = F_PEOFPOSMODE;
	else if (mode & ALLOCATEFROMVOL) fstore.fst_posmode = F_VOLPOSMODE;

	fstore.fst_offset = offset;
	fstore.fst_length = length;
	
	if (fcntl(fi->fh, F_PREALLOCATE, &fstore) == -1)
		return -errno;
	
	return 0;
}

#endif /* FUSE_VERSION >= 29 */

void *
loopback_init(struct fuse_conn_info *conn)
{	
	FUSE_ENABLE_SETVOLNAME(conn);
	FUSE_ENABLE_XTIMES(conn);
	struct fuse_context* context = fuse_get_context();
	lbdata * data = context->private_data;
	return data;
}


void
loopback_destroy(void *userdata)
{
	filet	* f = NULL;
	lbdata	* d = (lbdata*)(uintptr_t)userdata;
	table_t	* table = (table_t *)d->hashtable;
	uint32_t n = table->count, idx = 0;
	printf("Nr of Objects in hashtable %u \n",n);
	// while(n-->0)
	// {
	// 	idx = table->indexes[n];
	// 	printf("\nBucket: %x \n",idx);
	// 	f = table->entries[idx];
	// 	do
	// 	{
	// 		printf("\tfilet %s \n",f->path);
	// 	}
	// 	while((f=f->next));
	// 	printf("\n");
	// }
}


static struct fuse_operations loopback_oper = {
	.init		 = loopback_init,
	.destroy	 = loopback_destroy,
	.getattr	 = loopback_getattr,
	.fgetattr	 = loopback_fgetattr,
	// .access		 = loopback_access, 
	.readlink	 = loopback_readlink,
	.opendir	 = loopback_opendir,
	.readdir	 = loopback_readdir,
	.releasedir	 = loopback_releasedir,
	.mknod		 = loopback_mknod,
	.mkdir		 = loopback_mkdir,
	.symlink	 = loopback_symlink,
	.unlink		 = loopback_unlink,
	.rmdir		 = loopback_rmdir,
	.rename		 = loopback_rename,
	.link		 = loopback_link,
	.create		 = loopback_create,
	.open		 = loopback_open,
	.read		 = loopback_read,
	.write		 = loopback_write,
	.statfs		 = loopback_statfs,
	.flush		 = loopback_flush,
	.release	 = loopback_release,
	.fsync		 = loopback_fsync,
	.setxattr	 = loopback_setxattr,
	.getxattr	 = loopback_getxattr,
	.listxattr	 = loopback_listxattr,
	.removexattr = loopback_removexattr,
	.exchange	 = loopback_exchange,
	.getxtimes	 = loopback_getxtimes,
	.setattr_x	 = loopback_setattr_x,
	.fsetattr_x	 = loopback_fsetattr_x,
#if FUSE_VERSION >= 29
	.fallocate	 = loopback_fallocate,
#endif
};

// #define LB_OPT(t, p, v) { t, offsetof(struct_ddumb_param, p), v }
// 
// static int loopback_process_options(void *d, const char *arg, int key, struct fuse_args *outargs)
// {
// 	switch(key)
// 	{
// 		case FUSE_OPT_KEY_NONOPT: break;
// 		case LOOPBACKKEY_HELP:
// 			printf("Print HELP...\n");
// 			return 0;
// 			break;
// 		//case LOOPBACKKEY_F:
// 		case LOOPBACKKEY_FLAGS:
// 			EMULATE_FLAGS = 1;
// 			printf(" *** ENABLING FLAG EMULATION *** \n");
// 			return 0;
// 			break;
// 		default:
// 			return 0;
// 			break;
// 	}
// 	
// 	return 0;
// }











int
main(int argc, char *argv[])
{
	umask(0);
		
	//startStdOutLog(STDOUT_LOG_FILE_NAME);
	//startStdErrLog(STDERR_LOG_FILE_NAME);
	
	// initialize private data object
	int n = HTABLE_SIZE;
	lbdata * d = malloc(sizeof(lbdata));
	d->hashtable = malloc(sizeof(table_t));
	while(n-->0) d->hashtable->entries[n]=NULL;
	
	
	// startStdErrLog(STDOUT_LOG_FILE_NAME);
	// startStdOutLog(STDERR_LOG_FILE_NAME);
	return fuse_main(argc, argv, &loopback_oper, d);
}










