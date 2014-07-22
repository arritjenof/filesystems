#include <string.h>
#include "logfile.h"




// test file
static int
fileExists(const char * path, int *isDir)
{
	int err=0, isfile = 0, isdir = 0, isblck = 0;
	struct stat file;
	
	if( (err=stat(path, &file)) < 0 )
		return 0;
		
	isfile	= S_ISREG(file.st_mode);
	isdir	= S_ISDIR(file.st_mode);
	isblck	= S_ISBLK(file.st_mode);
	
	if( isDir!=NULL && (isfile||isdir||isblck) )
		*isDir = isdir;
	
	return (isfile||isdir);
}




const char *
rotatingLogName(const char *proposedName, const char * path)
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
		free(logdir);
		int islogdir = 0;
		if( (fileExists(lblogdir, &islogdir)!=0) )
		{
			mode_t mode = 0000755;
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
	printf("%s/%s/%s",userLibraryLogDir(),rotatingLogName(filename),filename);
	
	return 0;
}
static int startStdOutLog(const char * filename)
{
	const char * logdir = userLibraryLogDir();
	
	return 0;
}

