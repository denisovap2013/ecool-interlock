#include <ansi_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>

#include "TimeMarkers.h"
#include "TCP_Connection.h"
#include "dataskt.h"
#include <utility.h>
#include <userint.h>
#include <tcpsupp.h>
#include "MessageStack.h"
#include "ServerConfigData.h"
#include "ModBusUbs.h"
#include "Logging.h"
#include "ClientCommands.h"
#include "ServerData.h"

#define MAX_RECEIVED_BYTES 3000

// Variables

tcpConnection_ServerInterface_t tcpSI;

///////////////////////////////////////// 

/////////////////////////////////////////   SCHEDULE FUNCTIONS
void ubsBlockReconnection(int handle, int arg1);
void ubsBlockReadData(int handle, int arg1);
void ubsBlockReadLogInfo(int handle, int arg1);

void adcFileWrite(int handle, int arg1);
void deleteOldFiles(int handle, int arg1);
/////////////////////////////////////////

/////////////////////////////////////////   CONFIGURATION FUNCTIONS 
int prepareTimeSchedule(void); 


void DiscardAllResources(void);
/////////////////////////////////////////


/////////////////////////////////////////   
void bgdFunc(void);


/////////////////////////////////////////   FUNCTIONS DEFINITIONS   
void DiscardAllResources(void) {
	// TODO: disconenct from the UBS server
	UnregisterTCPServer(CFG_TCP_PORT);
	ReleaseServerData();
    ReleaseCommandParsers();
	msReleaseGlobalStack();
}


int CVICALLBACK userMainCallback(int MenuBarHandle, int MenuItemID, int event, void * callbackData, 
	int eventData1, int eventData2)
{
	switch(event)
	{
		case EVENT_END_TASK: 
			switch(eventData1)
			{
				case APP_CLOSE:
					if(ConfirmPopup("Closing the application.", "Do you want to close the application?")==0)
					{
						return 1;
					} else
					{
						logMessage("[SERVER] Closing the application.");
						WriteLogFiles(msGMS(), CFG_FILE_LOG_DIRECTORY);
						DiscardAllResources();
					}
					// Closing the application
					break;
				case SYSTEM_CLOSE:
					// Shutting down the system
						logMessage("[SERVER] The computer is shutting down! Closing the application.");
						WriteLogFiles(msGMS(), CFG_FILE_LOG_DIRECTORY);
						DiscardAllResources(); 
						MessagePopup("Shutting down the system","The server will be closed now.");
					break;
			}
			break;
	}
	return 0;
}


void bgdFunc(void)
{
	processScheduleEvents();
	WriteLogFiles(msGMS(), CFG_FILE_LOG_DIRECTORY);
	msPrintMsgs(msGMS(), stdout);
	msFlushMsgs(msGMS());
}


void ubsBlockReconnection(int handle, int arg1) {
	if (!modbusBlockData.connectionInfo.connected) {
		ConnectToUbsBlock(CFG_UBS_CONNECTION_IP, CFG_UBS_CONNECTION_PORT, CFG_UBS_CONNECTION_TIMEOUT, &modbusBlockData);
		
		if (!modbusBlockData.connectionInfo.connected) {
			logMessage("[MODBUS-UBS] Next connection request will be in %d seconds.", CFG_UBS_RECONNECTION_DELAY);  
		}
	}
}


void ubsBlockReadData(int handle, int arg1) {
	if (modbusBlockData.connectionInfo.connected) {
		requestUbsData(modbusBlockData.connectionInfo.conversationHandle);
	}
}


void ubsBlockReadLogInfo(int handle, int arg1) {
	if (modbusBlockData.connectionInfo.connected && !modbusBlockData.logInfo.currentlyReading) {
		requestLogInfo(modbusBlockData.connectionInfo.conversationHandle);
	}
}


void adcFileWrite(int handle, int arg1) {
	char buffer[3000];

	if (modbusBlockData.connectionInfo.connected) {
		FormatUbsProcessedData(&modbusBlockData.processed_data, buffer);
		WriteDataFiles(buffer, CFG_FILE_DATA_DIRECTORY);
	}
}


void deleteOldFiles(int handle, int arg1) {
	DeleteOldFiles(CFG_FILE_LOG_DIRECTORY, CFG_FILE_DATA_DIRECTORY, CFG_FILE_EVENTS_DIRECTORY, CFG_FILE_EXPIRATION);
}


int prepareTimeSchedule(void) {
	// Send reconnection request
	if (addRecordToSchedule(1, 1, CFG_UBS_RECONNECTION_DELAY, ubsBlockReconnection, "reconnection", 0) < 0) {
		msAddMsg(msGMS(), "[ERROR] Unable to add reconnection event.");
		return -1;	
	}

	// Send data request
	if (addRecordToSchedule(1, 0, CFG_UBS_DATA_REQUEST_INTERVAL, ubsBlockReadData, "data request", 0) < 0) {
		msAddMsg(msGMS(), "[ERROR] Unable to add data update event.");
		return -1;	
	}

	// Send log info request
	if (addRecordToSchedule(1, 0, CFG_UBS_LOG_STATE_REQUEST_INTERVAL, ubsBlockReadLogInfo, "log info request", 0) < 0) {
		msAddMsg(msGMS(), "[ERROR] Unable to add status update event.");
		return -1;	
	}

	// COMMENT THE FOLLOWING IF SMTH GOES WRONG WITH LOGGING  
	// Send status request
	if (addRecordToSchedule(1, 0, CFG_FILE_DATA_WRITE_INTERVAL, adcFileWrite, "file write", 0) < 0) {
		msAddMsg(msGMS(), "[ERROR] Unable to add status update event.");
		return -1;	
	}

	// Send status request
	if (addRecordToSchedule(1, 1, 60 * 60 * 24, deleteOldFiles, "delete old files", 0) < 0) {
		msAddMsg(msGMS(), "[ERROR] Unable to add status update event");
		return -1;	
	}
	return 0;
}


void prepareTcpCommandOld(char *str,int bytes){
	int i;
	for (i=0; i<(bytes-1);i++) {
		if (str[i] == 0) str[i]=' ';
		  else str[i] = toupper(str[i]);
	}
	if (bytes == MAX_RECEIVED_BYTES) 
		str[MAX_RECEIVED_BYTES-1] = 0;
	else
		str[bytes] = 0;
}


/////////////////////////////////////////////////////////////////////
//=================================================================== 
//===================================================================
//===================================================================
//===================================================================
///////////////////////////////////////////////////////////////////// 

int main(int argc, char **argv) {
    #define defaultConfigFile "ecool-interlock-server-config.ini"
    char configFilePath[1024], serverName[256];

    switch (argc) {
        case 1: strcpy(configFilePath, defaultConfigFile); break;
        case 2: strcpy(configFilePath, argv[1]); break;
        default:
            MessagePopup("Command line arguments error", "Incorrect number of arguments");
            exit(1);
    }

    InitServerConfig(configFilePath);
    copyConfigurationFile(CFG_FILE_LOG_DIRECTORY, configFilePath);

    /////////////////////////////////
    // Body of the program
    /////////////////////////////////

    // Setup the console window
    sprintf(serverName, "Server: %s", CFG_SERVER_NAME);
    
    SetStdioPort(CVI_STDIO_WINDOW);
    SetStdioWindowOptions(1000, 0, 0);
    SetSystemAttribute(ATTR_TASKBAR_BUTTON_TEXT, serverName);
    SetStdioWindowVisibility(1);  // Show the console
    SetSleepPolicy(VAL_SLEEP_SOME);  // SUPER IMPORTANT!!! If not set, the application will work in slow mode (which can lead to the issues with TCP stacks, etc.)
    InstallMainCallback(userMainCallback, 0, 0);  // Install a callback to handle app events (we need it to process attempts of closing the application).

    atexit(DiscardAllResources);

    InitServerData();
    
    msInitGlobalStack();  // Initialize the global message stack 
    msAddMsg(msGMS(), "Configuration file: %s", configFilePath);
    msAddMsg(msGMS(), "Server name: %s", CFG_SERVER_NAME); 
    msAddMsg(msGMS(),"---------------------------\n[ECool Interlock Server -- NEW SESSION]\n---------------------------");   

    // Prepare a TCP server for communication with high-end clients. 
    tcpConnection_InitServerInterface(&tcpSI);
    tcpConnection_SetBackgroundFunction(&tcpSI, bgdFunc);
    tcpConnection_SetDataExchangeFunction(&tcpSI, dataExchFunc);

    InitCommandParsers();  

    if (prepareTimeSchedule() < 0) {
        msAddMsg(msGMS(), "[ERROR] Unable to schedule all necessary events.");
        WriteLogFiles(msGMS(), CFG_FILE_LOG_DIRECTORY); 
        msFlushMsgs(msGMS());
        MessagePopup("Internal error", "Unable to schedule events for interacting with devices. See the log file for more details.");
        return 0;
    }
                                                  
    //--------------------------------------------------------
    
    // Initial logging of everything that happend during the initialization of the application.
    WriteLogFiles(msGMS(), CFG_FILE_LOG_DIRECTORY);
    msPrintMsgs(msGMS(), stdout);
    msFlushMsgs(msGMS());      
    //--------------------------------------------------------
    
    // Running the server which puts the application to an infinite loop.
    // The loop can be exited by setting the server_active parameter of a TCP server interface (tcpSI) to zero.
    tcpConnection_RunServer(CFG_TCP_PORT, &tcpSI); 

    // When shut down normally, this section can be reached.
    // We then save all unsaved data and discard all dynamically allocated resources.
    WriteLogFiles(msGMS(), CFG_FILE_LOG_DIRECTORY);
    msPrintMsgs(msGMS(), stdout);
    msFlushMsgs(msGMS()); 
        
    DiscardAllResources();
    GetKey();

    return 0;
}
