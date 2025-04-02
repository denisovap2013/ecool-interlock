//==============================================================================
//
//==============================================================================

#include "inifile.h"
#include <ansi_c.h>
#include "ubsClientConfiguration.h"

char SERVER_IP[256];
unsigned int SERVER_PORT;
double SERVER_CONNECTION_INTERVAL;
double UBS_REQUEST_RATE;


char LOG_DIRECTORY[256];
char DATA_DIRECTORY[256];
double  DATA_WRITE_INTERVAL;

unsigned int DI_MASKS[2];
unsigned short DQ_MASKS[3]; 


#define STOP_CONFIGURATION(x) MessagePopup("Configuration Error", (x)); Ini_Dispose(iniText); exit(0); 
#define BUF_SIZE 256


void ConfigurateClient(char * configPath) {
	int i;
	char strBuf[256], key[256], errorMsg[256];
	
	IniText iniText;
	if (!configPath) {
		STOP_CONFIGURATION("Configuration file path is NULL.");
	} 
	
	iniText = Ini_New(1);
	
	if( Ini_ReadFromFile(iniText,configPath) < 0 ) {
		STOP_CONFIGURATION("Cannot read the config file.");
	} 
	////////////////////////////
	// FILE
			// LOG
	if(Ini_GetStringIntoBuffer(iniText, "FILE", "logDir", LOG_DIRECTORY, BUF_SIZE) <= 0) {
		STOP_CONFIGURATION("Cannot read 'logDir' from the 'FILE' section.");  
	}
			// DATA
	if(Ini_GetStringIntoBuffer(iniText, "FILE", "dataDir", DATA_DIRECTORY, BUF_SIZE) <= 0) {
		STOP_CONFIGURATION("Cannot read 'dataDir' from the 'FILE' section.");
	}
			// WRITE INTERVAL
	if(Ini_GetDouble(iniText, "FILE", "dataWriteInterval", &DATA_WRITE_INTERVAL) <= 0) {
		STOP_CONFIGURATION("Cannot read 'dataWriteInterval' from the 'FILE' section.");
	}
	
	// SERVER
			// IP
	if(Ini_GetStringIntoBuffer(iniText, "TCP", "serverIp", SERVER_IP, BUF_SIZE) <= 0) {
		STOP_CONFIGURATION("Cannot read 'serverIp' from the 'TCP' section."); 
	}
			// Port
	if(Ini_GetInt(iniText, "TCP", "serverPort", &SERVER_PORT) <= 0) {
		STOP_CONFIGURATION("Cannot read 'serverPort' from the 'TCP' section.");
	}
			// UBS REQUEST RATE
	if(Ini_GetDouble(iniText, "TCP", "request", &UBS_REQUEST_RATE) <= 0) {
		STOP_CONFIGURATION("Cannot read 'request' from the 'TCP' section."); 
	}
			// UBS CONNECTION INTERVAL
	if(Ini_GetDouble(iniText, "TCP", "connection", &SERVER_CONNECTION_INTERVAL) <= 0) {
		STOP_CONFIGURATION("Cannot read 'connection' from the 'TCP' section.");
	}
	
	// 
	
	// DI MASKS
	for (i=0; i<2; i++) {
		sprintf(key, "di%d_mask", i);
		if(Ini_GetStringIntoBuffer(iniText, "MASKS", key, strBuf, BUF_SIZE) <= 0) {
			sprintf(errorMsg, "Cannot read '%s' from the 'MASKS' section.", key);
			STOP_CONFIGURATION(errorMsg);
		}
		
		if (sscanf(strBuf, "%X", &DI_MASKS[i]) < 1) {
			sprintf(errorMsg, "Unable to parse the hex code (%s) for '%s' from the 'MASKS' section.", strBuf, key);
			STOP_CONFIGURATION(errorMsg);	
		}
	}
	
	// DQ MASKS
	for (i=0; i<3; i++) {
		sprintf(key, "dq%d_mask", i);
		if(Ini_GetStringIntoBuffer(iniText, "MASKS", key, strBuf, BUF_SIZE) <= 0) {
			sprintf(errorMsg, "Cannot read '%s' from the 'MASKS' section.", key);
			STOP_CONFIGURATION(errorMsg);
		}
		
		if (sscanf(strBuf, "%hX", &DQ_MASKS[i]) < 1) {
			sprintf(errorMsg, "Unable to parse the hex code (%s) for '%s' from the 'MASKS' section.", strBuf, key);
			STOP_CONFIGURATION(errorMsg);	
		}
	}
	////////////////////////////
	Ini_Dispose(iniText);
	return;
}
