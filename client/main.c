#include "eventUBSclient.h"

#include <ansi_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UBSServerSettings.h"

#pragma comment(lib,"cangw.lib")
#include "cangw.h"

#include <Windows.h>

#include "TimeMarkers.h"
#include "TCP_Connection.h"
#include "dataskt.h"
#include <utility.h>
#include <userint.h>
#include <tcpsupp.h>
#include "CGW_Connection.h"
#include "MessageStack.h"
#include "CGW_CDAC20.h"
#include "CGW_CEAD20.h"  
#include "CGW_CEDIO_A.h" 
#include "CGW_Devices.h"
#include "ServerConfigData.h"
#include "CedioEvents.h"  

#define configFile "UBS server config.ini"

// Variables
cgw_devices_t deviceKit;
int eventServerHandle = -1;
int eventClients[5] = {-1,-1,-1,-1,-1}; 

CANGW_CONN_T conn_id = CANGW_ERR;
tcpConnection_ServerInterface_t tcpSI;

CedioEvent * bitBlockEvents = NULL, *bitBlockLastEvent = NULL;
/////////////////////////////////////////    SCHEDULE IDs   
int reconnectionRequestId = -1; 
int dsReconnectionRequest = -1;
int deviceXtimeoutHandle[64] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
								-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
								-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
								-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
///////////////////////////////////////// 

/////////////////////////////////////////   SCHEDULE FUNCTIONS
void serverReconnection(int handle);


void deviceXtimeout(int deviceNumber);
void deviceBIT0timeout(int handle);
void deviceBIT1timeout(int handle);
void deviceBIT2timeout(int handle);
void deviceBIT3timeout(int handle);
void deviceBIT4timeout(int handle);
void deviceBIT5timeout(int handle);
void deviceADCtimeout(int handle);

void deviceCedioRequest(int handle);   // And ADC output registers
void adcStatusRequest(int handle);    
void adcFileWrite(int handle);

void deleteOldFiles(int handle);
/////////////////////////////////////////

/////////////////////////////////////////   CONFIGURATION FUNCTIONS 
void prepareTimeSchedule(void);    
void preparationFunc(CANGW_CONN_T conid); 

void Config_Bits(unsigned char num);
void Config_ADC(unsigned char num);

void DiscardAllResources(void);
/////////////////////////////////////////

/////////////////////////////////////////   HOOK FUNCTIONS
void hookCedioRequest(cangw_msg_t msg, void * attr);
void hookUbsAnswers(cangw_msg_t msg, void * attr);
/////////////////////////////////////////   
void WriteLogFiles(void);
void WriteAdcFiles(void);

void dataExchFunc(unsigned handle,void * arg);  

void bgdFunc(void);
int tcpEventServerCallback(unsigned handle, int xType, int errCode, void * callbackData);

void getAllAdc(char *);
void getAllBits(char *);
void getAllBitsDec(char *);
void getOutReg(char *);
void getOutRegDec(char *);
void setOutReg(char *);

void avoidZeroCharacters(char *str,int bytes);



/////////////////////////////////////////   FUNCTIONS DEFINITIONS   
void WriteLogFiles(void){
	static char dir[500];
	static char fileName[256];
	static int day,month,year;
	FILE * outputFile;
	
	if ( msMsgsAvailable(msGMS()) ) {
		////
		GetSystemDate(&month,&day,&year);
		GetProjectDir(dir); 
		strcat(dir,"\\"); 
		strcat(dir,FILE_LOG_DIRECTORY);
		if (SetDir(dir) < 0) {
			if (MakeDir(dir) == 0) {
				SetDir(dir);	
			}
		}
		////
		sprintf(fileName,"ubsServerLog_%d.%02d.%02d.dat",year,month,day);    
		outputFile = fopen(fileName,"a");
		if (outputFile == NULL) {
			// Inform about errors
		} else {
			msPrintMsgs(msGMS(),outputFile);
			fclose(outputFile);
		}
	}
}

void WriteAdcFiles(void) {
	static char dir[500];  
	static char fileName[256];
	static int day,month,year;
	static char sAdc[256],sBits[100],sReg[15]; // plus CEDIO information
	static time_t curTime;
	FILE * outputFile;
	
	GetSystemDate(&month,&day,&year);
	GetProjectDir(dir); 
	strcat(dir,"\\");  
	strcat(dir,FILE_DATA_DIRECTORY);
	if (SetDir(dir) < 0) {
		if (MakeDir(dir) == 0) {
			SetDir(dir);	
		}
	}
	sprintf(fileName,"ubsServerAdc_%d.%02d.%02d.dat",year,month,day);    
	outputFile = fopen(fileName,"a");
	if (outputFile == NULL) {
		// Inform about errors
	} else {
		time(&curTime);
		getAllAdc(sAdc);
		getAllBitsDec(sBits);
		getOutRegDec(sReg);
		fprintf(outputFile,"%u %s %s %s\n",curTime,sAdc,sBits,sReg);
		fclose(outputFile);
	}
}

void DiscardAllResources(void) {
	if (conn_id != CANGW_ERR) cgwConnection_Release(conn_id);
	cgwDevices_ReleaseAll(&deviceKit);
	UnregisterTCPServer(TCP_PORT);
	if (eventServerHandle >= 0) {
		UnregisterTCPServer(TCP_EVENT_SERVER_PORT);	
	}
		
	msReleaseGlobalStack();
	clear_OutOfDate_Data(bitBlockEvents,time(NULL),0);		
}

int CVICALLBACK userMainCallback(int MenuBarHandle, int MenuItemID, int event, void * callbackData, 
	int eventData1, int eventData2)
{
	static char stime[30];
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
						TimeStamp(stime);   
						msAddMsg(msGMS(),"%s [SERVER] Closing the application.",stime);
						WriteLogFiles();
						DiscardAllResources();
					}
					// Closing the application
					break;
				case SYSTEM_CLOSE:
					// Shutting down the system
						TimeStamp(stime);   
						msAddMsg(msGMS(),"%s [SERVER] The computer is shutting down! Closing the application.",stime);
						WriteLogFiles();
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
	static char stime[30];
	static int connection_is_active = 1;
	
	
	
	//static int connection_delay = 0;
	cangw_msg_t msg_from_server;
	int cmsg; 
	
	cmsg = cgwConnection_Recv(conn_id,&msg_from_server,CANGW_RECV_TIMEOUT,"background looped process");
	TimeStamp(stime);
		if (cmsg > 0)
		{
			if (connection_is_active == 0)
			{
				connection_is_active = 1;
				msAddMsg(msGMS(),"%s [CANGW] CanGw connection has been restored.",stime);
			}
			
			cgwDevices_MessageHook(deviceKit,msg_from_server);
		}
		if (cmsg == CANGW_ERR)
		{
			if (connection_is_active == 1)
			{
				connection_is_active = 0;
				msAddMsg(msGMS(),"%s [CANGW] CanGw connection is lost.",stime);	
				activateScheduleRecord(reconnectionRequestId,1);
			}
		}
	////////////////
		
	processScheduleEvents();
	WriteLogFiles();
	msPrintMsgs(msGMS(),stdout);
	msFlushMsgs(msGMS());
}

void Config_Bits(unsigned char num)
{
	cgwCEDIO_A_Configurate(conn_id,num,0xFFFF); 
	cgwCEDIO_A_AttributesRequest(conn_id,num);
	cgwCEDIO_A_RegistersRequest(conn_id,num);
}
void Config_ADC(unsigned char num)
{
	// 23
	cgwCEAD20_MultiChannelConfig(conn_id, num,
			0,19,CEAD20_TIME_20MS,							 // ?????????????????????????   __40__?
			CEAD20_MODE_CONTINUOUS | CEAD20_MODE_TAKEOUT,0); 
	cgwCEAD20_AttributesRequest(conn_id,num);
}


void serverReconnection(int handle) {
	static char stime[30];
	
	if (cgwConnectionBroken) {
		
		conn_id = cgwConnection_Init(CANGW_IP,CANGW_PORT,125,5);
		if (conn_id >= 0) {
			cgwConnectionBroken = 0;	
			cgwConnection_PrepareForWork(conn_id,preparationFunc);	
			deactivateScheduleRecord(reconnectionRequestId);
			// +
				resetScheduleRecord(deviceXtimeoutHandle[UBS_ADC_PORT]); 
				resetScheduleRecord(deviceXtimeoutHandle[UBS_BIT_PORT[0]]); 
				resetScheduleRecord(deviceXtimeoutHandle[UBS_BIT_PORT[1]]); 
				resetScheduleRecord(deviceXtimeoutHandle[UBS_BIT_PORT[2]]); 
				resetScheduleRecord(deviceXtimeoutHandle[UBS_BIT_PORT[3]]); 
				resetScheduleRecord(deviceXtimeoutHandle[UBS_BIT_PORT[4]]); 
				resetScheduleRecord(deviceXtimeoutHandle[UBS_BIT_PORT[5]]); 
			
		} else {
			 ////////
			TimeStamp(stime);
			msAddMsg(msGMS(),"%s [CANGW] Next connection request will be in 5 seconds.",stime);  
		}
	}
}

void deviceCedioRequest(int handle) {   // And ADC output registers
	if (conn_id >= 0) {
		cgwCEDIO_A_RegistersRequest(conn_id,UBS_BIT_PORT[0]);
		cgwCEDIO_A_RegistersRequest(conn_id,UBS_BIT_PORT[1]); 
		cgwCEDIO_A_RegistersRequest(conn_id,UBS_BIT_PORT[2]); 
		cgwCEDIO_A_RegistersRequest(conn_id,UBS_BIT_PORT[3]); 
		cgwCEDIO_A_RegistersRequest(conn_id,UBS_BIT_PORT[4]); 
		cgwCEDIO_A_RegistersRequest(conn_id,UBS_BIT_PORT[5]);
		cgwCDAC20_RegistersRequest(conn_id,UBS_ADC_PORT);
	}
}

void adcStatusRequest(int handle) {
	cgwCDAC20_StatusRequest(conn_id,UBS_ADC_PORT);	
}

void adcFileWrite(int handle) {
	if (cgwConnectionBroken) return;
	
	WriteAdcFiles();
}

int revealData(char * input, int * year, int *month, int *day) {
	char inputCopy[256];
	strcpy(inputCopy,input);
	inputCopy[4] = ' ';
	inputCopy[7] = ' ';
	if (sscanf(inputCopy,"%d %d %d",year,month,day) == 3) return 1; else return 0;
}

void deleteOldFiles(int handle) {
	char dir[500];
	char logDir[500],dataDir[500];
	char fileName[256];
	char *pos;
	int year,month,day;
	int curDays;
	
	GetProjectDir(dir);
	sprintf(logDir,"%s\\%s",dir,FILE_LOG_DIRECTORY);
	sprintf(dataDir,"%s\\%s",dir,FILE_DATA_DIRECTORY);
	
	GetSystemDate(&month,&day,&year);
	curDays = (year-2000)*365 + month*30 + day;
	
	if (SetDir(logDir) == 0) {
		if ( GetFirstFile("*",1,0,0,0,0,0,fileName) == 0 ){
			do {
				if ( (pos = strstr(fileName,"ubsServerLog_")) != NULL ) {
					if (revealData(pos+strlen("ubsServerLog_"),&year,&month,&day)) {
						if ( (curDays - (year-2000)*365 - month*30 - day) > FILE_EXPIRATION ) {
							DeleteFile(fileName);		
						}
					}
				}
			} while (GetNextFile(fileName) == 0);
		}	
	}
	
	if (SetDir(dataDir) == 0) {
		if ( GetFirstFile("*",1,0,0,0,0,0,fileName) == 0 ){
			do {
				if ( (pos = strstr(fileName,"ubsServerAdc_")) != NULL ) {
					if (revealData(pos+strlen("ubsServerAdc_"),&year,&month,&day)) {
						if ( (curDays - (year-2000)*365 - month*30 - day) > FILE_EXPIRATION ) {
							DeleteFile(fileName);		
						}
					}
				}	
			} while (GetNextFile(fileName) == 0);
		}	
	}
}

void deviceXtimeout(int deviceNumber) {
	static char stime[30];
	int i;
	
	if (cgwConnectionBroken) return;
	if (conn_id < 0) return;
	
	TimeStamp(stime);
	msAddMsg(msGMS(),"%s [CANGW] Device [%d (0x%X)] \"%s\" - TIMEOUT",stime,deviceNumber,deviceNumber,deviceKit.name[deviceNumber]);	
	
	for (i=0; i<6; i++) {
		if (deviceNumber == UBS_BIT_PORT[i]) {
			//Config_Bits(deviceNumber);
			cgwCEDIO_A_AttributesRequest(conn_id,deviceNumber);
			return;
		}
	}
	if(deviceNumber == UBS_ADC_PORT) {
		//Config_ADC(UBS_ADC_PORT); 
		cgwCEDIO_A_AttributesRequest(conn_id,deviceNumber);
	}
		
}

void deviceBIT0timeout(int handle) {deviceXtimeout(UBS_BIT_PORT[0]);}
void deviceBIT1timeout(int handle) {deviceXtimeout(UBS_BIT_PORT[1]);} 
void deviceBIT2timeout(int handle) {deviceXtimeout(UBS_BIT_PORT[2]);} 
void deviceBIT3timeout(int handle) {deviceXtimeout(UBS_BIT_PORT[3]);} 
void deviceBIT4timeout(int handle) {deviceXtimeout(UBS_BIT_PORT[4]);} 
void deviceBIT5timeout(int handle) {deviceXtimeout(UBS_BIT_PORT[5]);} 
void deviceADCtimeout(int handle) {deviceXtimeout(UBS_ADC_PORT);} 

void prepareTimeSchedule(void) {
	// Depending on the options checked:
	// - CANGW periodic connection task
	// - CANGW Device ping
#ifndef NO_CAN
	reconnectionRequestId = addRecordToSchedule(1,1,CANGW_RECONNECTION_DELAY,serverReconnection);

#ifndef NO_CAN_EXCHANGE
	addRecordToSchedule(1, 1, UBS_DEVICE_PING_INTERVAL, deviceCedioRequest);
	addRecordToSchedule(1, 0, 30, adcStatusRequest); 
#endif
	
#endif
	
	addRecordToSchedule(1, 0, FILE_DATA_WRITE_INTERVAL, adcFileWrite);  
	addRecordToSchedule(1, 1, 60*60*24, deleteOldFiles);     
	
#ifndef NO_CAN
#ifndef NO_CAN_EXCHANGE
	deviceXtimeoutHandle[UBS_ADC_PORT]    = addRecordToSchedule(1, 0, UBS_DEVICE_RESPONSE_TIMEOUT, deviceADCtimeout); 
	deviceXtimeoutHandle[UBS_BIT_PORT[0]] = addRecordToSchedule(1, 0, UBS_DEVICE_RESPONSE_TIMEOUT, deviceBIT0timeout); 
	deviceXtimeoutHandle[UBS_BIT_PORT[1]] = addRecordToSchedule(1, 0, UBS_DEVICE_RESPONSE_TIMEOUT, deviceBIT1timeout); 
	deviceXtimeoutHandle[UBS_BIT_PORT[2]] = addRecordToSchedule(1, 0, UBS_DEVICE_RESPONSE_TIMEOUT, deviceBIT2timeout); 
	deviceXtimeoutHandle[UBS_BIT_PORT[3]] = addRecordToSchedule(1, 0, UBS_DEVICE_RESPONSE_TIMEOUT, deviceBIT3timeout); 
	deviceXtimeoutHandle[UBS_BIT_PORT[4]] = addRecordToSchedule(1, 0, UBS_DEVICE_RESPONSE_TIMEOUT, deviceBIT4timeout); 
	deviceXtimeoutHandle[UBS_BIT_PORT[5]] = addRecordToSchedule(1, 0, UBS_DEVICE_RESPONSE_TIMEOUT, deviceBIT5timeout); 
#endif
#endif
/**/
}

void hookCedioRequest(cangw_msg_t msg, void * attr) {
	////////////
	
	////////////
}

void hookUbsAnswers(cangw_msg_t msg, void * attr) {   // need to check sometimes ADC config
	int number,i;
	static char stime[30];
	
	number = cgwDevicess_NumberFromID(msg.id);
	if (number == UBS_ADC_PORT) {
		resetScheduleRecord(deviceXtimeoutHandle[number]);
		if (msg.data[0] == 0xFE) {
			if ( (msg.data[1] & 0x18) != 0x18) {
				TimeStamp(stime);
				msAddMsg(msGMS(),"%s [UBS] Error! Configuration of CEAD20 is wrong. Sending configuration request.",stime);
				cgwCEAD20_MultiChannelConfig(conn_id, number,0,19,CEAD20_TIME_20MS,			
											 CEAD20_MODE_CONTINUOUS | CEAD20_MODE_TAKEOUT,0); 
			}
		}
		return;
	}
	for (i=0; i<6; i++) {
		 if (number == UBS_BIT_PORT[i]) {
			 //printf("%d has incoming message\n", number);
			resetScheduleRecord(deviceXtimeoutHandle[number]);
			if (msg.data[0] == 0xFE) {
				if ( msg.data[2] != 0xFF || msg.data[3] != 0xFF) {
					TimeStamp(stime);
					msAddMsg(msGMS(),"%s [UBS] Error! Configuration of CEDIO20 is wrong. Sending configuration request.",stime);
					cgwCEDIO_A_Configurate(conn_id,number,0xFFFF); 		
				}
			}
			
			if (msg.data[0] == 0xFA) {
				// oraora
				bitBlockEvents = clear_OutOfDate_Data(bitBlockEvents,time(NULL),UBS_EVENTS_EXPIRATION*24*60*60);
				if (bitBlockEvents == NULL) bitBlockLastEvent = NULL;
				bitBlockLastEvent = addNewEvent(bitBlockLastEvent,i, msg.data[2] | (msg.data[5] << 8), msg.data[3] | (msg.data[6] << 8) ); 
				if (bitBlockEvents == NULL) bitBlockEvents = bitBlockLastEvent;
			}
			return;
		}
	}
}

void preparationFunc(CANGW_CONN_T con_id)
{
	int i;
	
	Config_ADC(UBS_ADC_PORT);
	
	for(i=0;i<6;i++)
	{
		Config_Bits(UBS_BIT_PORT[i]);
	}
}

int main() {
	int i;
	char buff[500],*bf,stime[40];
	/////////////////////////////////
	// Body of the program
	/////////////////////////////////
	
	if ( (i = InitServerConfig(configFile)) < 0 ) {
		switch(-i) {
			case 1: // null pointer on filename
				MessagePopup("Configuration error","File name isn't set");
				return 0;
			case 2:
				sprintf(buff,"Unable to open the file \"%s\".",configFile);
				MessagePopup("Configuration error",buff);
				return 0;
			case 3:
				MessagePopup("Configuration error","ADC configurations are incorrect.");
				return 0;
			case 4:
				MessagePopup("Configuration error","CanGw configurations are incorrect."); 
				return 0;
			case 5:
				MessagePopup("Configuration error","Configurations of the output directories are incorrect."); 
				return 0;
			case 6:
				MessagePopup("Configuration error","Configurations of the TCP server are incorrect.");
				return 0;
			case 7:
				MessagePopup("Configuration error","UBS configurations are incorrect.");
				return 0;
			case 8:
				MessagePopup("Configuration error","BIT configurations are incorrect."); 
				return 0;
		}
	}
	
	SetStdioWindowOptions (10000, 0, 0);
	
	msInitGlobalStack();  // Initialize the global message stack 
	msAddMsg(msGMS(),"------------\n[UBS Server -- NEW SESSION]\n------------");   
	
	prepareTimeSchedule();
	
	// event server
	eventServerHandle = RegisterTCPServer(TCP_EVENT_SERVER_PORT,tcpEventServerCallback,NULL);
	TimeStamp(stime);
	if (eventServerHandle < 0) {
		bf = GetTCPSystemErrorString();
		msAddMsg(msGMS(),"%s [SERVER] Unable ot run event server on the port %d: %s",stime,TCP_EVENT_SERVER_PORT,bf);	
	} else {
		msAddMsg(msGMS(),"%s [SERVER] The event server on the port %d has been successfully created.",stime,TCP_EVENT_SERVER_PORT);
	}
	/////////////////////
	
	tcpConnection_InitServerInterface(&tcpSI);
	tcpConnection_SetBackgroundFunction(&tcpSI,bgdFunc);
	tcpConnection_SetDataExchangeFunction(&tcpSI,dataExchFunc);
	
	SetStdioWindowVisibility(1);
	InstallMainCallback(userMainCallback,0,0);
	//--------------------------------------------------------
	cgwDevices_InitDevicesKit(&deviceKit);
	
	cgwCEAD20_RegisterDevice(&deviceKit,UBS_ADC_PORT,Config_ADC,hookUbsAnswers); // Add unique device ID
	for (i=0; i<6; i++) {
		 cgwCEDIO_A_RegisterDevice(&deviceKit,UBS_BIT_PORT[i],Config_Bits,hookUbsAnswers);
	}
	
		WriteLogFiles();
		msPrintMsgs(msGMS(),stdout);
		msFlushMsgs(msGMS());  	   
	//--------------------------------------------------------
	
	SetSleepPolicy(VAL_SLEEP_SOME);	
	tcpConnection_RunServer(TCP_PORT,&tcpSI); 
	
	WriteLogFiles();
	msPrintMsgs(msGMS(),stdout);
	msFlushMsgs(msGMS()); 
		
	DiscardAllResources();
	GetKey();

	return 0;
}

#define MAX_RECEIVED_BYTES 3000

void prepareTcpCommand(char *str,int bytes){
	int i;
	for (i=0; i<(bytes-1);i++) {
		if (str[i] == 0) str[i]=' ';
		  else str[i] = toupper(str[i]);
	}
	if (bytes == MAX_RECEIVED_BYTES) str[MAX_RECEIVED_BYTES-1] = 0;
}

void printAllEvents(void) {
	CedioEvent * buf;
	buf = bitBlockEvents;
	printf ("-------------- EVENTS --------------\n");
	while (buf != NULL) {
		printf("%u %u %X %X \n",buf->t,buf->blockN, buf->channelN, buf->changesN);
		buf = buf->next;
	}
	
	printf ("-------------- END OF EVENTS --------------\n");
}

void dataExchFunc(unsigned handle,void * arg)
{
	static char command[MAX_RECEIVED_BYTES*2] = "";
	static char subcommand[MAX_RECEIVED_BYTES*2];
	static char buf[MAX_RECEIVED_BYTES];
	static char buf2[MAX_RECEIVED_BYTES];
	static char * bufpos, *lfp;
	static int byteRecv,bufInt,bufInt2;
	static char stime[30];
	
	byteRecv = ServerTCPRead(handle,buf,MAX_RECEIVED_BYTES,0);
	if ( byteRecv <= 0 )
	{
		TimeStamp(stime);
		msAddMsg(msGMS(),"%s [SERVER CLIENT] Error occured while receiving messages from the client >> %s",stime,GetTCPSystemErrorString());
		return;
	}
	
	prepareTcpCommand(buf,byteRecv);
	strcpy(command,buf);
	
	//printf("%s\n",command);
	
	// Selecting all incoming commands
	while ( (lfp = strstr(command,"\n")) != NULL ) {
		lfp[0] = 0;
		strcpy(subcommand,command);
		strcpy(command,lfp+1);
		
		////////////////////////////////  subcommand decoding
		// UBS:ALLADC1
		bufpos = strstr(subcommand,"UBS:ALLADC1");
		if (bufpos)
				{
					getAllAdc(buf);
					sprintf(buf2,"UBS:ALLADC1 %s\n",buf);
					ServerTCPWrite(handle,buf2,strlen(buf2)+1,0);
					continue;
				} 
		// UBS:ALLADC2
		bufpos = strstr(subcommand,"UBS:ALLADC2");
		if (bufpos)
				{
					getAllAdc(buf);
					sprintf(buf2,"UBS:ALLADC2 %s\n",buf); 
					ServerTCPWrite(handle,buf2,strlen(buf2)+1,0);
					continue;
				} 
		// UBS:ALLBITS
		bufpos = strstr(subcommand,"UBS:ALLBITS");
		if (bufpos)
				{
					getAllBits(buf);
					sprintf(buf2,"UBS:ALLBITS %s\n",buf); 
					ServerTCPWrite(handle,buf2,strlen(buf2)+1,0);
					continue;
				}

		// UBS:ALLINFO
		bufpos = strstr(subcommand,"UBS:ALLINFO");
		if (bufpos)
				{
					cgwCEAD20_RegistersRequest(conn_id,23); 
					strcpy(buf,"");
						getAllAdc(buf2);
						strcat(buf,buf2);
						strcat(buf," ");
					
						getAllBits(buf2);
						strcat(buf,buf2);
						strcat(buf," ");
					
						getOutReg(buf2);
						strcat(buf,buf2);
					
					ServerTCPWrite(handle,buf,strlen(buf)+1,0);
					continue;
				}
		// UBS:GETOUTREG
		bufpos = strstr(subcommand,"UBS:GETOUTREG");
		if (bufpos)
				{
					getOutReg(buf);
					sprintf(buf2,"UBS:GETOUTREG %s\n",buf); 
					ServerTCPWrite(handle,buf2,strlen(buf2)+1,0);
					//cgwCEAD20_WriteToOutputRegister(conn_id,23,0x0);
					continue;
				}
		// UBS:SETOUTREG X
		bufpos = strstr(subcommand,"UBS:SETOUTREG");
		if (bufpos)
				{
					setOutReg(bufpos+strlen("UBS:SETOUTREG"));
					//cgwCEAD20_WriteToOutputRegister(conn_id,23,0x0);
					continue;
				}
		// UBS:BITNAME
		bufpos = strstr(subcommand,"UBS:BITNAME");
		if (bufpos)
				{
					if (sscanf(bufpos+strlen("UBS:BITNAME"),"%d %d",&bufInt,&bufInt2) == 2) {
						if (bufInt < 0 || bufInt >= 12) continue;  
						if (bufInt2 < 0 || bufInt2 >= 16) continue;
						strcpy(buf,UBS_BIT_NAMES[bufInt*16 + bufInt2]);
						ServerTCPWrite(handle,buf,strlen(buf)+1,0);
					}
					continue;
				}
		// UBS:ADCNAME1
		bufpos = strstr(subcommand,"UBS:ADCNAME1");
		if (bufpos)
				{
					if (sscanf(bufpos+strlen("UBS:ADCNAME1"),"%d",&bufInt) == 1) {
						if (bufInt < 0 || bufInt >= 20) continue;
						strcpy(buf,UBS_ADC_NAMES[bufInt]);
						ServerTCPWrite(handle,buf,strlen(buf)+1,0);
					}
					continue;
				}
		// UBS:ADCNAME2
		bufpos = strstr(subcommand,"UBS:ADCNAME2");
		if (bufpos)
				{
					if (sscanf(bufpos+strlen("UBS:ADCNAME2"),"%d",&bufInt) == 1) {
						if (bufInt < 0 || bufInt >= 20) continue;
						strcpy(buf,UBS_ADC_NAMES[20+bufInt]);
						ServerTCPWrite(handle,buf,strlen(buf)+1,0);
					}
					continue;
				}
		// EVENTS
		bufpos = strstr(subcommand,"EVENTS");
		if (bufpos)
				{
					printAllEvents();
					continue;
				}
		
	   ////////////////////////////////  end of subcommand decoding  
	}
	
	// It's likely that too long strings are not the commands
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void getAllAdc(char *target){
	char buf2[256];
	int i;
	
	strcpy(target,"");
	
	sprintf(buf2,"%lf",
		((cgwCEAD20_Information_t*)deviceKit.parameters[UBS_ADC_PORT])->ChannelVoltage[0]*UBS_ADC_COEFF[0][0]+UBS_ADC_COEFF[0][1]);
	strcat(target,buf2);
		for(i=1; i<20; i++)
		{
			sprintf(buf2," %.8lf",
				((cgwCEAD20_Information_t*)deviceKit.parameters[UBS_ADC_PORT])->ChannelVoltage[i]*UBS_ADC_COEFF[i][0]+UBS_ADC_COEFF[i][1]);
			strcat(target,buf2);
		}
}

void getAllBits(char *target){
	int i;
	char buf2[256];
	
	strcpy(target,"");
	
	sprintf(buf2,"%X",	
		((cgwCEDIO_A_Information_t*)deviceKit.parameters[UBS_BIT_PORT[0]])->InputRegisterData);
	strcat(target,buf2);
		for(i=1; i<6; i++)
		{
			sprintf(buf2," %X",
				((cgwCEDIO_A_Information_t*)deviceKit.parameters[UBS_BIT_PORT[i]])->InputRegisterData);
			strcat(target,buf2);
		}
}

void getAllBitsDec(char *target){
	int i;
	char buf2[256];
	
	strcpy(target,"");
	
	sprintf(buf2,"%d",	
		((cgwCEDIO_A_Information_t*)deviceKit.parameters[UBS_BIT_PORT[0]])->InputRegisterData);
	strcat(target,buf2);
		for(i=1; i<6; i++)
		{
			sprintf(buf2," %d",
				((cgwCEDIO_A_Information_t*)deviceKit.parameters[UBS_BIT_PORT[i]])->InputRegisterData);
			strcat(target,buf2);
		}
}

void getOutReg(char *target){
	sprintf(target,"%X",
		((cgwCEAD20_Information_t*)deviceKit.parameters[UBS_ADC_PORT])->OutputRegisterData);	
}

void getOutRegDec(char *target){
	sprintf(target,"%d",
		((cgwCEAD20_Information_t*)deviceKit.parameters[UBS_ADC_PORT])->OutputRegisterData);	
}

void setOutReg(char *src){
	unsigned int outreg;

	if(sscanf(src,"%X",&outreg) > 0){
		outreg = outreg & 0xFF;
		cgwCEAD20_WriteToOutputRegister(conn_id,UBS_ADC_PORT,outreg);  
		return;
	}
}

/////////////////////////////////////////////////////////////////////
//=================================================================== 
//===================================================================
//===================================================================
//===================================================================
///////////////////////////////////////////////////////////////////// 

int tcpEventServerCallback(unsigned handle, int xType, int errCode, void * callbackData)
{
	char buf[1000];
	unsigned int eventMessage[3000], *eMsgP;
	int clientNum,i,bytes;
	unsigned int eventNumber;
	static char stime[30];
	static unsigned int zero[1] = {0};
	
	GetTCPPeerAddr(handle,buf,256);
	switch(xType)
	{
		case TCP_CONNECT:
			for (i=0; i<5; i++) {
				if (eventClients[i] < 0) {
					eventClients[i] = handle;
					clientNum = i;
					TimeStamp(stime); 
					msAddMsg(msGMS(),"%s [CLIENT] An event viewer (%d) [IP: %s] has connected.",stime,clientNum,buf);
					break;
				}
			}
			// not connected
			if (i==5) DisconnectTCPClient(handle);
			break;
		case TCP_DISCONNECT:
			for (i=0; i<5; i++) {
				if (eventClients[i] == handle) {
					eventClients[i] = -1;
					clientNum = i;
					break;
				}
			}
			TimeStamp(stime);   
			msAddMsg(msGMS(),"%s [CLIENT] An event viewer (%d) [IP: %s] has disconnected.",stime,clientNum,buf);
			break;
		case TCP_DATAREADY:
			bytes = ServerTCPRead(handle,eventMessage,sizeof(eventMessage),20);
			if (bytes == sizeof(unsigned int)*2) {
				eMsgP = getEventData(bitBlockEvents,eventMessage[0],eventMessage[1],&eventNumber);
				if (eMsgP != NULL) {
					ServerTCPWrite(handle,eMsgP,eventNumber,100);
					free(eMsgP);	
				} else {
					ServerTCPWrite(handle,zero,sizeof(unsigned int)*1,100);	
				}
			}
			break;
	}
	return 0;
}
