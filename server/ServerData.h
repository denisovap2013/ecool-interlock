#ifndef __ServerData_H__
#define __ServerData_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "cvidef.h"

#include "ServerConfigData.h"  
#include "ModBusUbs.h"
		
//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// External variables
extern modbus_block_data_t modbusBlockData;

//==============================================================================
// Global functions
void InitServerData(void);
void ReleaseServerData(void);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __ServerData_H__ */
