// /*
//   FUSE: Filesystem in Userspace
//   Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
// 
//   This program can be distributed under the terms of the GNU GPL.
//   See the file COPYING.
// 
// */
// 
// /*
//  * Loopback OSXFUSE file system in C. Uses the high-level FUSE API.
//  * Based on the fusexmp_fh.c example from the Linux FUSE distribution.
//  * Amit Singh <http://osxbook.com>
//  */
// 
// #include <AvailabilityMacros.h>
// 
// #if !defined(AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER)
// #error "This file system requires Leopard and above."
// #endif
// 
// #define FUSE_USE_VERSION 26
// 
// #define _GNU_SOURCE
// 
// #include <fuse.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <dirent.h>
// #include <errno.h>
// #include <sys/time.h>
// #include <sys/xattr.h>
// #include <sys/attr.h>
// #include <sys/param.h>
// #include <sys/vnode.h>
// #include <math.h>
// #include <sys/time.h>
// 
// #if defined(_POSIX_C_SOURCE)
// typedef unsigned char  u_char;
// typedef unsigned short u_short;
// typedef unsigned int   u_int;
// typedef unsigned long  u_long;
// #endif
// 
// #define G_PREFIX					   "org"
// #define G_KAUTH_FILESEC_XATTR G_PREFIX ".apple.system.Security"
// #define A_PREFIX					   "com"
// #define A_KAUTH_FILESEC_XATTR A_PREFIX ".apple.system.Security"
// #define XATTR_APPLE_PREFIX			   "com.apple."
// 
// 
// static inline double t() { struct timeval tv; gettimeofday(&tv,0); return (double)tv.tv_sec+(((double)tv.tv_usec)/1000000.0); }
// 
// typedef struct loopback_dirp loopback_dirp;
// 
// struct loopback_dirp
// {
//               DIR *dp;
//     struct dirent *entry;
//             off_t offset;
//          uint32_t hash;
// };
// 
// 
// #pragma mark -
// #pragma mark • Hashtable defenitions 
// 
// 
// //#define  BUCKET_MAX 65536
// // #define  HTABLE_SIZE 63367	//63367
// // 
// // typedef struct hentry hentry;
// // typedef struct htable htable;
// // 
// // struct hentry
// // {
// // 	uint32_t flags;
// // 	char path[PATH_MAX];
// //     struct hentry *next;
// // };
// // 
// // struct htable
// // {
// // 	struct hentry * entries[HTABLE_SIZE];
// // };
// // 
// // static struct htable ftable;
// // 
// // static uint32_t
// // hash(const char *path);
// // 
// // static int
// // init_htable(struct htable * ht);
// // 
// // static uint32_t
// // get_flags_for_path(const char * path);
// // 
// // static int
// // set_flags_for_path(const char * path, uint32_t flags);
// // 
// // static struct hentry *
// // get_entry_for_path(const char * path);
// // 
// // static struct hentry *
// // add_entry_for_path(const char * path);
// // 
// // static int
// // del_entry_for_path(const char * path);
// 
// #pragma mark -
// #pragma mark • loopback
// 
// 
// static inline struct loopback_dirp * loopback_get_dirp(struct fuse_file_info *fi)
// {
// 	return (struct loopback_dirp *)(uintptr_t)fi->fh;
// }
// 
// 
// static inline int loopback_reset_linkcount(const char * path, struct stat * stbuf, struct fuse_file_info * fi )
// {
// 	int res;
// 	if( (  fi && (res=fstat(fi->fh, stbuf))>-1 && stbuf ) || (path && (res=lstat(path, stbuf))>-1 && stbuf ) )
// 	{
// 		if( (stbuf->st_mode&S_IFDIR)==0 ) // if not directory
// 		{
// 			stbuf->st_nlink = 1;
// 			//stbuf->st_flags = get_flags_for_path(path);
// 		}
// 		return 0;
// 	}
//    
// 	return -errno;	
// }
// 
// // static ino_t get_inode(const char * path)
// // {
// // 	int ret = 0;
// // 	struct stat sb;
// // 	if((ret=stat(path,&sb)))
// // 	{
// // 		printf("can't stat %s\n",path);
// // 		fflush(stdout);
// // 		return ret;
// // 	}
// // 	return sb.st_ino;
// // }
// 
// 
// static int loopback_getattr(const char *path, struct stat *stbuf)
// {
// 	int res;
// 
// 	if ( (res = lstat(path, stbuf)) == -1 )
// 		return -errno;
// 	
// 	//stbuf->st_flags = get_flags_for_path(path);
// 	return loopback_reset_linkcount(path, stbuf, NULL ); 
// }
// 
// 
// static int loopback_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
// {
// 	int res;
// 	(void)path;
// 
// 	if ( (res = fstat(fi->fh, stbuf)) == -1)
// 		return -errno;
// 	
// 	return loopback_reset_linkcount(path,stbuf,fi);
// }
// 
// // static int loopback_access(const char *path, int mask)
// // {
// // 	int res;
// // 	
// // 	if( ( res = access(path, mask) ) != 0)
// // 		return -errno;
// // 	return 0;
// // }
// 
// 
// static int loopback_readlink(const char *path, char *buf, size_t size)
// {
// 	int res;
// 	
// 	if ( (res = readlink(path, buf, size - 1)) == -1) 
// 		return -errno;
// 	
// 	buf[res] = '\0';
// 
// 	return 0;
// }
// 
// static int loopback_opendir(const char *path, struct fuse_file_info *fi)
// {
// 	int res;
// 	struct loopback_dirp *dp = NULL;
// 	if( (dp=(struct loopback_dirp*)(uintptr_t)fi->fh)==NULL )
// 	{
// 		if ( (dp = malloc(sizeof(struct loopback_dirp*))) == NULL)
// 			return -ENOMEM;	
// 	}
// 	
// 	if ( (dp->dp=opendir(path)) == NULL )
// 	{
// 		res = -errno;
// 		free(dp);
// 		return res;
// 	}
// 	//printf("opendir %s\n",path);
// 	dp->offset = 0;
// 	dp->entry = NULL;
// 	fi->fh = (intptr_t)dp;
// 	//add_entry_for_path(path);
// 	return 0;
// }
// 
// 
// static int loopback_readdir( const char	 *path,
// 								  void	 *buf,
// 						 fuse_fill_dir_t  filler,
// 								   off_t  offset,
// 				   struct fuse_file_info *fi )
// {
// 	struct loopback_dirp *d = NULL;
// 	
// 	if((d=(struct loopback_dirp *)(uintptr_t)fi->fh)==NULL)
// 	{
// 		printf("Can't get dirp %s fileinfo %p",path,fi);
// 		return -1;
// 	}
// 	(void)path;
// 	
// 	if (offset != d->offset)
// 	{
// 		seekdir(d->dp, offset);
// 		d->entry = NULL;
// 		d->offset = offset;
// 	}
// 	
// 	//char pad[PATH_MAX];
// 	
// 	while (1)
// 	{
// 		struct stat st;
// 		off_t nextoff;
// 		if (!d->entry && !(d->entry=readdir(d->dp)) )
// 			break;
// 		memset(&st, 0, sizeof(st));
// 		st.st_ino = d->entry->d_ino;
// 		st.st_flags = 0x00000000;
// 		st.st_mode = d->entry->d_type << 12;
// 		
// 		//printf("readdir %s\n",d->entry->d_name);
// 		//struct hentry * entry = add_entry_for_path(path);
// 		//entry->flags = st.st_flags;
// 		//set_flags_for_path(d->entry->d_name, st.st_flags);
// 		nextoff = telldir(d->dp);
// 		if (filler(buf, d->entry->d_name, &st, nextoff)) break;
// 		d->entry = NULL;
// 		d->offset = nextoff;
// 	}
// 
// 	return 0;
// }
// 
// static int loopback_releasedir(const char *path, struct fuse_file_info *fi)
// {
// 	struct loopback_dirp *d = NULL;
// 
// 	if((d=(struct loopback_dirp *)(uintptr_t)fi->fh)==NULL)
// 	{
// 		printf("fi->fh (%s)=NULL\n",path);
// 		return -errno;
// 	}
// 	(void)path;
// 	closedir(d->dp);
// 	//free(d);
// 	return 0;
// }
// 
// static int loopback_mknod(const char *path, mode_t mode, dev_t rdev)
// {
// 	int res;
// 
// 	if (S_ISFIFO(mode)) {
// 		res = mkfifo(path, mode);
// 	} else {
// 		res = mknod(path, mode, rdev);
// 	}
// 
// 	if (res == -1) {
// 		return -errno;
// 	}
// 
// 	return 0;
// }
// 
// static int loopback_mkdir(const char *path, mode_t mode)
// {
// 	int res;
// 
// 	res = mkdir(path, mode);
// 	if (res == -1) {
// 		return -errno;
// 	}
// 
// 	return 0;
// }
// 
// static int loopback_unlink(const char *path)
// {
// 	int res;
// 	
// //	del_entry_for_path(path);
// 	res = unlink(path);
// 	if (res == -1) {
// 		return -errno;
// 	}
// 
// 	return 0;
// }
// 
// static int loopback_rmdir(const char *path)
// {
// 	int res;
// 	//del_entry_for_path(path);
// 	res = rmdir(path);
// 	if (res == -1) {
// 		return -errno;
// 	}
// 
// 	return 0;
// }
// 
// static int loopback_symlink(const char *from, const char *to)
// {
// 	int res;
// 
// 	res = symlink(from, to);
// 	if (res == -1) {
// 		return -errno;
// 	}
// 
// 	return 0;
// }
// 
// static int loopback_rename(const char *from, const char *to)
// {
// 	int res;
// //	struct hentry * entry = get_entry_for_path(from);
// 	//uint32_t flags = 0;//entry->flags;
// 	res = rename(from, to);
// 	if (res == -1) {
// 		return -errno;
// 	}
// 	//del_entry_for_path(from);
// 	//struct hentry *nentry = add_entry_for_path(to);
// 	//nentry->flags = flags;
// 	return 0;
// }
// 
// static int loopback_exchange(const char *path1, const char *path2, unsigned long options)
// {
// 	int res;
// 	//uint32_t flags1, flags2;
// 	
// 	res = exchangedata(path1, path2, options);
// 	if (res == -1) {
// 		return -errno;
// 	}
// 
// 	return 0;
// }
// 
// static int loopback_link(const char *from, const char *to)
// {
// 	int res;
// 
// 	res = link(from, to);
// 	if (res == -1) {
// 		return -errno;
// 	}
// 
// 	return 0;
// }
// 
// 
// 
// 
// 
// 
// //struct fuse_file_info
// //{
// //	int flags;					/** Open flags.	 Available in open() and release() */
// //	unsigned long fh_old;		/** Old file handle, don't use */
// //	int writepage;				/** In case of a write operation indicates if this was caused by a writepage */
// //	unsigned int direct_io : 1; /** Can be filled in by open, to use direct I/O on this file. Introduced in version 2.4 */
// //	unsigned int keep_cache : 1;/** Can be filled in by open, to indicate, that cached file data need not be invalidated.  Introduced in version 2.4 */
// //	unsigned int flush : 1;		/** Indicates a flush operation.  Set in flush operation, also maybe set in highlevel lock operation and lowlevel release operation. Introduced in version 2.6 */
// //	uint64_t fh;				/** File handle.  May be filled in by filesystem in open(). Available in all other file operations */
// //	uint64_t lock_owner;		/** Lock owner id. Available in locking operations and flush */
// //#ifdef __APPLE__
// //	/** Padding.  Do not use*/
// //	unsigned int padding : 27;
// //	unsigned int purge_attr : 1;
// //	unsigned int purge_ubc : 1;
// //#else
// //	/** Padding.  Do not use*/
// //	unsigned int padding : 29;
// //#endif
// //};
// 
// 
// 
// 
// static int loopback_fsetattr_x(const char *path, struct setattr_x *attr, struct fuse_file_info *fi)
// {
// 	int res;
// 	uid_t uid = -1;
// 	gid_t gid = -1;
// 	
// 	if (SETATTR_WANTS_MODE(attr))
// 	{
// 		
// 		res = lchmod(path, attr->mode);
// 		if (res == -1) {
// 			return -errno;
// 		}
// 	}
// 
// 	if (SETATTR_WANTS_UID(attr))
// 	{
// 		uid = attr->uid;
// 	}
// 
// 	if (SETATTR_WANTS_GID(attr))
// 	{
// 		gid = attr->gid;
// 	}
// 
// 	if ((uid != -1) || (gid != -1))
// 	{
// 		res = lchown(path, uid, gid);
// 		if (res == -1) {
// 			return -errno;
// 		}
// 	}
// 
// 	if (SETATTR_WANTS_SIZE(attr)) {
// 		if (fi) {
// 			res = ftruncate(fi->fh, attr->size);
// 		} else {
// 			res = truncate(path, attr->size);
// 		}
// 		if (res == -1) {
// 			return -errno;
// 		}
// 	}
// 
// 	if (SETATTR_WANTS_MODTIME(attr)) {
// 		struct timeval tv[2];
// 		if (!SETATTR_WANTS_ACCTIME(attr)) {
// 			gettimeofday(&tv[0], NULL);
// 		} else {
// 			tv[0].tv_sec = attr->acctime.tv_sec;
// 			tv[0].tv_usec = attr->acctime.tv_nsec / 1000;
// 		}
// 		tv[1].tv_sec = attr->modtime.tv_sec;
// 		tv[1].tv_usec = attr->modtime.tv_nsec / 1000;
// 		res = utimes(path, tv);
// 		if (res == -1) {
// 			return -errno;
// 		}
// 	}
// 
// 	if (SETATTR_WANTS_CRTIME(attr)) {
// 		struct attrlist attributes;
// 
// 		attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
// 		attributes.reserved = 0;
// 		attributes.commonattr = ATTR_CMN_CRTIME;
// 		attributes.dirattr = 0;
// 		attributes.fileattr = 0;
// 		attributes.forkattr = 0;
// 		attributes.volattr = 0;
// 
// 		res = setattrlist(path, &attributes, &attr->crtime,
// 				  sizeof(struct timespec), FSOPT_NOFOLLOW);
// 
// 		if (res == -1) {
// 			return -errno;
// 		}
// 	}
// 
// 	if (SETATTR_WANTS_CHGTIME(attr)) {
// 		struct attrlist attributes;
// 
// 		attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
// 		attributes.reserved = 0;
// 		attributes.commonattr = ATTR_CMN_CHGTIME;
// 		attributes.dirattr = 0;
// 		attributes.fileattr = 0;
// 		attributes.forkattr = 0;
// 		attributes.volattr = 0;
// 
// 		res = setattrlist(path, &attributes, &attr->chgtime,
// 				  sizeof(struct timespec), FSOPT_NOFOLLOW);
// 
// 		if (res == -1) {
// 			return -errno;
// 		}
// 	}
// 
// 	if (SETATTR_WANTS_BKUPTIME(attr)) {
// 		struct attrlist attributes;
// 
// 		attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
// 		attributes.reserved = 0;
// 		attributes.commonattr = ATTR_CMN_BKUPTIME;
// 		attributes.dirattr = 0;
// 		attributes.fileattr = 0;
// 		attributes.forkattr = 0;
// 		attributes.volattr = 0;
// 
// 		res = setattrlist(path, &attributes, &attr->bkuptime,
// 				  sizeof(struct timespec), FSOPT_NOFOLLOW);
// 
// 		if (res == -1) {
// 			return -errno;
// 		}
// 	}
// 
// 	if (SETATTR_WANTS_FLAGS(attr))
// 	{
// 		if ((res=lchflags(path, attr->flags))==-1)
// 		{
// 			printf("can't change flags on file - using pseudo-/fuse-flags\n");
// 			printf("can't get dirp for path %s\n ",path);
// 			
// 			//set_flags_for_path(path,attr->flags);
// 			//struct hentry * entry = get_entry_for_path(path);
// 			//entry->flags = attr->flags;
// 		}
// 	}
// 	return 0;
// }
// 
// static int loopback_setattr_x(const char *path, struct setattr_x *attr)
// {
// 	//printf("*** setattr_x %s\n",path);
// 	return loopback_fsetattr_x(path, attr, (struct fuse_file_info *)0);
// }
// 
// static int loopback_getxtimes(const char *path, struct timespec *bkuptime, struct timespec *crtime)
// {
// 	int res = 0;
// 	struct attrlist attributes;
// 
// 	attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
// 	attributes.reserved	   = 0;
// 	attributes.commonattr  = 0;
// 	attributes.dirattr	   = 0;
// 	attributes.fileattr	   = 0;
// 	attributes.forkattr	   = 0;
// 	attributes.volattr	   = 0;
// 
// 
// 
// 	struct xtimeattrbuf {
// 		uint32_t size;
// 		struct timespec xtime;
// 	} __attribute__ ((packed));
// 
// 
// 	struct xtimeattrbuf buf;
// 
// 	attributes.commonattr = ATTR_CMN_BKUPTIME;
// 	res = getattrlist(path, &attributes, &buf, sizeof(buf), FSOPT_NOFOLLOW);
// 	if (res == 0) {
// 		(void)memcpy(bkuptime, &(buf.xtime), sizeof(struct timespec));
// 	} else {
// 		(void)memset(bkuptime, 0, sizeof(struct timespec));
// 	}
// 
// 	attributes.commonattr = ATTR_CMN_CRTIME;
// 	res = getattrlist(path, &attributes, &buf, sizeof(buf), FSOPT_NOFOLLOW);
// 	if (res == 0) {
// 		(void)memcpy(crtime, &(buf.xtime), sizeof(struct timespec));
// 	} else {
// 		(void)memset(crtime, 0, sizeof(struct timespec));
// 	}
// 
// 	return 0;
// }
// 
// static int loopback_create(const char *path, mode_t mode, struct fuse_file_info *fi)
// {
// 	int fd;
// 
// 	fd = open(path, fi->flags, mode);
// 	if (fd == -1)
// 		return -errno;
// 	fi->fh = fd;
// 	return 0;
// }
// 
// static int loopback_open(const char *path, struct fuse_file_info *fi)
// {
// 	int fd;
// 	fd = open(path, fi->flags);
// 	if (fd == -1)
// 		return -errno;
// 	
// 	fi->fh = fd;
// 	return 0;
// }
// 
// static int loopback_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
// {
// 	int res;
// 	(void)path;
// 	res = pread(fi->fh, buf, size, offset);
// 	if (res == -1)
// 		res = -errno;
// 
// 	return res;
// }
// 
// static int loopback_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
// {
// 	int res;
// 
// 	(void)path;
// 
// 	res = pwrite(fi->fh, buf, size, offset);
// 	if (res == -1)
// 		res = -errno;
// 	
// 	return res;
// }
// 
// static int loopback_statfs(const char *path, struct statvfs *stbuf)
// {
// 	int res;
// 
// 	res = statvfs(path, stbuf);
// 	if (res == -1)
// 		return -errno;
// 	
// 	return 0;
// }
// 
// static int loopback_flush(const char *path, struct fuse_file_info *fi)
// {
// 	int res;
// 	(void)path;
// 	res = close(dup(fi->fh));
// 	if (res == -1)
// 	{
// 		return -errno;
// 	}
// 	return 0;
// }
// 
// static int loopback_release(const char *path, struct fuse_file_info *fi)
// {
// 	(void)path;
// 	close(fi->fh);
// 	return 0;
// }
// 
// static int loopback_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
// {
// 	int res;
// 
// 	(void)path;
// 
// 	(void)isdatasync;
// 
// 	res = fsync(fi->fh);
// 	if (res == -1)
// 	{
// 		return -errno;
// 	}
// 
// 	return 0;
// }
// 
// static int loopback_setxattr(const char *path, const char *name, const char *value, size_t size, int flags, uint32_t position)
// {
// 	int res;
// 
// 	if (!strncmp(name, XATTR_APPLE_PREFIX, sizeof(XATTR_APPLE_PREFIX) - 1)) {
// 		flags &= ~(XATTR_NOSECURITY);
// 	}
// 
// 	if (!strcmp(name, A_KAUTH_FILESEC_XATTR))
// 	{
// 		char new_name[MAXPATHLEN];
// 		memcpy(new_name, A_KAUTH_FILESEC_XATTR, sizeof(A_KAUTH_FILESEC_XATTR));
// 		memcpy(new_name, G_PREFIX, sizeof(G_PREFIX) - 1);
// 		res = setxattr(path, new_name, value, size, position, XATTR_NOFOLLOW);
// 	}
// 	else
// 	{
// 		res = setxattr(path, name, value, size, position, XATTR_NOFOLLOW);
// 	}
// 
// 	if (res == -1)
// 	{
// 		return -errno;
// 	}
// 
// 	return 0;
// }
// 
// static int loopback_getxattr(const char *path, const char *name, char *value, size_t size, uint32_t position)
// {
// 	int res;
// 
// 	if (strcmp(name, A_KAUTH_FILESEC_XATTR) == 0)
// 	{
// 
// 		char new_name[MAXPATHLEN];
// 		memcpy(new_name, A_KAUTH_FILESEC_XATTR, sizeof(A_KAUTH_FILESEC_XATTR));
// 		memcpy(new_name, G_PREFIX, sizeof(G_PREFIX) - 1);
// 		res = getxattr(path, new_name, value, size, position, XATTR_NOFOLLOW);
// 	}
// 	else
// 	{
// 		res = getxattr(path, name, value, size, position, XATTR_NOFOLLOW);
// 	}
// 
// 	if (res == -1)
// 	{
// 		return -errno;
// 	}
// 
// 	return res;
// }
// 
// static int loopback_listxattr(const char *path, char *list, size_t size)
// {
// 	ssize_t res = listxattr(path, list, size, XATTR_NOFOLLOW);
// 	if (res > 0)
// 	{
// 		if (list)
// 		{
// 			size_t len = 0;
// 			char *curr = list;
// 			do
// 			{
// 				size_t thislen = strlen(curr) + 1;
// 				if (strcmp(curr, G_KAUTH_FILESEC_XATTR) == 0)
// 				{
// 					memmove(curr, curr + thislen, res - len - thislen);
// 					res -= thislen;
// 					break;
// 				}
// 				curr += thislen;
// 				len += thislen;
// 			}
// 			while (len < res);
// 		}
// 		else
// 		{
// 			//ssize_t res2 = getxattr(path, G_KAUTH_FILESEC_XATTR, NULL, 0, 0, XATTR_NOFOLLOW);
// 			//if (res2 >= 0) res -= sizeof(G_KAUTH_FILESEC_XATTR);
// 		}
// 	}
// 
// 	if (res == -1)
// 	{
// 		return -errno;
// 	}
// 	return res;
// }
// 
// static int loopback_removexattr(const char *path, const char *name)
// {
// 	int res;
// 
// 	if (strcmp(name, A_KAUTH_FILESEC_XATTR) == 0) {
// 
// 		char new_name[MAXPATHLEN];
// 
// 		memcpy(new_name, A_KAUTH_FILESEC_XATTR, sizeof(A_KAUTH_FILESEC_XATTR));
// 		memcpy(new_name, G_PREFIX, sizeof(G_PREFIX) - 1);
// 
// 		res = removexattr(path, new_name, XATTR_NOFOLLOW);
// 
// 	} else {
// 		res = removexattr(path, name, XATTR_NOFOLLOW);
// 	}
// 
// 	if (res == -1) {
// 		return -errno;
// 	}
// 
// 	return 0;
// }
// 
// #if FUSE_VERSION >= 29
// 
// static int loopback_fallocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi)
// {
// 	fstore_t fstore;
// 
// 	if (!(mode & PREALLOCATE)) {
// 		return -ENOTSUP;
// 	}
// 	
// 	fstore.fst_flags = 0;
// 	if (mode & ALLOCATECONTIG) {
// 		fstore.fst_flags |= F_ALLOCATECONTIG;
// 	}
// 	if (mode & ALLOCATEALL) {
// 		fstore.fst_flags |= F_ALLOCATEALL;
// 	}
// 
// 	if (mode & ALLOCATEFROMPEOF) {
// 		fstore.fst_posmode = F_PEOFPOSMODE;
// 	} else if (mode & ALLOCATEFROMVOL) {
// 		fstore.fst_posmode = F_VOLPOSMODE;
// 	}
// 
// 	fstore.fst_offset = offset;
// 	fstore.fst_length = length;
// 
// 	if (fcntl(fi->fh, F_PREALLOCATE, &fstore) == -1) {
// 		return -errno;
// 	} else {
// 		return 0;
// 	}
// }
// 
// #endif /* FUSE_VERSION >= 29 */
// 
// void *
// loopback_init(struct fuse_conn_info *conn)
// {
// 	FUSE_ENABLE_SETVOLNAME(conn);
// 	FUSE_ENABLE_XTIMES(conn);
// 
// 	struct fuse_context* context = fuse_get_context();
//   	return context->private_data;
// 	//return (void*)&ftable;
// }
// 
// void
// loopback_destroy(void *userdata)
// {
// 	/* nothing */
// }
// 
// 
// 
// 
// static struct fuse_operations loopback_oper =
// {
// 	.init		 = loopback_init,
// 	.destroy	 = loopback_destroy,
// 	.getattr	 = loopback_getattr,
// 	.fgetattr	 = loopback_fgetattr,
// 	//.access		 = loopback_access,
// 	.readlink	 = loopback_readlink,
// 	.opendir	 = loopback_opendir,
// 	.readdir	 = loopback_readdir,
// 	.releasedir	 = loopback_releasedir,
// 	.mknod		 = loopback_mknod,
// 	.mkdir		 = loopback_mkdir,
// 	.symlink	 = loopback_symlink,
// 	.unlink		 = loopback_unlink,
// 	.rmdir		 = loopback_rmdir,
// 	.rename		 = loopback_rename,
// 	.link		 = loopback_link,
// 	.create		 = loopback_create,
// 	.open		 = loopback_open,
// 	.read		 = loopback_read,
// 	.write		 = loopback_write,
// 	.statfs		 = loopback_statfs,
// 	.flush		 = loopback_flush,
// 	.release	 = loopback_release,
// 	.fsync		 = loopback_fsync,
// 	.setxattr	 = loopback_setxattr,
// 	.getxattr	 = loopback_getxattr,
// 	.listxattr	 = loopback_listxattr,
// 	.removexattr = loopback_removexattr,
// 	.exchange	 = loopback_exchange,
// 	.getxtimes	 = loopback_getxtimes,
// 	.setattr_x	 = loopback_setattr_x,
// 	.fsetattr_x	 = loopback_fsetattr_x,
// #if FUSE_VERSION >= 29
// 	.fallocate	 = loopback_fallocate,
// #endif
// };
// 
// int
// main(int argc, char *argv[])
// {
// 	umask(0);
// 	
// 	//ftable = malloc(sizeof(struct htable));
// //	init_htable(&ftable);
// 	return fuse_main(argc, argv, &loopback_oper, NULL);//&ftable);
// }
// 
// 
// 
// 
// 
// 
// 
// #pragma mark -
// #pragma mark • Hashtable
// 
// // static int hash(const char *path) 
// // { 
// // 	printf("hashing %s => ",path);
// // 	int result = 0x55555555;
// // 	while (*input)
// // 	{
// // 		result ^= *input++;
// // 		result = rot(result, 5);
// //     }
// // 	printf("%d\n", hash);
// // 	return result;
// // }
// 
// 
// // static unsigned long hash(const char *path)
// // {
// // 	printf("hashing %s => ",path);
// // 	unsigned long hash = 5381;
// // 	int c;
// // 	
// // 	while ((c=*path++))
// // 	 	hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
// // 	hash = hash % HTABLE_SIZE;
// // 	
// // 	printf("%lu\n", hash);
// // 	return hash;
// // }
// 
// 
// // 
// // static uint32_t hash(const char *path)
// // {
// // 	int len = strlen(path);
// // 	uint32_t hash, i;
// // 	for(hash = i = 0; i < len; ++i)
// // 	{
// // 		hash += path[i];
// // 		hash += (hash << 10);
// // 		hash ^= (hash >> 6);
// // 	}
// // 	hash += (hash << 3);
// // 	hash ^= (hash >> 11);
// // 	hash += (hash << 15);
// // 	hash = hash % HTABLE_SIZE;
// // 	printf("hashing %s: %u\n", path, hash);
// // 	return hash;
// // }
// // 
// // static int init_htable(struct htable * ht)
// // {
// // 	int i = 0;
// // 	for(i=0;i<HTABLE_SIZE;i++)
// // 		ht->entries[i] = NULL;
// // 	return 0;
// // }
// // 
// // static struct hentry *
// // get_entry_for_path(const char * path)
// // {
// // 	uint32_t hsh = hash(path);
// // 	struct hentry * entry = NULL;
// // 	
// // 	struct htable * ht  = NULL;
// // 	if((ht=(struct htable *)(uintptr_t)fuse_get_context()->private_data)==NULL)
// // 	{
// // 		printf("can't get context - get_flags_for_path(%s)\n ",path);
// // 		return NULL;
// // 	}
// // 	
// // 	if( (entry=ht->entries[hsh]) )
// // 	{
// // 		while(entry)
// // 		{
// // 			if( strnstr( entry->path, path, strlen(entry->path) )==0 )
// // 				return entry;
// // 			entry = entry->next;
// // 		}
// // 	}
// // 	return NULL;
// // }
// // 
// // 
// // static struct hentry *
// // add_entry_for_path(const char * path)
// // {
// // 	struct hentry * entry = NULL;
// // 	if((entry=get_entry_for_path(path)))
// // 	{
// // 		printf("Entry exists %s\n",path);
// // 		return entry;
// // 	}
// // 	
// // 	if((entry = malloc(sizeof(struct hentry*)))==NULL)
// // 	{
// // 		printf("Can't create entry %s\n",path);
// // 		return NULL;
// // 	}
// // 	
// // 	struct htable * ht = NULL;
// // 	if(( ht=(struct htable *)fuse_get_context()->private_data)==NULL)
// // 	{
// // 		printf("can't get context - get_flags_for_path(%s)\n ",path);
// // 		return NULL;
// // 	}
// // 	
// // 	int hsh = hash(path);
// // 	printf("path %s\n",path);
// // 	strcpy(entry->path, path);
// // 	entry->next = ht->entries[hsh];
// // 	ht->entries[hsh] = entry;
// // 	return entry;
// // }
// // 
// // static int
// // del_entry_for_path(const char * path)
// // {
// // 	struct hentry * entry = NULL, *e = NULL, *prev = NULL;
// // 	if((entry=get_entry_for_path(path))==NULL)
// // 	{
// // 		printf("Entry not found %s\n",path);
// // 		return -errno;
// // 	}
// // 	struct htable * ht = NULL;
// // 	if(( ht=(struct htable *)fuse_get_context()->private_data)==NULL)
// // 	{
// // 		printf("can't get context - get_flags_for_path(%s)\n ",path);
// // 		return -errno;
// // 	}
// // 	int hsh = hash(path);
// // 	e=ht->entries[hsh];
// // 	while(e)
// // 	{
// // 		if(entry==e)
// // 		{
// // 			
// // 			if(entry->next)
// // 			{
// // 				if(prev)
// // 					prev->next = entry->next;
// // 				else
// // 					ht->entries[hsh] = entry->next;
// // 			}
// // 			entry->next = NULL;
// // 			break;
// // 		}
// // 		prev = e;
// // 		e=e->next;
// // 	}
// // 	free(entry->path);
// // 	entry->flags = 0;
// // 	free(entry);
// // 	entry = NULL;
// // 	return 0;
// // }
// // 
// // 
// // static uint32_t
// // get_flags_for_path(const char * path)
// // {
// // 	struct hentry * entry = NULL;
// // 	if((entry=get_entry_for_path(path))==NULL)
// // 	{
// // 		printf("No Entry for Path %s\n",path);
// // 		return 0;
// // 	}
// // 	return entry->flags;
// // }
// // 
// // 
// // 
// // static int
// // set_flags_for_path(const char * path, uint32_t flags)
// // {
// // 	struct hentry * entry = NULL;
// // 	if( (entry=get_entry_for_path(path))==NULL )
// // 	{
// // 		if((entry=add_entry_for_path(path))==NULL)
// // 		{
// // 			printf("No Entry Created for Path %s\n",path);
// // 			return UINT_MAX;
// // 		}
// // 	}
// // 	entry->flags = flags;
// // 	return 0;
// // }













// // an file-entry (objects in hashtable): stores flags and additional data
// struct entry_t
// {
// 	char	 *	path;
// 	uint32_t 	flags;
// 	uint32_t	hidx;
// 	filet	 *	next;
// };
// 
// // the hashtable etc.
// struct table_s
// {
// 	uint32_t	count;
// 	uint32_t	indexes[HTABLE_SIZE];
// 	filet	*	entries[HTABLE_SIZE];
// };

//static pthread_mutex_t file_lock;


#pragma mark -

// static int EMULATE_FLAGS = 1;	//0
// static int FLAGS_SUPPORTED = 0;	//-1
// static int IS_EMULATING_FLAGS = 0; //0


// /* the hashing algorithm */
// static uint32_t hash(const char * path);
// 
// /* getting flags */
// static uint32_t flagsforfile(const char * path);
// 
// /* setting flags */
// static int setflagsforfile(const char * path, uint32_t flags);
// 
// /* helper function: get the entry corresponding to a path */
// static filet * entryforfile(const char * path);
// 
// /* Create and add a new entry for a path */ 
// static filet * addentryforfile(const char * path);

#pragma mark -

// 
// /*
//  * create hash from path-string
//  * - implementation of Jenkins' hash function
//  */
// 
// static uint32_t
// hash(const char *path)
//  {
// 	int len = strlen(path);
// 	uint32_t hash, i;
// 	for(hash = i = 0; i < len; ++i)
// 	{
// 		hash += path[i];
// 		hash += (hash << 10);
// 		hash ^= (hash >> 6);
// 	}
// 	hash += (hash << 3);
// 	hash ^= (hash >> 11);
// 	hash += (hash << 15);
// 	hash = hash % HTABLE_SIZE;
// 	// printf("%x ",hash);
// 	return hash;
// }
// 
// /* return the flags of file at path 'path' */
// 
// static uint32_t
// flagsforfile(const char * path)
// {
// 	filet * f = entryforfile(path);
// 	if(f==NULL)
// 		return UINT32_MAX;
// 	return f->flags;
// }
// 
// 
// /* set (store) flags for a file */
// static int
// setflagsforfile(const char * path, uint32_t flags)
// {
// 	filet * f = entryforfile(path);
// 	if(f==NULL && (f=addentryforfile(path))==NULL )
// 		return -1;
// 	f->flags = flags;
// 	return 0;	
// }
// 
// 
// /* Create new filet* and insert in array */
// static filet *
// addentryforfile(const char * path)
// {
// 	filet * f = NULL;
// 	if((f=entryforfile(path))) return f;
// 	
// 	lbdata * d = (lbdata*)(uintptr_t)fuse_get_context()->private_data;
// 	table_t * table = d->hashtable;
// 	if(table==NULL) return NULL;
// 	if((f=malloc(sizeof(filet)))==NULL)	return NULL;
// 	
// 	int len = strlen(path);
// 	f->path = malloc(sizeof(char)*len+1);
// 	memcpy(f->path, path, len);
// 	f->path[len]='\0';
// 	f->flags = 0x0;
// 	f->next = NULL;
// 	
// 	uint32_t h = hash(path);
// 	f->hidx = h;
// 	
// 	printf("new object #%u - idx %x - for path %s\n",table->count,h,path);
// 	
// 	if((table->entries[h])) // collision
// 		f->next = table->entries[h];
// 	else
// 	 	table->indexes[table->count] = h;
// 	
// 	table->entries[h] = f;
// 	
// 	table->count++;
// 	return f;
// }
// 
// // helper-function for obtaining a reference to an enrtry (filet)
// // 
// static filet *
// entryforfile(const char * path)
// {
// 	// get private data object
// 	table_t	*table = NULL;
// 	lbdata	*    d = NULL;
// 	filet	*    f = NULL;
// 
// 	if( (d=(lbdata*)(uintptr_t)fuse_get_context()->private_data) == NULL )
// 		return NULL;
// 	
// 	if( (table=(table_t *)d->hashtable)==NULL) 
// 		return NULL;
// 	
// 	uint32_t h = hash(path);
// 	if(h==0)
// 	{
// 		printf("ZER INDEX !!!\n");
// 		return NULL;
// 	}
// 	
// 	f = (filet*)table->entries[h];
// 	while(f)
// 	{
// 		if( strlen(path)==strlen(f->path) && strcmp(path,f->path)==0 ) 
// 			return f;
// 		f = f->next;
// 	}
// 	return NULL;
// }
// 

#pragma mark -
#pragma mark -
#pragma mark -










// test file
static int
fileExists(const char * path, int *isDir)
{
	int err=0, isfile = 0, isdir = 0, isblck = 0;
	struct stat file;
	
	if( (err=stat(path, &file)) < 0 )
	{
		//if(dbg) fprintf( stderr, "stat error:%d %s", err, path );
		return 0;
	}
	
	isfile	= S_ISREG(file.st_mode);
	isdir	= S_ISDIR(file.st_mode);
	isblck	= S_ISBLK(file.st_mode);
	
	//printf("path %s - file:%d,dir:%d,block:%d",path,isfile,isdir,isblck);
	
	if( isDir!=NULL && (isfile||isdir||isblck) )
	*isDir = isdir;
	
	return (isfile||isdir);
}




const char *
rotatingLogName(const char *name, const char * path)
{
	return name;
	//return NULL;
}



const char*
userLibraryLogDir()
{
	static const char * kUSERLIBDIR = "Library";
	static const char * kUSERLOGDIR = "Logs";
	static const char * kLOOPBACKLOGNAME = "nl.filmpartners.mxffs";
	static const char * kLBLOGDIR_CACHE = NULL;
	if(kLBLOGDIR_CACHE!=NULL) 
		return kLBLOGDIR_CACHE;
	
	char * logdir = malloc(sizeof(char)*PATH_MAX);
	char * lblogdir = NULL;
	sprintf(logdir, "%s/%s/%s", getenv("HOME"), kUSERLIBDIR, kUSERLOGDIR);
	
	int isdir = 0;
	if( (  fileExists(logdir, &isdir)==0)  && isdir==1 )
	{
		lblogdir = malloc(sizeof(char)*PATH_MAX);
		sprintf(lblogdir, "%s/%s", logdir, kLOOPBACKLOGNAME);
		int islogdir = 0;
		if( (fileExists(lblogdir, &islogdir)!=0) )
		{
			mode_t mode = 0000766;
			if ( mkdir(lblogdir,mode)!=0 ) 
			{
				printf("Failed to create log directory\n");
				return NULL;
			}
		}
	}
	kLBLOGDIR_CACHE = lblogdir;
	return lblogdir;
}

static int startStdErrLog(const char * filename)
{
	//printf("%s/%s/%s",userLibraryLogDir(),rotatingLogName(filename),filename);
	
	
	return 0;
}
static int startStdOutLog(const char * filename)
{
	const char * logdir = userLibraryLogDir();
	//char * 
	//printf("%s/%s/%s",userLibraryLogDir(),rotatingLogName(filename,),filename);
	return 0;
}

