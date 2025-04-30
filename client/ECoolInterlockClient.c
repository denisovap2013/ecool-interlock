
//==============================================================================
// Include files

#include <tcpsupp.h>
#include <ansi_c.h>
#include <cvirte.h>     
#include <userint.h>

#include "TimeMarkers.h"
#include "MessageStack.h"
#include "clientConfiguration.h"

#include "clientInterface.h"
#include "ECoolInterlockClient.h"
#include "conversation.h"
#include "ClientLogging.h"  


#include "toolbox.h"

//==============================================================================
// Constants

#define TIMER_TICK_TIME 0.040
#define CONNETCION_DELAY_TIME   40
#define BLOCK_REQUEST_INTERVAL  4 

#define MAX_RECEIVED_BYTES 3000

#define MODULE_STATUS_REQUEST_PERIOD 3

//==============================================================================
// Types

//==============================================================================
// Static global variables

unsigned int serverHandle; 
int connectionEstablished;

int connectionWaitTics;

char logFileName[256], dataFileName[256], infoFileName[256];
int infoFileCreated = 0;

#define WAIT_FOR_BLOCK_NAMES 1

//==============================================================================
// Static functions

int initConnectionToServer(void);
void initLogAndDataFilesNames(void);
void createInfoFile(void);
void RequestNames(void);

int PrepareDataRequestOnSchedule(void);
int PrepareModuleStatusRequestOnSchedule(void);

int WriteDataFilesOnSchedule(void);

void DiscardAllResources(void);  
//==============================================================================

//==============================================================================
// Global functions
void assembleDataForLogging(char * target);

/// HIFN The main entry-point function.
#define configFile "ecool-interlock-client-config.ini"


void DiscardAllResources(void) {
	int i,j; 
	
	if (mainPanelHandle >= 0) DiscardPanel (mainPanelHandle);
	if (helpPanelHandle >= 0) DiscardPanel (helpPanelHandle); 
	if (eventPanelHandle >= 0) DiscardPanel (eventPanelHandle); 
	
	for (i=0; i<DI_NUMBER; i++) {
		if (diPanelHandles[i] >= 0) DiscardPanel (diPanelHandles[i]); 
	}
	
	for (i=0; i<DQ_NUMBER; i++) {
		if (dqPanelHandles[i] >= 0) DiscardPanel (dqPanelHandles[i]); 
	}
	
	for (i=0; i<ADC_NUMBER; i++) {
		if (adcPanelHandles[i] >= 0) DiscardPanel (adcPanelHandles[i]); 
		for (j=0; j<CHANNELS_PER_ADC; j++) {
			if (adcGraphPanels[i][j] >= 0)
				DiscardPanel (adcGraphPanels[i][j]); 	
		}
	}
	
	for (i=0; i<DAC_NUMBER; i++) {
		if (dacPanelHandles[i] >= 0) DiscardPanel (dacPanelHandles[i]); 
	}
	
	if(connectionEstablished) {
		DisconnectFromTCPServer(serverHandle);		
	}
	
	msReleaseGlobalStack();
}


int initConnectionToServer(void) {
	return ConnectToTCPServer(&serverHandle, CFG_SERVER_PORT, CFG_SERVER_IP, clientCallbackFunction, NULL, 100);	
}


void initLogAndDataFilesNames(void) {
	static int day, month, year, hours, minutes, seconds;
	char nameBase[256];
	
	GetSystemDate(&month, &day, &year);
	GetSystemTime(&hours, &minutes, &seconds);

	sprintf(nameBase, "%02d.%02d.%02d_%02d-%02d-%02d", year, month, day, hours, minutes, seconds);
	sprintf(logFileName, "ubsClientLog_%s.dat", nameBase);
	sprintf(dataFileName, "ubsClientData_%s.dat", nameBase);
	sprintf(infoFileName, "ubsClientInfo_%s.txt", nameBase);
}


void createInfoFile(void) {
	message_stack_t messageStack;
	int i, j, columnIndex;
	
	msInitStack(&messageStack);
	
	// Corresponding log / data files
	msAddMsg(messageStack, "[Log files]"); 
	msAddMsg(messageStack, "  Log file: %s", logFileName);
	msAddMsg(messageStack, "  Data file: %s", dataFileName);
	msAddMsg(messageStack, "\n");  
	
	// Data columns description
	msAddMsg(messageStack, "[Data columns description]");
	msAddMsg(messageStack, "  Calendar time (seconds)");
	msAddMsg(messageStack, "  DI blocks data (%d hex numbers, 32 channels each)", DI_NUMBER);
	msAddMsg(messageStack, "  DQ blocks data (%d hex numbers, 16 channels each)", DQ_NUMBER);
	msAddMsg(messageStack, "  ADC blocks data (%d blocks with %d float point numbers each)", ADC_NUMBER, CHANNELS_PER_ADC);
	msAddMsg(messageStack, "  DAC blocks data (%d blocks with %d float point numbers each)", DAC_NUMBER, CHANNELS_PER_DAC);
	msAddMsg(messageStack, "  Diagnostics data (a single hex number, 16 bits)"); 
	
	msAddMsg(messageStack, "\n");
	
	
	// Extended data columns description
	msAddMsg(messageStack, "[Data columns detailed description]");
	columnIndex = 0;
	
	msAddMsg(messageStack, "  %02d: time", columnIndex++); 
	
	for (i=0; i<DI_NUMBER; i++) {
		msAddMsg(messageStack, "  %02d: DI-%d (hex, 32 bits)", columnIndex++, i);
	}
	
	for (i=0; i<DQ_NUMBER; i++) {
		msAddMsg(messageStack, "  %02d: DQ-%d (hex, 16 bits)", columnIndex++, i);
	}
	
	for (i=0; i<ADC_NUMBER; i++) {
		for (j=0; j<CHANNELS_PER_ADC; j++) 
			msAddMsg(messageStack, "  %02d: ADC-%d channel %02d. %s", columnIndex++, i, j, adcTextLabels[i][j]);
	}
	
	for (i=0; i<DAC_NUMBER; i++) {
		for (j=0; j<CHANNELS_PER_DAC; j++) 
			msAddMsg(messageStack, "  %02d: DAC-%d channel %02d. %s", columnIndex++, i, j, dacTextLabels[i][j]);
	}
	
	msAddMsg(messageStack, "  %02d: diagnostics (hex, 16 bits)", columnIndex++); 
	
	msAddMsg(messageStack, "\n");
	
	// Blocks description
	
	msAddMsg(messageStack, "[DI and DQ blocks]\n(registers are coded in the order from right to left)"); 
	
	for (i=0; i<DI_NUMBER; i++) {
		msAddMsg(messageStack, ""); 
		msAddMsg(messageStack, "DI-%d:", i);
		for (j=0; j<CHANNELS_PER_DI; j++) 
			msAddMsg(messageStack, "  %02d: %s", j, diTextLabels[i][j]);

	}
	
	for (i=0; i<DQ_NUMBER; i++) {
		msAddMsg(messageStack, "");
		msAddMsg(messageStack, "DQ-%d:", i);
		for (j=0; j<CHANNELS_PER_DQ; j++) 
			msAddMsg(messageStack, "  %02d: %s", j, dqTextLabels[i][j / 2]);
	}
	
	
	// ADC description
	msAddMsg(messageStack, "\n");
	msAddMsg(messageStack, "[ADC blocks]");
	
	for (i=0; i<ADC_NUMBER; i++) {
		msAddMsg(messageStack, "");
		msAddMsg(messageStack, "ADC-%d:", i);
		for (j=0; j<CHANNELS_PER_ADC; j++) 
			msAddMsg(messageStack, "  %02d: %s", j, adcTextLabels[i][j]);
	}
	
	// DAC description
	msAddMsg(messageStack, "\n");
	msAddMsg(messageStack, "[DAC blocks]");
	
	for (i=0; i<DAC_NUMBER; i++) {
		msAddMsg(messageStack, "");
		msAddMsg(messageStack, "DAC-%d:", i);
		for (j=0; j<CHANNELS_PER_DAC; j++) 
			msAddMsg(messageStack, "  %02d: %s", j, dacTextLabels[i][j]);
	}
	
	// Diagnostics
	msAddMsg(messageStack, "\n");
	msAddMsg(messageStack, "[Diagnostics]");
	msAddMsg(messageStack, "(devices are coded in the reversed order, read bits from left to right)"); 
	msAddMsg(messageStack, "(0 (zero bit) - OK, 1 (non-zero bit) - ERROR)");
	
	msAddMsg(messageStack, "  00: DI-0");  
	msAddMsg(messageStack, "  01: DI-1");

	msAddMsg(messageStack, "  02: DQ-0");
	msAddMsg(messageStack, "  03: DQ-1");
	msAddMsg(messageStack, "  04: DQ-2");
	
	msAddMsg(messageStack, "  05: ADC-0");
	msAddMsg(messageStack, "  06: ADC-1");
	msAddMsg(messageStack, "  07: ADC-2");
	msAddMsg(messageStack, "  08: ADC-3");
	msAddMsg(messageStack, "  09: DAC-0");
	
	WriteDataDescription(messageStack, CFG_DATA_DIRECTORY, infoFileName);
}


void RequestNames(void) {
	int i;
	char command[256];
	int flags[5] = {0, 0, 0, 0, 0};
	
	// DI
	sprintf(command, "%s\n", CMD_GET_DI_NAMES);
	appendGlobalRequestQueueRecord(CMD_GET_DI_NAMES_ID, command, NULL);
	
	// DQ
	sprintf(command, "%s\n", CMD_GET_DQ_NAMES);
	appendGlobalRequestQueueRecord(CMD_GET_DQ_NAMES_ID, command, NULL);
	
	// ADC
	for(i=0; i<ADC_NUMBER; i++) {
		sprintf(command, "%s %d\n", CMD_GET_ADC_NAMES, i);
		flags[0] = i;
		appendGlobalRequestQueueRecord(CMD_GET_ADC_NAMES_ID, command, &flags);
	}
	
	// DAC
	for(i=0; i<DAC_NUMBER; i++) {
		sprintf(command, "%s %d\n", CMD_GET_DAC_NAMES, i);
		flags[0] = i;
		appendGlobalRequestQueueRecord(CMD_GET_DAC_NAMES_ID, command, &flags);
	}
}



int PrepareModuleStatusRequestOnSchedule(void) {
	char command[256];
	static moduleStatusWaitingTics = 0;
	
	if (!connectionEstablished) return 0;
	
	if (hasSameGlobalRequestsRecordID(CMD_GET_CONNECTION_STATE_ID)) return 0;

	if (moduleStatusWaitingTics * TIMER_TICK_TIME >= MODULE_STATUS_REQUEST_PERIOD) {
		sprintf(command,"%s\n", CMD_GET_CONNECTION_STATE); 
		
		if (appendGlobalRequestQueueRecord(CMD_GET_CONNECTION_STATE_ID, command, NULL)) {
			moduleStatusWaitingTics = 0; 
			return 1;
		}
		
	} else {
		moduleStatusWaitingTics++;
	}
	
	return 0;
}


int PrepareDataRequestOnSchedule(void) {
	char command[256];
	static ubsRequestWaitingTics = 0;	
	
	if (!connectionEstablished) return 0;
	
	if (hasSameGlobalRequestsRecordID(CMD_GET_ALLVALUES_ID)) return 0;

	if (ubsRequestWaitingTics * TIMER_TICK_TIME >= CFG_SERVER_REQUEST_RATE) {
		sprintf(command,"%s\n", CMD_GET_ALLVALUES); 

		if (appendGlobalRequestQueueRecord(CMD_GET_ALLVALUES_ID, command, NULL)) {
			ubsRequestWaitingTics = 0; 
			return 1;
		}
		
	} else {
		ubsRequestWaitingTics++;
	}
	
	return 0;
}



int WriteDataFilesOnSchedule(void) {
	static int dataLogWaitingTics = 0;
	static char data[2048];

	if (!connectionEstablished) return 0;
	
	if (dataLogWaitingTics * TIMER_TICK_TIME >= CFG_DATA_WRITE_INTERVAL) {
		assembleDataForLogging(data);
		WriteDataFiles(data, CFG_DATA_DIRECTORY, dataFileName);
		dataLogWaitingTics = 0;
		return 1;
	} else {
		dataLogWaitingTics++;	
	}	

	return 0;
}


void prepareTcpCommand(char *str,int bytes){
	int i;
	for (i=0; i<(bytes-1);i++) {
		if (str[i] == 0) str[i]=' ';
		  else str[i] = toupper(str[i]);
	}
	if (bytes == MAX_RECEIVED_BYTES) str[MAX_RECEIVED_BYTES-1] = 0;
}


void assembleDataForLogging(char * target) {
	int i,j;
	char buffer[256];
	
	target[0] = 0;
	
	for (i=0; i< DI_NUMBER; i++) {
		sprintf(buffer, "%08X ", DI_VALUES[i]);  // Space added to the end of the string on purpose
		strcat(target, buffer); 	
	}
	
	for (i=0; i< DQ_NUMBER; i++) {
		sprintf(buffer, "%04X ", DQ_VALUES[i]);  // Space added to the end of the string on purpose
		strcat(target, buffer); 	
	}
	
	for (i=0; i< ADC_NUMBER; i++) {
		for(j=0; j<CHANNELS_PER_ADC; j++) {
			sprintf(buffer, "%.8lf ", ADC_VALUES[i][j]);  // Space added to the end of the string on purpose
			strcat(target, buffer);	
		}
	}
	
	for (i=0; i< DAC_NUMBER; i++) {
		for(j=0; j<CHANNELS_PER_DAC; j++) {
			sprintf(buffer, "%.8lf ", DAC_VALUES[i][j]);  // Space added to the end of the string on purpose
			strcat(target, buffer);	
		}
	}
	
	sprintf(buffer, "%04X", DEVICES_DIAGNOSTICS);  // It is the last value. Therefore the end-of-line symbol is added.
	strcat(target, buffer);	
}

//============================================================================== 

//==============================================================================
// UI callback function prototypes

/// HIFN Exit when the user dismisses the panel.
int CVICALLBACK mainPanelCallback (int panel, int event, void *callbackData,
        int eventData1, int eventData2)
{
    if (event == EVENT_CLOSE) {
        QuitUserInterface (0);
	}
    return 0;
}


int CVICALLBACK helpPanelCallback (int panel, int event, void *callbackData,
        int eventData1, int eventData2)
{
    if (event == EVENT_CLOSE) {
        HidePanel(panel);
	}
    return 0;
}

int CVICALLBACK graphVerticalRange (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double ymin, ymax;
	double buf;
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(panel, Graph_maxValue, &ymax);
			GetCtrlVal(panel, Graph_minValue, &ymin);
			if (ymax < ymin)  {
				buf = ymax;
				ymax = ymin;
				ymin = buf;
				SetCtrlVal(panel, Graph_maxValue, ymax);
				SetCtrlVal(panel, Graph_minValue, ymin);
			}
			
			if (ymax == ymin) {
				ymax += 1;
				SetCtrlVal(panel,Graph_maxValue,ymax);
			}
			
			SetAxisScalingMode(panel,Graph_GRAPH,VAL_LEFT_YAXIS,VAL_MANUAL,ymin,ymax);
			break;
	}	 
	return 0;
}

int  CVICALLBACK fitPlotButtonCallback(int panel, int control, int event, 
	     void *callbackData, int eventData1, int eventData2)
{
	double ymin, ymax, diff;
	int axisMode;
	switch (event)
	{
		case EVENT_COMMIT:
			SetAxisScalingMode(panel, Graph_GRAPH, VAL_LEFT_YAXIS, VAL_AUTOSCALE, 0, 0);
			ProcessDrawEvents();  
			GetAxisScalingMode(panel, Graph_GRAPH, VAL_LEFT_YAXIS, &axisMode, &ymin, &ymax); 
			
			diff = ymax - ymin;
			ymax += diff * 0.1;
			ymin -= diff * 0.1;
			
			SetAxisScalingMode(panel, Graph_GRAPH, VAL_LEFT_YAXIS, VAL_MANUAL, ymin, ymax); 
			SetCtrlVal(panel, Graph_maxValue, ymax);
			SetCtrlVal(panel, Graph_minValue, ymin);
			break;
	}	 
	return 0;		
}

int  CVICALLBACK clearPlotCallback(int panel, int control, int event, 
	     void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			ClearStripChart(panel, Graph_GRAPH);
			break;
	}	 
	return 0;		
}


int CVICALLBACK graphPanelCallback (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	int i, j;
	int h, w;
	switch (event) {
		case EVENT_PANEL_SIZE:  
			GetPanelAttribute(panel, ATTR_HEIGHT, &h);
			if (h<150) SetPanelAttribute(panel, ATTR_HEIGHT, 150);
			GetPanelAttribute(panel, ATTR_WIDTH, &w);
			if (w<300) SetPanelAttribute(panel, ATTR_WIDTH, 300);
		case EVENT_PANEL_SIZING:
			
			GetPanelAttribute(panel, ATTR_WIDTH, &w);
			GetPanelAttribute(panel, ATTR_HEIGHT, &h);
			SetCtrlAttribute(panel, Graph_GRAPH,ATTR_WIDTH, w-113);
			SetCtrlAttribute(panel, Graph_GRAPH,ATTR_HEIGHT, h - 30); 
			SetCtrlAttribute(panel, Graph_currentValue,ATTR_TOP, h/2); 
			SetCtrlAttribute(panel, Graph_minValue,ATTR_TOP, h-30);
			break;
		case EVENT_CLOSE:
			DiscardPanel(panel);
			for (i=0; i<ADC_NUMBER; i++) {
				for (j=0; j<CHANNELS_PER_ADC; j++) {
					if (adcGraphPanels[i][j] == panel) {
						adcGraphPanels[i][j] = -1;
						break;
					}
				}
				if (j < CHANNELS_PER_ADC) break;
			}
			break;
	}  
	return 0;
}

int CVICALLBACK blockPanelCallback (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_CLOSE:
			HidePanel(panel);
			break;
	}
	return 0;
}

int CVICALLBACK tick (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_TIMER_TICK:
			if (connectionEstablished) {
				// sending request to the server
				PrepareModuleStatusRequestOnSchedule();
				PrepareDataRequestOnSchedule();
				
				// Process the global requests queue
				sendRequestIfAvailable(serverHandle);
				
				// writing data to files
				WriteDataFilesOnSchedule();

			} else {
				// Trying to reconnect
				connectionWaitTics++;
				if (connectionWaitTics * TIMER_TICK_TIME >= CFG_SERVER_CONNECTION_INTERVAL) {
					connectionWaitTics = 0;

					msAddMsg(msGMS(), "%s Connection to the server (\"%s:%d\") ...", TimeStamp(0), CFG_SERVER_IP, CFG_SERVER_PORT);
					if (initConnectionToServer() >= 0) {
						connectionEstablished = 1;
						RequestNames();
						
						if (!infoFileCreated) {
							infoFileCreated = 1;
							createInfoFile();
						}
						
						SetCtrlVal(mainPanelHandle, clientToServerConnectionLED, 1);
						SetCtrlAttribute(mainPanelHandle, clientToServerConnectionLED, ATTR_LABEL_TEXT, "Clinet-Server: online");
						msAddMsg(msGMS(), "%s Connection to the server is established.", TimeStamp(0));
					} else {
						msAddMsg(msGMS(), "%s Connection to the server failed. Next connection request in %.1fs.", TimeStamp(0), CFG_SERVER_CONNECTION_INTERVAL);
					}
				}
			}
			if (msMsgsAvailable(msGMS())) {
				WriteLogFiles(msGMS(), CFG_LOG_DIRECTORY, logFileName);
				msFlushMsgs(msGMS());
			}
			break;
	}
	return 0;
}


int clientCallbackFunction(unsigned handle, int xType, int errCode, void * callbackData){
	char message[8192];
	char *pos;
	int bytesRecv;
	
	switch(xType)
	{
		case TCP_DISCONNECT:
			connectionEstablished = 0;
			clearGlobalRequestsQueue();
			
			SetCtrlVal(mainPanelHandle, clientToServerConnectionLED, 0);
			SetCtrlAttribute(mainPanelHandle, clientToServerConnectionLED, ATTR_LABEL_TEXT, "Clinet-Server: offline");
			msAddMsg(msGMS(), "%s Connection to the server has lost. Next connection request in %.1fs.", TimeStamp(0), CFG_SERVER_CONNECTION_INTERVAL);
			break;
		case TCP_DATAREADY:
			bytesRecv = ClientTCPRead(handle, message, sizeof(message) - 1, 100);
			message[bytesRecv] = 0;  // enforce the zero ending

			// Check for errors
			pos = strstr(message, "ERROR");
			
			if (pos && pos == message) {
				msAddMsg(msGMS(), "%s Error! Server request failed: %s", TimeStamp(0), message);  // Server answer already contain \n symbol at the end of the string.
			} else {
				//printf("Answer from server: %s\n", message);  // For debugging
				
				switch (globalRequestState.requestID) {
					case CMD_GET_DI_NAMES_ID:
						SetDiNames(message + strlen(CMD_GET_DI_NAMES) + 1); break;
			
					case CMD_GET_DQ_NAMES_ID:
						SetDqNames(message + strlen(CMD_GET_DQ_NAMES) + 1); break;
																																			 
					case CMD_GET_ADC_NAMES_ID:
						SetAdcNames(globalRequestState.flags[0], message + strlen(CMD_GET_ADC_NAMES) + 1); break;
																																											
					case CMD_GET_DAC_NAMES_ID:
						SetDacNames(globalRequestState.flags[0], message + strlen(CMD_GET_DAC_NAMES) + 1); break;
																																											
					case CMD_GET_ALLVALUES_ID:
						if (ParseValues(message + strlen(CMD_GET_ALLVALUES)) == 0) { 
							UpdateAllValues();
							UpdateAdcGraphs();
						} else
							msAddMsg(msGMS(), "%s Error! Unable to parse the following UBS data: %s.\n", TimeStamp(0), message); 
						break;	
					
					case CMD_GET_CONNECTION_STATE_ID:
						if (ParseConnectionState(message + strlen(CMD_GET_CONNECTION_STATE)) == 0) UpdateConnectionStateIndicator(); 
						break;
					
					case CMD_GET_EVENTS_ID:
						if (ParseEvents(message + strlen(CMD_GET_EVENTS) + 1) < 0) {
							// Error occurred. Clear the events list
							ClearListCtrl(eventPanelHandle, eventPanel_LISTBOX);
							InsertListItem(eventPanelHandle, eventPanel_LISTBOX, 0, "Error occurred", 0);
							UpdateTheEventsIndicators(-1);
						} else {
							PrintTheEventsList();	
						}
						break;
						
					case -1:
						msAddMsg(msGMS(), "%s Error! Data recieved while request type is set to -1: %s\n", TimeStamp(0), message); break;
						
					default:
						msAddMsg(msGMS(), "%s Error! Unhandled request type specified: %d\n", TimeStamp(0), globalRequestState.requestID);
				}
			}
			
			globalRequestState.finished = 1;
			break;
	}
	return 0;
}

int CVICALLBACK mainSystemCallback (int handle, int control, int event, void *callbackData, 
									int eventData1, int eventData2)
{
	switch(event) {
		case EVENT_END_TASK: 
			switch(eventData1)
			{
				case APP_CLOSE:
				case SYSTEM_CLOSE:
					DiscardAllResources();
					break;
			}
			break;	
	}
	return 0;
}


int main (int argc, char *argv[])
{
    int error = 0;
	
	ConfigurateClient(configFile);
	// Prepare log and data files names
	initLogAndDataFilesNames();
	
	msInitGlobalStack();
	msAddMsg(msGMS(), "-------------\n[NEW SESSION]\n-------------");
	
	// Force the app to connect to the server immediately  
	connectionWaitTics = CFG_SERVER_CONNECTION_INTERVAL / TIMER_TICK_TIME;  
    
    /* initialize and load resources */
    nullChk (InitCVIRTE (0, argv, 0));
    errChk (mainPanelHandle = LoadPanel (0, "ECoolInterlockClient.uir", BlockMenu));
	errChk (helpPanelHandle = LoadPanel (0, "ECoolInterlockClient.uir", DESCR)); 
	errChk (eventPanelHandle = LoadPanel (0, "ECoolInterlockClient.uir", eventPanel));
	
	// NEW ELEMENTS
	errChk (CreateBlocksGui());
	SetCtrlAttribute(mainPanelHandle, BlockMenu_TIMER, ATTR_INTERVAL, TIMER_TICK_TIME); 
    /////////////
	
    /* display the panel and run the user interface */
	InstallMainCallback(mainSystemCallback, 0, 0); 
	
    errChk (DisplayPanel(mainPanelHandle));
    errChk (RunUserInterface());

Error:
    /* clean up */
    DiscardAllResources();
	return 0;
}
