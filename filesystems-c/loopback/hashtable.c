#include <string.h>
#include "hashtable.h"



/*
 * create hash from path-string
 * - implementation of Jenkins' hash function
 */

uint32_t
hash(const char *path)
 {
	int len = strlen(path);
	uint32_t hash, i;
	for(hash = i = 0; i < len; ++i)
	{
		hash += path[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	hash = hash % HTABLE_SIZE;
	// printf("%x ",hash);
	return hash;
}

/* return the flags of file at path 'path' */

uint32_t
flagsforfile(const char * path)
{
	filet * f = entryforfile(path);
	if(f==NULL)
		return UINT32_MAX;
	return f->flags;
}


/* set (store) flags for a file */
int
setflagsforfile(const char * path, uint32_t flags)
{
	filet * f = entryforfile(path);
	if(f==NULL && (f=addentryforfile(path))==NULL )
		return -1;
	f->flags = flags;
	return 0;	
}


/* Create new filet* and insert in array */
filet *
addentryforfile(const char * path)
{
	filet * f = NULL;
	if((f=entryforfile(path)))
		return f;
	
	lbdata * d = (lbdata*)(uintptr_t)fuse_get_context()->private_data;
	table_t * table = d->hashtable;
	
	if(table==NULL)
		return NULL;
	
	if((f=malloc(sizeof(filet)))==NULL)	
		return NULL;
	
	int len = strlen(path);
	f->path = malloc(sizeof(char)*len+1);
	memcpy(f->path, path, len);
	f->path[len]='\0';
	f->flags = 0x0;
	f->next = NULL;
	
	uint32_t h = hash(path);
	f->hidx = h;
	
	/*
	printf("new object #%u - idx %x - for path %s\n",table->count,h,path);
	 */
	
	if((table->entries[h])) // collision
		f->next = table->entries[h];
	else
	 	table->indexes[table->count] = h;
	
	table->entries[h] = f;
	table->count++;
	
	return f;
}

// helper-function for obtaining a reference to an enrtry (filet)
// 
filet *
entryforfile(const char * path)
{
	// get private data object
	table_t	*table = NULL;
	lbdata	*    d = NULL;
	filet	*    f = NULL;

	if( (d=(lbdata*)(uintptr_t)fuse_get_context()->private_data) == NULL )
		return NULL;
	
	if( (table=(table_t *)d->hashtable)==NULL) 
		return NULL;
	
	uint32_t h = hash(path);
	if(h==0)
	{
		printf("ZER INDEX !!!\n");
		return NULL;
	}
	
	f = (filet*)table->entries[h];
	while(f)
	{
		if( strlen(path)==strlen(f->path) && strcmp(path,f->path)==0 ) 
			return f;
		f = f->next;
	}
	return NULL;
}

