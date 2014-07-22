// LOGFILE 
#include <AvailabilityMacros.h>

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 29
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>



const char * rotatingLogName(const char *name, const char * path);
	
const char* userLibraryLogDir();

static int startStdErrLog(const char * filename);

static int startStdOutLog(const char * filename);