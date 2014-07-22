
#include <AvailabilityMacros.h>

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 29
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fuse.h>

#define HTABLE_SIZE  63367

// types/-defs
typedef struct       entry_t filet;
typedef struct       table_s table_t;
typedef struct loopback_data lbdata;

// encapsulate private data
struct loopback_data
{
	table_t * hashtable;
};

// entry_t stores the data (flags,path,etc) associated with each file
// typedef to 'filet' to avoid typing 'struct' all the time

// file-entry
struct entry_t
{
	char	 *	path;
	uint32_t 	flags;
	uint32_t	hidx;
	filet	 *	next;
};


// the hashtable
struct table_s
{
	// number of stored objects
	uint32_t	count;
	
	// all associated flags <-> files
	filet * entries[HTABLE_SIZE];
    
	// a linear list with all used indexes in entries;
	uint32_t	indexes[HTABLE_SIZE];
};

#pragma mark -
#pragma mark === Hash-table declaration / implementation ===


/* HASHING - QUICK BASICS
 * 
 * ***  READ  IF  YOU ' RE  NOT  FAMILIAIR  WITH  HASHES  ***
 *
 * Using hashes (and linked lists) you can store and retrieve
 * objects based on keys rather than an numeric index. 
 * When you're working with files for instance, it makes sense
 * to use the path of each file for this purpose.
 * 
 * This is done by using a magic hash-function to convert keys
 * into.. : indexes. The conversion-magic (hashing) is chose in
 * such a way, that the occurrence of different string result 
 * in the same hash-value is reduced as much as possible. But 
 * these 'collisions' WILL occur. We have to therefore be aware
 * that each 'index' in the hash-table can store 1 OR MORE items.
 * This means that finding an object in the hash-table involves a
 * 2 steps process:
 * 
 * 1. Calulate the hash from the key, and look this up in the
 *    hash-table. If that index contains any value at all:
 * 
 * 2. Walk through the chained objects that you've found, to 
 *    find the actual object you're looking for.
 * 
 * the size of our hash-table. It's recommended to use a Prime
 * number, not too close to a power of 2. It's not exactly clear
 * what magnitude 'not too close' is in.
 * i just took a large number that still fits in 16bit, but is 'not
 * too close' to 2^16.
 * 
 */


#pragma mark -

// extern int EMULATE_FLAGS;
// extern int FLAGS_SUPPORTED;
// extern int IS_EMULATING_FLAGS;


/* the hashing algorithm */
extern
uint32_t hash(const char * path);

/* getting flags */
extern
uint32_t flagsforfile(const char * path);

/* setting flags */
extern
int setflagsforfile(const char * path, uint32_t flags);

/* helper function: get the entry corresponding to a path */
extern
filet * entryforfile(const char * path);

/* Create and add a new entry for a path */ 
extern
filet * addentryforfile(const char * path);
