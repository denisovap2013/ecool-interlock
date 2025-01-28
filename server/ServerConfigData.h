//==============================================================================
//
//==============================================================================

#ifndef __ServerConfigData_H__
#define __ServerConfigData_H__

//==============================================================================
// Include files

#include "cvidef.h"
//==============================================================================  

// UBS parameters
extern char 	UBS_ADC_NAMES[4][8][256];
extern double 	UBS_ADC_COEFF[4][8][2];

extern char		UBS_DAC_NAMES[1][4][256];

extern char 	UBS_DI_NAMES[2][32][256];
extern char 	UBS_DQ_NAMES[3][16][256];
extern int	    LOG_ADDRESS;
	
// UBS connection parameters
extern char 	UBS_CONNECTION_IP[256];
extern int		UBS_CONNECTION_PORT;
extern int		UBS_CONNECTION_TIMEOUT;
extern int		UBS_RECONNECTION_DELAY;
extern int		UBS_CONNECTION_READ_TIMEOUT;
extern int		UBS_CONNECTION_SEND_TIMEOUT;

// UBS requests parameters
extern int		UBS_DATA_REQUEST_INTERVAL;
extern int		UBS_LOG_STATE_REQUEST_INTERVAL;

// TCP/IP parameters
extern int		TCP_PORT;
	
// LOG parameters
extern char		FILE_LOG_DIRECTORY[256];
extern char 	FILE_DATA_DIRECTORY[256];
extern char 	FILE_EVENTS_DIRECTORY[256]; 
extern int		FILE_DATA_WRITE_INTERVAL; //seconds
extern int 		FILE_EXPIRATION;	// days

void InitServerConfig(char * configPath);
		
//==============================================================================  		


#endif  /* ndef __ServerConfigData_H__ */
