/*
Complete! >w<
*/

#include "cJSON.h"
#include <stdio.h>
#include <string.h>

typedef struct 
{
	char siteName[64];
	char banChars[64];
	char reqChars[64];
	unsigned int pasCount;
} siteStruct;

int writeJSON(siteStruct *info, char *directory) 
{
	// Create JSON for the site	
	cJSON *site = cJSON_CreateObject();
	cJSON_AddItemToObject(site, "name", cJSON_CreateString(info->siteName));
	cJSON_AddItemToObject(site, "passwordChangeCounter", cJSON_CreateNumber(info->pasCount));
	cJSON_AddItemToObject(site, "bannedCharacters", cJSON_CreateString(info->banChars));
	cJSON_AddItemToObject(site, "requiredCharacters", cJSON_CreateString(info->reqChars));
	char * str = cJSON_Print(site);
	cJSON_Delete(site);

	
	// Create a string with the directory and file name
	char fileName[256] = "";
	strncat(fileName, directory, 64);
	strncat(fileName, info->siteName, 64);
	strcat(fileName, ".json");

	// Open for wrtiting
	FILE *fp = fopen(fileName, "w");
	if(!fp) 
	{
		fprintf(stderr, "There was a problem opening the file: %s\n", fileName);
		return 1;
	}

	// Fill and close file
	fputs(str, fp);
	fclose(fp);
	return 0;
}

int readJSON(siteStruct *returnInfo, char *file) 
{
	// json buffer
	char jsonBuffer[256];

	// Open json file
	FILE *fp = fopen(file, "r");
	if(!fp) {
		fprintf(stderr, "There was a problem opening the file: %s\n", file);
		return 1;
	}

	// Fill the json buffer with the json information
	int c, i;
	for(i = 0; ((c = fgetc(fp)) != EOF) && i < 256; i++) {
		jsonBuffer[i] = (char) c;
	}
	jsonBuffer[i] = '\0';
	
	// Close json file
	fclose(fp);

	// cJSON stuff
	cJSON *siteJSON = cJSON_Parse(jsonBuffer);
	if(!siteJSON) {
		fprintf(stderr, "Error parsing JSON:\n%s\n", cJSON_GetErrorPtr());
		return 1;
	}

	// Fill struct with information from the json
	strncpy(returnInfo->siteName, cJSON_GetObjectItemCaseSensitive(siteJSON, "name")->valuestring, 63);
	returnInfo->pasCount = cJSON_GetObjectItemCaseSensitive(siteJSON, "passwordChangeCounter")->valueint;
	strncpy(returnInfo->banChars, cJSON_GetObjectItemCaseSensitive(siteJSON, "bannedCharacters")->valuestring, 63);
	strncpy(returnInfo->reqChars, cJSON_GetObjectItemCaseSensitive(siteJSON, "requiredCharacters")->valuestring, 63);

	// Delete json
	cJSON_Delete(siteJSON);

	return 0;
}
