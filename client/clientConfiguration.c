//==============================================================================
//
//==============================================================================

#include "inifile.h"
#include <ansi_c.h>
#include "clientConfiguration.h"

char CFG_SERVER_IP[256];
unsigned int CFG_SERVER_PORT;
double CFG_SERVER_CONNECTION_INTERVAL;
double CFG_UBS_REQUEST_RATE;


char CFG_LOG_DIRECTORY[256];
char CFG_DATA_DIRECTORY[256];
double CFG_DATA_WRITE_INTERVAL;

unsigned int CFG_DI_MASKS[2];
unsigned short CFG_DQ_MASKS[3]; 


#define BUF_SIZE 256


void ConfigurateClient(char * configPath) {
	int i;
	char strBuf[256], key[256], errorMsg[256];
	
	IniText iniText;

	#define INFORM_AND_STOP(msg) MessagePopup("Configuration Error", (msg)); Ini_Dispose(iniText); exit(0);
	#define STOP_CONFIGURATION(s, k) sprintf(errorMsg, "Cannot read '%s' from the '%s' section. (Line: %d)", (k), (s), Ini_LineOfLastAccess(iniText)); INFORM_AND_STOP(errorMsg);  
	#define READ_STRING(s, k, var) if(Ini_GetStringIntoBuffer(iniText, (s), (k), (var), sizeof((var))) <= 0) {STOP_CONFIGURATION((s), (k));} 
	#define READ_INT(s, k, var) if(Ini_GetInt(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}
	#define READ_DOUBLE(s, k, var) if(Ini_GetDouble(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}

	iniText = Ini_New(1);

	if (!configPath) {
		INFORM_AND_STOP("Configuration file path is NULL.");
	}
	
	if( Ini_ReadFromFile(iniText, configPath) < 0 ) {
		sprintf(errorMsg, "Unable to read the configuration file '%s'.", configPath);
		INFORM_AND_STOP(errorMsg);
	}

	////////////////////////////
	// == FILE ==
	// LOG
	READ_STRING("FILE", "logDir", CFG_LOG_DIRECTORY);
	// DATA
	READ_STRING("FILE", "dataDir", CFG_DATA_DIRECTORY);
	// WRITE INTERVAL
	READ_DOUBLE("FILE", "dataWriteInterval", CFG_DATA_WRITE_INTERVAL);
	
	// == SERVER ==
	// IP
	READ_STRING("TCP", "serverIp", CFG_SERVER_IP);
	// Port
	READ_INT("TCP", "serverPort", CFG_SERVER_PORT);
	// UBS REQUEST RATE
	READ_DOUBLE("TCP", "request", CFG_UBS_REQUEST_RATE);
	// UBS CONNECTION INTERVAL
	READ_DOUBLE("TCP", "connection", CFG_SERVER_CONNECTION_INTERVAL);
	
	// == MASKS ==
	// DI MASKS
	for (i=0; i<2; i++) {
		sprintf(key, "di%d_mask", i);
		READ_STRING("MASKS", key, strBuf);
		
		if (sscanf(strBuf, "%X", &CFG_DI_MASKS[i]) < 1) {
			sprintf(errorMsg, "Unable to parse the hex code (%s) for '%s' from the 'MASKS' section.", strBuf, key);
			INFORM_AND_STOP(errorMsg);	
		}
	}
	
	// DQ MASKS
	for (i=0; i<3; i++) {
		sprintf(key, "dq%d_mask", i);
		READ_STRING("MASKS", key, strBuf);
		
		if (sscanf(strBuf, "%hX", &CFG_DQ_MASKS[i]) < 1) {
			sprintf(errorMsg, "Unable to parse the hex code (%s) for '%s' from the 'MASKS' section.", strBuf, key);
			INFORM_AND_STOP(errorMsg);	
		}
	}
	////////////////////////////
	Ini_Dispose(iniText);
	return;
}
