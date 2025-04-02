//==============================================================================
//
//==============================================================================

#ifndef __ubsClientConfiguration_H__
#define __ubsClientConfiguration_H__


//==============================================================================
// Include files

#include "cvidef.h"

//==============================================================================
// Global variables
extern char SERVER_IP[256];
extern unsigned int SERVER_PORT;
extern double SERVER_CONNECTION_INTERVAL;
extern double UBS_REQUEST_RATE;


extern char LOG_DIRECTORY[256];
extern char DATA_DIRECTORY[256];
extern double DATA_WRITE_INTERVAL;

extern unsigned int DI_MASKS[2];
extern unsigned short DQ_MASKS[3]; 

//==============================================================================
// Global functions

void ConfigurateClient(char * configPath);

#endif  /* ndef __ubsClientConfiguration_H__ */
