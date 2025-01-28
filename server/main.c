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

#define configFile "ecool-interlock-server-config.ini"
#define MAX_RECEIVED_BYTES 3000

// Variables

modbus_block_data_t modbusBlockData = {0};

tcpConnection_ServerInterface_t tcpSI;

///////////////////////////////////////// 

/////////////////////////////////////////   SCHEDULE FUNCTIONS
void ubsBlockReconnection(int handle);
void ubsBlockReadData(int handle);


void adcFileWrite(int handle);
void deleteOldFiles(int handle);
/////////////////////////////////////////

/////////////////////////////////////////   CONFIGURATION FUNCTIONS 
void prepareTimeSchedule(void); 


void DiscardAllResources(void);
/////////////////////////////////////////


/////////////////////////////////////////   
void dataExchFunc(unsigned handle,void * arg);  


void bgdFunc(void);


void avoidZeroCharacters(char *str,int bytes);


/////////////////////////////////////////   FUNCTIONS DEFINITIONS   
void DiscardAllResources(void) {
	// TODO: disconenct from the UBS server
	UnregisterTCPServer(TCP_PORT);
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
						msAddMsg(msGMS(),"%s [SERVER] Closing the application.",TimeStamp(0));
						WriteLogFiles(msGMS(), FILE_LOG_DIRECTORY);
						DiscardAllResources();
					}
					// Closing the application
					break;
				case SYSTEM_CLOSE:
					// Shutting down the system
						msAddMsg(msGMS(),"%s [SERVER] The computer is shutting down! Closing the application.", TimeStamp(0));
						WriteLogFiles(msGMS(), FILE_LOG_DIRECTORY);
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
	WriteLogFiles(msGMS(), FILE_LOG_DIRECTORY);
	msPrintMsgs(msGMS(),stdout);
	msFlushMsgs(msGMS());
}


void ubsBlockReconnection(int handle) {
	if (!modbusBlockData.connectionInfo.connected) {
		ConnectToUbsBlock(UBS_CONNECTION_IP, UBS_CONNECTION_PORT, UBS_CONNECTION_TIMEOUT, &modbusBlockData);
		
		if (!modbusBlockData.connectionInfo.connected) {
			msAddMsg(msGMS(),"%s [MODBUS-UBS] Next connection request will be in %d seconds.",TimeStamp(0), UBS_RECONNECTION_DELAY);  
		}
	}
}


void ubsBlockReadData(int handle) {
	if (modbusBlockData.connectionInfo.connected) {
		requestUbsData(modbusBlockData.connectionInfo.conversationHandle);
	}
}


void ubsBlockReadLogInfo(int handle) {
	if (modbusBlockData.connectionInfo.connected && !modbusBlockData.logInfo.currentlyReading) {
		requestLogState(modbusBlockData.connectionInfo.conversationHandle);
	}
}


void adcFileWrite(int handle) {
	char buffer[3000];

	if (modbusBlockData.connectionInfo.connected) {
		FormatUbsProcessedData(&modbusBlockData.processed_data, buffer);
		WriteDataFiles(buffer, FILE_DATA_DIRECTORY);
	}
}


void deleteOldFiles(int handle) {
	DeleteOldFiles(FILE_LOG_DIRECTORY, FILE_DATA_DIRECTORY, FILE_EVENTS_DIRECTORY, FILE_EXPIRATION);
}


void prepareTimeSchedule(void) {
	addRecordToSchedule(1, 1, UBS_RECONNECTION_DELAY, ubsBlockReconnection);
	addRecordToSchedule(1, 0, UBS_DATA_REQUEST_INTERVAL, ubsBlockReadData);
	addRecordToSchedule(1, 0, UBS_LOG_STATE_REQUEST_INTERVAL, ubsBlockReadLogInfo);  // COMMENT THIS IF SMTH GOES WRONG WITH LOGGING    
	addRecordToSchedule(1, 0, FILE_DATA_WRITE_INTERVAL, adcFileWrite);  
	addRecordToSchedule(1, 1, 60 * 60 * 24, deleteOldFiles);     
}


void prepareTcpCommand(char *str,int bytes){
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


void dataExchFunc(unsigned handle,void * arg)
{
	static char command[MAX_RECEIVED_BYTES] = "";
	static char answer[MAX_RECEIVED_BYTES];
	char *lfp;
	int byteRecv, eofCounter;
	
	byteRecv = ServerTCPRead(handle, command, MAX_RECEIVED_BYTES - 1, 0);
	if ( byteRecv <= 0 )
	{
		msAddMsg(msGMS(),"%s [SERVER CLIENT] Error occured while receiving messages from the client >> %s", TimeStamp(0), GetTCPSystemErrorString());
		return;
	}

	command[byteRecv] = 0;  // Create a zero-ending string
	prepareTcpCommand(command, byteRecv);
	
	eofCounter = 0;
	lfp = command;
	
	while ( (lfp = strstr(lfp, "\n")) != NULL ) {
		eofCounter++;
		lfp++;
	}
	
	lfp = strstr(command, "\n");
	
	if (eofCounter == 0) {
		sprintf(answer, "ERROR: [%s] No end-of-line symbol.\n", command); 	
	} else if (eofCounter > 1){
		sprintf(answer, "ERROR: Multiple (%d) end-of-line symbols.\n", eofCounter); 	
	} else if (lfp == command) {
		sprintf(answer, "ERROR: Empty command.\n"); 
	} else {
		lfp[0] = 0;
		PrepareAnswerForClient(command, &modbusBlockData, answer);
	}
	
	if (ServerTCPWrite(handle, answer, strlen(answer), 100) < 0) {
		msAddMsg(msGMS(),"%s [SERVER CLIENT] Error occured while sending a message to the client >> %s", TimeStamp(0), GetTCPSystemErrorString());
	}
}


/////////////////////////////////////////////////////////////////////
//=================================================================== 
//===================================================================
//===================================================================
//===================================================================
///////////////////////////////////////////////////////////////////// 

int main() {
	/////////////////////////////////
	// Body of the program
	/////////////////////////////////
	InitServerConfig(configFile);
	copyConfigurationFile(FILE_DATA_DIRECTORY, configFile);

	// Initialize modbus block data with zeros 
	// (in case a client connect before we connect to the UBS block, 
	// so we can send something to the client)
	memset(&modbusBlockData, 0, sizeof(modbusBlockData));
	
	msInitGlobalStack();  // Initialize the global message stack 
	msAddMsg(msGMS(),"---------------------------\n[UBS Server -- NEW SESSION]\n---------------------------");   
	
	prepareTimeSchedule();

	// Prepare a TCP server for communication with high-end clients. 
	tcpConnection_InitServerInterface(&tcpSI);
	tcpConnection_SetBackgroundFunction(&tcpSI, bgdFunc);
	tcpConnection_SetDataExchangeFunction(&tcpSI, dataExchFunc);
												  
	// CVI window setup
	SetStdioWindowOptions (10000, 0, 0);
	SetStdioWindowVisibility(1);  // Show the console
	InstallMainCallback(userMainCallback, 0, 0);  // Install a callback to handle app events (we need it to process attempts of closing the application).
	SetSleepPolicy(VAL_SLEEP_SOME);  // SUPER IMPORTANT!!! If not set, the application will work in slow mode (which can lead to the issues with TCP stacks, etc.)
	//--------------------------------------------------------
	
	// Initial logging of everything that happend during the initialization of the application.
	WriteLogFiles(msGMS(), FILE_LOG_DIRECTORY);
	msPrintMsgs(msGMS(), stdout);
	msFlushMsgs(msGMS());  	   
	//--------------------------------------------------------
	
	// Running the server which puts the application to an infinite loop.
	// The loop can be exited by setting the server_active parameter of a TCP server interface (tcpSI) to zero.
	tcpConnection_RunServer(TCP_PORT, &tcpSI); 

	// When shut down normally, this section can be reached.
	// We then save all unsaved data and discard all dynamically allocated resources.
	WriteLogFiles(msGMS(), FILE_LOG_DIRECTORY);
	msPrintMsgs(msGMS(), stdout);
	msFlushMsgs(msGMS()); 
		
	DiscardAllResources();
	GetKey();

	return 0;
}
