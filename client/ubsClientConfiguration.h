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
extern char CFG_SERVER_IP[256];
extern unsigned int CFG_SERVER_PORT;
extern double CFG_SERVER_CONNECTION_INTERVAL;
extern double CFG_UBS_REQUEST_RATE;


extern char CFG_LOG_DIRECTORY[256];
extern char CFG_DATA_DIRECTORY[256];
extern double CFG_DATA_WRITE_INTERVAL;

extern unsigned int CFG_DI_MASKS[2];
extern unsigned short CFG_DQ_MASKS[3]; 

//==============================================================================
// Global functions

void ConfigurateClient(char * configPath);

#endif  /* ndef __ubsClientConfiguration_H__ */
