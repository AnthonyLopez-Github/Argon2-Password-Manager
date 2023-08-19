/*
TODO:
- Add a command for incrementing the password change counter of sites
- Rename some variables to make the code less confusing
- Find and fix bugs
*/

#include "argon2/argon2.h"
#include "psswdmgrjson/mkjson.h"
#include "b64/b64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

typedef struct 
{
	uint32_t t_cost;
	uint32_t m_cost;
	uint32_t parallelism;
} argon2_params;

typedef enum 
{
	CMD_MAKE_SITE = 0,
	CMD_GEN_PASSWD,
	CMD_EXIT,
	CMD_INVALID
} command_t;

char *commandTable[] = 
{
	"make site",
	"gen psswd",
	"exit"
};

void getStringAndExcludeNewline(char *str, size_t size)
{
	char c = 0;
	int i;
	for(i = 0 ; i < size - 1; i++) 
	{
		c = getchar();

		if (c == EOF) break;
		if (c == '\n') break;

		str[i] = c;
	}

	str[i] = '\0';

	// eat to newline
	while (c != '\n') 
	{
		c = getchar();
	}
}

int siteBuildPrompt(void) 
{
	// Directory buffer
	char dir[128];
	snprintf(dir, 128, "%s/.psswdmgr/sites/", getenv("HOME"));

	// Struct with site constraints
	siteStruct site;
	site.pasCount = 0;

	// Set the name of the site
	printf("\nSite name: ");
	getStringAndExcludeNewline(site.siteName, 64);

	// Set the banned characters
	printf("Banned characters: ");
	getStringAndExcludeNewline(site.banChars, 64);

	// Set the required characters
	printf("Required characters: ");
	getStringAndExcludeNewline(site.reqChars, 64);

	printf("\nBuilding JSON...");
	if(writeJSON(&site, dir))
	{
		printf(" Failed\n\n");
		return 1;
	}

	printf(" Done\n\n");

	// Generate an empty password file for the site
	memset(dir, 0, 128);
	snprintf(dir, 128, "%s/.psswdmgr/sites/psswd/%s.txt", getenv("HOME"), site.siteName);

	FILE *fp;
	fp = fopen(dir, "w");
	fclose(fp);

	return 0;
}

int applyConstraintsToPassword(char *pswd, char *dirJson) 
{
	siteStruct site;
	if(readJSON(&site, dirJson))
	{
		return 1;
	}

	printf("Retrieved password constraints...\n");

	// Strip out characters that are not allowed
	for(int i = 0 ; i < strlen(pswd) ; i++) 
	{
		for(int j = 0 ; j < strlen(site.banChars) ; j++)
		{
			if(pswd[i] == site.banChars[j]) 
			{
				pswd[i] = '0';
			}
		}
	}
	
	// Append required characters
	strncat(pswd, site.reqChars, (256 - strlen(pswd)));

	return 0;
}

int commandMode(void) {
	char buffer[32]; // Command buffer
	command_t swc = CMD_INVALID; // Case
	int cmdTableSize = sizeof(commandTable) / sizeof(commandTable[0]); // Get size of command table
	int loop = 1; // Loop boolean

	while(loop) 
	{
		printf("[cmd] >>> ");
		getStringAndExcludeNewline(buffer, 32);

		for(int i = 0 ; i < cmdTableSize ; i++) 
		{
			if(!strncmp(buffer, commandTable[i], strlen(commandTable[i])))
			{
				swc = i;
				break;
			}
		}

		switch(swc) 
		{
			case CMD_MAKE_SITE:
				if(siteBuildPrompt()) return 1;
				break;
			case CMD_GEN_PASSWD:
				loop = 0;
				break;
			case CMD_EXIT:
				return 1;
				break;
			case CMD_INVALID:
				fprintf(stderr, "Invalid command...\n");
				return 1;
				break;
			default:
				return 1;
				break;
		}

		memset(buffer, 0, 32);
		swc = CMD_INVALID;
	}

	return 0;
}

int main(void) 
{
	if(!commandMode())
	{
		// Termios stuff #1
		struct termios oflags, nflags; // Struct for original flags and new flags
		tcgetattr(fileno(stdin), &oflags);
		nflags = oflags;
		nflags.c_lflag &= ~ECHO;
		nflags.c_lflag |= ECHONL;
		
		if(tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) 
		{
			fprintf(stderr, "Error setting terminal attributes...\n");
			return 1;
		}

		// Copy key 1 and hash it. Save hash to a file. Make sure site JSON for key 2 exists first.
		char key1[256];
		printf("[key] >>> ");
		getStringAndExcludeNewline(key1, 64);

		char key2[64];
		printf("[site] >>> ");
		getStringAndExcludeNewline(key2, 64);

		// Termios stuff #2
		if(tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) 
		{
			fprintf(stderr, "Error reverting terminal attributes...\n");
			return 1;
		}

		// Do verification stuff		
		// Check if site exists
		char dirJson[128];
		snprintf(dirJson, 128, "%s/.psswdmgr/sites/%s.json", getenv("HOME"), key2);
		
		if(access(dirJson, F_OK)) {
			fprintf(stderr, "Could not locate specified site...\n");
			return 1;
		}

		// Dir for psswd hash
		char dirTxt[128];
		snprintf(dirTxt, 128, "%s/.psswdmgr/sites/psswd/%s.txt", getenv("HOME"), key2);

		// Check if file is empty then write the hashed password
		argon2_params psswdParams;
		psswdParams.t_cost = 2;
		psswdParams.m_cost = (1 << 12); // ~4MiB
		psswdParams.parallelism = 1;

		char hashedPsswd[512];
		char psswdSalt[64] = "SALT FOR HASHING KEY1";

		FILE *fp;
		fp = fopen(dirTxt, "r+");
		if(fp == NULL) {
			fprintf(stderr, "Error opening the file: %s\n", dirTxt);
			return 1;
		}

		// Check size of password hash file, if 0 then gen hash. If not then compare hashes
		fseek(fp, 0, SEEK_END);

		if(ftell(fp) == 0) 
		{
			// Generate password hash
			char buff[32];

			int hashStatus = argon2i_hash_raw(
					psswdParams.t_cost,
					psswdParams.m_cost,
					psswdParams.parallelism,
					key1,
					strlen(key1),
					psswdSalt,
					strlen(psswdSalt),
					buff,
					32
					);
			if(hashStatus) return 1;

			strncpy(hashedPsswd, b64_encode(buff, sizeof(buff)), 512);
			fputs(hashedPsswd, fp);
			memset(hashedPsswd, 0, 512);
		} else
		{
			fseek(fp, 0, SEEK_SET);
			// Generate password and compare to file
			char buff[32];

			int hashStatus = argon2i_hash_raw(
					psswdParams.t_cost,
					psswdParams.m_cost,
					psswdParams.parallelism,
					key1,
					strlen(key1),
					psswdSalt,
					strlen(psswdSalt),
					buff,
					32
					);
			if(hashStatus) return 1;

			strncpy(hashedPsswd, b64_encode(buff, sizeof(buff)), 512);

			char filePsswd[512];
			fgets(filePsswd, strlen(hashedPsswd) + 1, fp);

			if(strncmp(hashedPsswd, filePsswd, strlen(hashedPsswd))) {
				fprintf(stderr, "Password for site is incorrect...\n");
				return 1;
			}
		}
		fclose(fp);

		// Append password change counter to key1
		siteStruct site;
		if(readJSON(&site, dirJson))
		{
			return 1;
		}
	
		char *key1Strdup = strdup(key1);
		snprintf(key1, 128, "%s%d", key1Strdup, site.pasCount);
		free(key1Strdup);

		// Concat inputs
		strncat(key1, key2, (256 - strlen(key1)));

		// Salt with random characters
		char salt[64] = "SALT FOR GENERATED PASSWORD";
		
		// Buffer for storing the hash
		uint8_t hashBuffer[32];

		// Setup
		argon2_params outputParams;
		outputParams.t_cost = 3; // Three iterations
		outputParams.m_cost = (1 << 20); // ~1 GiB
		outputParams.parallelism = 1; // Use 1 thread

		// Hash and store in buffer
		int hashStatus = argon2i_hash_raw(
				outputParams.t_cost, 
				outputParams.m_cost,
				outputParams.parallelism,
				key1,
				strlen(key1),
				salt,
				strlen(salt),
				hashBuffer,
				sizeof(hashBuffer)
				);	
		
		if(hashStatus) return 1;

		// Output as base64
		char b64Encoded[256];
		strncpy(b64Encoded, b64_encode(hashBuffer, sizeof(hashBuffer)), 256);

		applyConstraintsToPassword(b64Encoded, dirJson);
		
		printf("\n%s\n", b64Encoded);
	}
	return 0;
}
