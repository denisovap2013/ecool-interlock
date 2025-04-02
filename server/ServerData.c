//==============================================================================
// Include files

#include <ansi_c.h>
#include <ansi_c.h>

#include "ServerData.h"


//==============================================================================
// Types

//==============================================================================
// Static global variables
int server_data_initialized = 0;

//==============================================================================
// Static functions


//==============================================================================
// Global variables
modbus_block_data_t modbusBlockData = {0};

//==============================================================================
// Global functions

void InitServerData(void) {
    if (server_data_initialized) return;
    
    // Initialize modbus block data with zeros 
    // (in case a client connect before we connect to the UBS block, 
    // so we can send something to the client)
    memset(&modbusBlockData, 0, sizeof(modbusBlockData));
    server_data_initialized = 1;
}

void ReleaseServerData(void) {
	if (!server_data_initialized) return;

	// Nothing to release at the moment.

	server_data_initialized = 0;
}
