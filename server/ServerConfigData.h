//==============================================================================
//
//==============================================================================

#ifndef __ServerConfigData_H__
#define __ServerConfigData_H__

//==============================================================================
// Include files

#include "cvidef.h"
//==============================================================================  

// Server general parameters
extern char     CFG_SERVER_NAME[256];

// UBS parameters
extern char 	CFG_UBS_ADC_NAMES[4][8][256];
extern double 	CFG_UBS_ADC_COEFF[4][8][2];

extern char		CFG_UBS_DAC_NAMES[1][4][256];

extern char 	CFG_UBS_DI_NAMES[2][32][256];
extern char 	CFG_UBS_DQ_NAMES[3][16][256];
extern int	    CFG_LOG_ADDRESS;
	
// UBS connection parameters
extern char 	CFG_UBS_CONNECTION_IP[256];
extern int		CFG_UBS_CONNECTION_PORT;
extern int		CFG_UBS_CONNECTION_TIMEOUT;
extern int		CFG_UBS_RECONNECTION_DELAY;
extern int		CFG_UBS_CONNECTION_READ_TIMEOUT;
extern int		CFG_UBS_CONNECTION_SEND_TIMEOUT;

// UBS requests parameters
extern int		CFG_UBS_DATA_REQUEST_INTERVAL;
extern int		CFG_UBS_LOG_STATE_REQUEST_INTERVAL;

// TCP/IP parameters
extern int		CFG_TCP_PORT;
	
// LOG parameters
extern char		CFG_FILE_LOG_DIRECTORY[256];
extern char 	CFG_FILE_DATA_DIRECTORY[256];
extern char 	CFG_FILE_EVENTS_DIRECTORY[256]; 
extern int		CFG_FILE_DATA_WRITE_INTERVAL; //seconds
extern int 		CFG_FILE_EXPIRATION;	// days

void InitServerConfig(char * configPath);
		
//==============================================================================  		


#endif  /* ndef __ServerConfigData_H__ */
