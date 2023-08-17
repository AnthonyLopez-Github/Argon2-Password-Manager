#ifndef MKJSON
#define MKJSON

typedef struct {
	char siteName[64];
	char banChars[64];
	char reqChars[64];
	unsigned int pasCount;
} siteStruct;

int writeJSON(siteStruct *info, char *directory);
int readJSON(siteStruct *returnInfo, char *file);

#endif
