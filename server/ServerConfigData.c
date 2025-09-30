#include "inifile.h"
#include "ServerConfigData.h" 

//==============================================================================
// Server general parameters
char    CFG_SERVER_NAME[256];

char 	CFG_UBS_ADC_NAMES[4][8][256];
double 	CFG_UBS_ADC_COEFF[4][8][2];

char	CFG_UBS_DAC_NAMES[1][4][256];

char 	CFG_UBS_DI_NAMES[2][32][256];
char 	CFG_UBS_DQ_NAMES[3][16][256];
	
// UBS connection parameters
char 	CFG_UBS_CONNECTION_IP[256];
int		CFG_UBS_CONNECTION_PORT;
int		CFG_UBS_CONNECTION_TIMEOUT;
int		CFG_UBS_RECONNECTION_DELAY;
int		CFG_UBS_CONNECTION_READ_TIMEOUT;
int		CFG_UBS_CONNECTION_SEND_TIMEOUT;

// UBS requests parameters
int		CFG_UBS_DATA_REQUEST_INTERVAL;
int		CFG_UBS_LOG_STATE_REQUEST_INTERVAL;


// TCP/IP parameters
int		CFG_TCP_PORT;
	
// LOG parameters
char	CFG_FILE_LOG_DIRECTORY[256];
char 	CFG_FILE_DATA_DIRECTORY[256];
char 	CFG_FILE_EVENTS_DIRECTORY[256]; 
int		CFG_FILE_DATA_WRITE_INTERVAL; //seconds
int 	CFG_FILE_EXPIRATION;


void InitServerConfig(char * configPath) {
	char stringBuffer[256];
	
	int i, j;
	char key[256], section[64], tmp_text[256];
	char msg[256];
	IniText iniText;
	
	#define INFORM_AND_STOP(msg) MessagePopup("Configuration Error", (msg)); Ini_Dispose(iniText); exit(0);
	#define STOP_CONFIGURATION(s, k) sprintf(msg, "Cannot read '%s' from the '%s' section. (Line: %d)", (k), (s), Ini_LineOfLastAccess(iniText)); INFORM_AND_STOP(msg); 
	#define READ_STRING(s, k, var) if(Ini_GetStringIntoBuffer(iniText, (s), (k), (var), sizeof((var))) <= 0) {STOP_CONFIGURATION((s), (k));} 
	#define READ_INT(s, k, var) if(Ini_GetInt(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}
	#define READ_DOUBLE(s, k, var) if(Ini_GetDouble(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}
	#define READ_DOUBLE_OR_DEFAULT(s, k, var, default_val) if (Ini_ItemExists(iniText, (s), (k))) { READ_DOUBLE((s), (k), (var)) } else { (var) = (default_val); }
	#define READ_COEFFICIENTS(s, k, sbuf, c1, c2) if(Ini_GetStringIntoBuffer(iniText, (s), (k), (sbuf), sizeof((sbuf))) <= 0) {STOP_CONFIGURATION((s), (k));} else { if (sscanf((sbuf), "%lf %lf", &(c1), &(c2)) < 2 ) {STOP_CONFIGURATION((s), (k));} }
	#define READ_COEFFICIENTS_OR_DEFAULT(s, k, sbuf, c1, c2, dc1, dc2) if (Ini_ItemExists(iniText, (s), (k))) { READ_COEFFICIENTS((s), (k), (sbuf), (c1), (c2)) } else { (c1) = (dc1); (c2) = (dc2); }  
	#define READ_INT_OR_DEFAULT(s, k, var, default_val) if (Ini_ItemExists(iniText, (s), (k))) { READ_INT((s), (k), (var)) } else { (var) = (default_val); }
	#define READ_STRING_OR_DEFAULT(s, k, var, default_val) if (Ini_ItemExists(iniText, (s), (k))) { READ_STRING((s), (k), (var)) } else { strcpy((var), (default_val)); }

	#define GENERAL_SECTION "GENERAL" 
	#define FILE_SECTION "FILE" 
	#define TCP_SECTION "TCP"
	#define UBS_BLOCK_CONNECTION "UBS_BLOCK_CONNECTION"
	#define UBS_BLOCK_LOG "UBS_BLOCK_LOG"
	#define UBS_REQUESTS "UBS_REQUESTS"

	iniText = Ini_New(0);

	if (!configPath) {
		INFORM_AND_STOP("Configuration file path is NULL.");
	}
	
	if( Ini_ReadFromFile(iniText, configPath) < 0 ) {
		sprintf(msg, "Unable to read the configuration file '%s'.", configPath);
		INFORM_AND_STOP(msg);
	}

	////////////////////////////////////////////////////
	// GENERAL //
	////////////////////////////////////////////////////
	READ_STRING(GENERAL_SECTION, "serverName", CFG_SERVER_NAME); 

	////////////////////////////////////////////////////
	// FILE //
	//////////////////////////////////////////////////// 

	// directories for wrinting data and logs
	READ_STRING(FILE_SECTION, "logDir", CFG_FILE_LOG_DIRECTORY);
	READ_STRING(FILE_SECTION, "dataDir", CFG_FILE_DATA_DIRECTORY);
	READ_STRING(FILE_SECTION, "eventsDir", CFG_FILE_EVENTS_DIRECTORY);

	// data write interval
	READ_INT(FILE_SECTION, "dataWriteInterval", CFG_FILE_DATA_WRITE_INTERVAL);
	
	// dataOldFiles
	READ_INT(FILE_SECTION, "oldFiles", CFG_FILE_EXPIRATION);

	////////////////////////////////////////////////////
	// TCP //
	//////////////////////////////////////////////////// 

	// Port
	READ_INT(TCP_SECTION, "tcpPort", CFG_TCP_PORT);

	////////////////////////////////////////////////////
	// UBS_BLOCK_CONNECTION //
	//////////////////////////////////////////////////// 

	// IP and Port
	READ_STRING(UBS_BLOCK_CONNECTION, "ip", CFG_UBS_CONNECTION_IP);
	READ_INT(UBS_BLOCK_CONNECTION, "port", CFG_UBS_CONNECTION_PORT);

	// Connection timeout
	READ_INT(UBS_BLOCK_CONNECTION, "connectionTimeout", CFG_UBS_CONNECTION_TIMEOUT);

	// Reconnection delay
	READ_INT(UBS_BLOCK_CONNECTION, "connectionDelay", CFG_UBS_RECONNECTION_DELAY);

	// Recieve timeout
	READ_INT(UBS_BLOCK_CONNECTION, "readTimeout", CFG_UBS_CONNECTION_READ_TIMEOUT);

	// Send timeout
	READ_INT(UBS_BLOCK_CONNECTION, "sendTimeout", CFG_UBS_CONNECTION_SEND_TIMEOUT);

	////////////////////////////////////////////////////
	// UBS_BLOCK_LOG //
	//////////////////////////////////////////////////// 
	
	//     Add parameters reading here
	
	////////////////////////////////////////////////////
	// UBS_REQUESTS //
	////////////////////////////////////////////////////

	READ_INT(UBS_REQUESTS, "dataRequestInterval", CFG_UBS_DATA_REQUEST_INTERVAL);
	READ_INT(UBS_REQUESTS, "logStateRequestInterval", CFG_UBS_LOG_STATE_REQUEST_INTERVAL);

	////////////////////////////////////////////////////
	// UBS-ADC-xx //
	//////////////////////////////////////////////////// 
    // ADC names and coefficients
	for (i=0; i<4; i++) {  // ADC index
		sprintf(section, "UBS-ADC-%d", i);
		for (j=0; j<8; j++) {  // ADC channel index

			// Read names
			sprintf(key, "adcName%d", j);

			READ_STRING(section, key, CFG_UBS_ADC_NAMES[i][j]);
				
			if (strstr(CFG_UBS_ADC_NAMES[i][j], "|") != NULL ) {
				sprintf(tmp_text, "Name '%s' from the '%s' section contains the prohibited symbol '|'.", key, section);
				INFORM_AND_STOP(tmp_text);
			}
			
			if (!strlen(CFG_UBS_ADC_NAMES[i][j])) {
				sprintf(tmp_text, "Name '%s' from the '%s' section is empty.", key, section);
				INFORM_AND_STOP(tmp_text);	
			}
			
			// Read coefficients
			sprintf(key, "adcCoeff%d", j);
			READ_STRING(section, key, stringBuffer);

			if (sscanf(stringBuffer, "%lf %lf", &CFG_UBS_ADC_COEFF[i][j][0], &CFG_UBS_ADC_COEFF[i][j][1]) < 2 ) {
				sprintf(tmp_text, "Cannot parse ADC coefficients for '%s' from the '%s' section (expected two numbers), got '%s'.", key, section, stringBuffer);
				INFORM_AND_STOP(tmp_text); 	
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
			READ_STRING(section, key, CFG_UBS_DAC_NAMES[i][j]);
				
			if (strstr(CFG_UBS_DAC_NAMES[i][j], "|") != NULL ) {
				sprintf(tmp_text, "Name '%s' from the '%s' section contains the prohibited symbol '|'.", key, section);
				INFORM_AND_STOP(tmp_text);	
			}
			
			if (!strlen(CFG_UBS_DAC_NAMES[i][j])) {
				sprintf(tmp_text, "Name '%s' from the '%s' section is empty.", key, section);
				INFORM_AND_STOP(tmp_text);	
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
			READ_STRING(section, key, CFG_UBS_DI_NAMES[i][j]);
				
			if (strstr(CFG_UBS_DI_NAMES[i][j], "|") != NULL ) {
				sprintf(tmp_text, "Name '%s' from the '%s' section contains the prohibited symbol '|'.", key, section);
				INFORM_AND_STOP(tmp_text);	
			}
			
			if (!strlen(CFG_UBS_DI_NAMES[i][j])) {
				sprintf(tmp_text, "Name '%s' from the '%s' section is empty.", key, section);
				INFORM_AND_STOP(tmp_text);	
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
			READ_STRING(section, key, CFG_UBS_DQ_NAMES[i][j]);
				
			if (strstr(CFG_UBS_DQ_NAMES[i][j], "|") != NULL ) {
				sprintf(tmp_text, "Name '%s' from the '%s' section contains the prohibited symbol '|'.", key, section);
				INFORM_AND_STOP(tmp_text);	
			}
			
			if (!strlen(CFG_UBS_DQ_NAMES[i][j])) {
				sprintf(tmp_text, "Name '%s' from the '%s' section is empty.", key, section);
				INFORM_AND_STOP(tmp_text);	
			}
		}
	}
	
#ifdef __VERBOSE__
	printf("[VERBOSE] Configuration is initialized.\n");
#endif

}
