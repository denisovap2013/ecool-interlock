#include "inifile.h"
#include "ServerConfigData.h" 

#define STOP_CONFIGURATION(x) MessagePopup("Configuration Error", (x)); Ini_Dispose(iniText); exit(0);
//==============================================================================
char 	UBS_ADC_NAMES[4][8][256];
double 	UBS_ADC_COEFF[4][8][2];

char	UBS_DAC_NAMES[1][4][256];

char 	UBS_DI_NAMES[2][32][256];
char 	UBS_DQ_NAMES[3][16][256];

int		LOG_ADDRESS;
	
// UBS connection parameters
char 	UBS_CONNECTION_IP[256];
int		UBS_CONNECTION_PORT;
int		UBS_CONNECTION_TIMEOUT;
int		UBS_RECONNECTION_DELAY;
int		UBS_CONNECTION_READ_TIMEOUT;
int		UBS_CONNECTION_SEND_TIMEOUT;

// UBS requests parameters
int		UBS_DATA_REQUEST_INTERVAL;
int		UBS_LOG_STATE_REQUEST_INTERVAL;


// TCP/IP parameters
int		TCP_PORT;
	
// LOG parameters
char	FILE_LOG_DIRECTORY[256];
char 	FILE_DATA_DIRECTORY[256];
char 	FILE_EVENTS_DIRECTORY[256]; 
int		FILE_DATA_WRITE_INTERVAL; //seconds
int 	FILE_EXPIRATION;


void InitServerConfig(char * configPath) {
	char stringBuffer[256];
	
	int i, j;
	char key[256], section[64], tmp_text[256];
	IniText iniText;
	
	if (!configPath) {
		STOP_CONFIGURATION("Configuration file path is NULL.");
	}
	
	iniText = Ini_New(1);
	if( Ini_ReadFromFile(iniText,configPath) < 0 ) {
		STOP_CONFIGURATION("Cannot read the config file."); 
	}

	////////////////////////////////////////////////////
	////////////////////////////////////////////////////
	// FILE //
	//////////////////////////////////////////////////// 
	// log dir
	if(Ini_GetStringIntoBuffer(iniText, "FILE", "logDir", FILE_LOG_DIRECTORY, sizeof(FILE_LOG_DIRECTORY)) <= 0) {
		STOP_CONFIGURATION("Cannot read 'logDir' from the 'FILE' section.");  
	}

	// data dir
	if(Ini_GetStringIntoBuffer(iniText, "FILE", "dataDir", FILE_DATA_DIRECTORY, sizeof(FILE_DATA_DIRECTORY)) <= 0) {
		STOP_CONFIGURATION("Cannot read 'dataDir' from the 'FILE' section.");
	}
	
	// data dir
	if(Ini_GetStringIntoBuffer(iniText, "FILE", "eventsDir", FILE_EVENTS_DIRECTORY, sizeof(FILE_EVENTS_DIRECTORY)) <= 0) {
		STOP_CONFIGURATION("Cannot read 'eventsDir' from the 'FILE' section.");
	}

	// data write interval
	if(Ini_GetInt(iniText, "FILE", "dataWriteInterval", &FILE_DATA_WRITE_INTERVAL) <= 0) {
		STOP_CONFIGURATION("Cannot read 'dataWriteInterval' from the 'FILE' section."); 
	}
	
	// dataOldFiles
	if(Ini_GetInt(iniText, "FILE", "oldFiles", &FILE_EXPIRATION) <= 0) {
		STOP_CONFIGURATION("Cannot read 'oldFiles' from the 'FILE' section."); 
	}

	////////////////////////////////////////////////////
	// TCP //
	//////////////////////////////////////////////////// 
	// Port
	if(Ini_GetInt(iniText, "TCP", "tcpPort", &TCP_PORT) <= 0) {
		STOP_CONFIGURATION("Cannot read 'tcpPort' from the 'TCP' section.");
	}

	////////////////////////////////////////////////////
	// UBS_BLOCK_CONNECTION //
	//////////////////////////////////////////////////// 
	// Port
	if(Ini_GetInt(iniText, "UBS_BLOCK_CONNECTION", "port", &UBS_CONNECTION_PORT) <= 0) {
		STOP_CONFIGURATION("Cannot read 'port' from the 'UBS_BLOCK_CONNECTION' section."); 
	}

	// Ip
	if(Ini_GetStringIntoBuffer(iniText, "UBS_BLOCK_CONNECTION", "ip", UBS_CONNECTION_IP, sizeof(UBS_CONNECTION_IP)) <= 0) {
		STOP_CONFIGURATION("Cannot read 'ip' from the 'UBS_BLOCK_CONNECTION' section."); 
	}

	// Connection timeout
	if(Ini_GetInt(iniText, "UBS_BLOCK_CONNECTION", "connectionTimeout", &UBS_CONNECTION_TIMEOUT) <= 0) {
		STOP_CONFIGURATION("Cannot read 'connectionTimeout' from the 'UBS_BLOCK_CONNECTION' section."); 
	}

	// Reconnection delay
	if(Ini_GetInt(iniText, "UBS_BLOCK_CONNECTION", "connectionDelay", &UBS_RECONNECTION_DELAY) <= 0) {
		STOP_CONFIGURATION("Cannot read 'connectionDelay' from the 'UBS_BLOCK_CONNECTION' section.");
	}

	// Recieve timeout
	if(Ini_GetInt(iniText, "UBS_BLOCK_CONNECTION", "readTimeout", &UBS_CONNECTION_READ_TIMEOUT) <= 0) {
		STOP_CONFIGURATION("Cannot read 'readTimeout' from the 'UBS_BLOCK_CONNECTION' section."); 
	}

	// Send timeout
	if(Ini_GetInt(iniText, "UBS_BLOCK_CONNECTION", "sendTimeout", &UBS_CONNECTION_SEND_TIMEOUT) <= 0) {
		STOP_CONFIGURATION("Cannot read 'sendTimeout' from the 'UBS_BLOCK_CONNECTION' section."); 
	}

	////////////////////////////////////////////////////
	// UBS_BLOCK_LOG //
	//////////////////////////////////////////////////// 
	// Log address in the UBS block buffer
	if(Ini_GetInt(iniText, "UBS_BLOCK_LOG", "logAddress", &LOG_ADDRESS) <= 0) {
		STOP_CONFIGURATION("Cannot read 'logAddress' from the 'UBS_BLOCK_LOG' section."); 
	}
	
	////////////////////////////////////////////////////
	// UBS_REQUESTS //
	//////////////////////////////////////////////////// 
	if(Ini_GetInt(iniText, "UBS_REQUESTS", "dataRequestInterval ", &UBS_DATA_REQUEST_INTERVAL) <= 0) {
		STOP_CONFIGURATION("Cannot read 'dataRequestInterval ' from the 'UBS_REQUESTS' section."); 
	}
	
	if(Ini_GetInt(iniText, "UBS_REQUESTS", "logStateRequestInterval  ", &UBS_LOG_STATE_REQUEST_INTERVAL) <= 0) {
		STOP_CONFIGURATION("Cannot read 'logStateRequestInterval  ' from the 'UBS_REQUESTS' section."); 
	}
	
	////////////////////////////////////////////////////
	// UBS-ADC-xx //
	//////////////////////////////////////////////////// 
    // ADC names and coefficients
	for (i=0; i<4; i++) {  // ADC index
		sprintf(section, "UBS-ADC-%d", i);
		for (j=0; j<8; j++) {  // ADC channel index

			// Read names
			sprintf(key, "adcName%d", j);
			if(Ini_GetStringIntoBuffer(iniText, section, key, UBS_ADC_NAMES[i][j], sizeof(UBS_ADC_NAMES[i][j])) <= 0) {
				sprintf(tmp_text, "Cannot read '%s' from the '%s' section.", key, section);
				STOP_CONFIGURATION(tmp_text); 
			}
				
			if (strstr(UBS_ADC_NAMES[i][j], "|") != NULL ) {
				sprintf(tmp_text, "Name '%s' from the '%s' section contains the prohibited symbol '|'.", key, section);
				STOP_CONFIGURATION(tmp_text);	
			}
			
			if (!strlen(UBS_ADC_NAMES[i][j])) {
				sprintf(tmp_text, "Name '%s' from the '%s' section is empty.", key, section);
				STOP_CONFIGURATION(tmp_text);	
			}
			
			// Read coefficients
			sprintf(key, "adcCoeff%d", j);
			if(Ini_GetStringIntoBuffer(iniText,section,key, stringBuffer, sizeof(stringBuffer)) <= 0) {
				sprintf(tmp_text, "Cannot read '%s' from the '%s' section.", key, section);
				STOP_CONFIGURATION(tmp_text); 
			}

			if (sscanf(stringBuffer, "%lf %lf", &UBS_ADC_COEFF[i][j][0], &UBS_ADC_COEFF[i][j][1]) < 2 ) {
				sprintf(tmp_text, "Cannot parse ADC coefficients for '%s' from the '%s' section (expected two numbers).", key, section);
				STOP_CONFIGURATION(tmp_text); 	
			}

		}
	}
	
	////////////////////////////////////////////////////
	// UBS-DAC-xx //
	//////////////////////////////////////////////////// 
	// DAC names
	for (i=0; i<1; i++) {  // DAC index
		sprintf(section, "UBS-DAC-%d", i);
		for (j=0; j<4; j++) {  // DAC channel index

			sprintf(key, "dacName%d", j);
			if(Ini_GetStringIntoBuffer(iniText, section, key, UBS_DAC_NAMES[i][j], sizeof(UBS_DAC_NAMES[i][j])) <= 0) {
				sprintf(tmp_text, "Cannot read '%s' from the '%s' section.", key, section);
				STOP_CONFIGURATION(tmp_text);
			}
				
			if (strstr(UBS_DAC_NAMES[i][j], "|") != NULL ) {
				sprintf(tmp_text, "Name '%s' from the '%s' section contains the prohibited symbol '|'.", key, section);
				STOP_CONFIGURATION(tmp_text);	
			}
			
			if (!strlen(UBS_DAC_NAMES[i][j])) {
				sprintf(tmp_text, "Name '%s' from the '%s' section is empty.", key, section);
				STOP_CONFIGURATION(tmp_text);	
			}
		}
	}
	
	////////////////////////////////////////////////////
	// UBS-DI-x //
	//////////////////////////////////////////////////// 
	// DI names
	for (i=0; i<2; i++) {  // DI index
		sprintf(section, "UBS-DI-%d", i); 
		for (j=0; j<32; j++) {  // DI channel index

			sprintf(key,"diName%d", j);
			if(Ini_GetStringIntoBuffer(iniText, section, key, UBS_DI_NAMES[i][j], sizeof(UBS_DI_NAMES[i][j])) <= 0) {
				sprintf(tmp_text, "Cannot read '%s' from the '%s' section.", key, section);
				STOP_CONFIGURATION(tmp_text);
			}
				
			if (strstr(UBS_DI_NAMES[i][j], "|") != NULL ) {
				sprintf(tmp_text, "Name '%s' from the '%s' section contains the prohibited symbol '|'.", key, section);
				STOP_CONFIGURATION(tmp_text);	
			}
			
			if (!strlen(UBS_DI_NAMES[i][j])) {
				sprintf(tmp_text, "Name '%s' from the '%s' section is empty.", key, section);
				STOP_CONFIGURATION(tmp_text);	
			}
		}
	}
	
	////////////////////////////////////////////////////
	// UBS-DQ-x //
	////////////////////////////////////////////////////
	// DQ names
	for (i=0; i<3; i++) {  // DQ index
		sprintf(section, "UBS-DQ-%d", i); 
		for (j=0; j<16; j++) {  // DQ channel index

			sprintf(key, "dqName%d", j);
			if(Ini_GetStringIntoBuffer(iniText, section, key, UBS_DQ_NAMES[i][j], sizeof(UBS_DQ_NAMES[i][j])) <= 0) {
				sprintf(tmp_text, "Cannot read '%s' from the '%s' section.", key, section);
				STOP_CONFIGURATION(tmp_text);
			}
				
			if (strstr(UBS_DQ_NAMES[i][j], "|") != NULL ) {
				sprintf(tmp_text, "Name '%s' from the '%s' section contains the prohibited symbol '|'.", key, section);
				STOP_CONFIGURATION(tmp_text);	
			}
			
			if (!strlen(UBS_DQ_NAMES[i][j])) {
				sprintf(tmp_text, "Name '%s' from the '%s' section is empty.", key, section);
				STOP_CONFIGURATION(tmp_text);	
			}
		}
	}

}
