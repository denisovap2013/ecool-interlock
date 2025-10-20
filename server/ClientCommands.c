//==============================================================================
//
// Title:       ClientCommands.c
// Purpose:     A short description of the implementation.
//
// Created on:  21.12.2020 at 10:11:35 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include <userint.h>
#include <ansi_c.h>
#include <tcpsupp.h> 

#include "MessageStack.h"
#include "TimeMarkers.h"
#include "ClientCommands.h"
#include "ModBusUbs.h"
#include "ServerConfigData.h"
#include "hash_map.h"
#include "ServerData.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// Static global variables

//==============================================================================
// Static functions
int processUserCommand(char *userCmd, char *answerBuffer, char *ip);
void toupperCase(char *text);
void logNotification(char *ip, char *message, ...);

//==============================================================================
// Global variables
parserFunciton RegisteredParsers[MAX_PARSERS_NUM];

int registeredParsersNum = 0;
int parsersInitialized = 0;
map_int_t commandsHashMap;

#define MAX_RECEIVED_BYTES 3000

//==============================================================================
// Global functions

void InitCommandParsers(void) {
	if (parsersInitialized) return;

	map_init(&commandsHashMap);
	registeredParsersNum = 0;
	parsersInitialized = 1;	
	
	// Registering commands parsers
	registerCommandParser(CMD_P_GET_ALLVALUES, CMD_A_GET_ALLVALUES, cmdParserGetAllValues);
	registerCommandParser(CMD_P_GET_DI_VALUES, CMD_A_GET_DI_VALUES, cmdParserGetDiValues);
	registerCommandParser(CMD_P_GET_DI_NAMES, CMD_A_GET_DI_NAMES, cmdParserGetDiNames);
	registerCommandParser(CMD_P_GET_DQ_VALUES, CMD_A_GET_DQ_VALUES, cmdParserGetDqValues);
	registerCommandParser(CMD_P_GET_DQ_NAMES, CMD_A_GET_DQ_NAMES, cmdParserGetDqNames);
	registerCommandParser(CMD_P_GET_DIAGNOSTICS, CMD_A_GET_DIAGNOSTICS, cmdParserGetDiagnostics);
	registerCommandParser(CMD_P_GET_ADC_VALUES, CMD_A_GET_ADC_VALUES, cmdParserGetAdcValues);
	registerCommandParser(CMD_P_GET_ADC_NAMES, CMD_A_GET_ADC_NAMES, cmdParserGetAdcNames);
	registerCommandParser(CMD_P_GET_ADC_COEFFICIENTS, CMD_A_GET_ADC_COEFFICIENTS, cmdParserGetAdcCoefficients);
	registerCommandParser(CMD_P_SET_DAC_VALUE, CMD_A_SET_DAC_VALUE, cmdParserSetDacValue);
	registerCommandParser(CMD_P_GET_DAC_VALUES, CMD_A_GET_DAC_VALUES, cmdParserGetDacValues);
	registerCommandParser(CMD_P_GET_DAC_NAMES, CMD_A_GET_DAC_NAMES, cmdParserGetDacNames);
	registerCommandParser(CMD_P_GET_CONNECTION_STATE, CMD_A_GET_CONNECTION_STATE, cmdParserGetConnectinState);
	registerCommandParser(CMD_P_GET_EVENTS, CMD_A_GET_EVENTS, cmdParserGetEvents);
	
	registerCommandParser(CMD_P_GET_EVENTS_NUM, 0, cmdParserGetEventsNum);
	registerCommandParser(CMD_P_GET_EVENT_BY_IDX, 0, cmdParserGetEventByIdx);
	registerCommandParser(CMD_P_CLEAR_EVENTS_BUFFER, 0, cmdParserClearEventsBuffer);
}

void ReleaseCommandParsers(void) {
	if (!parsersInitialized) return;

	map_deinit(&commandsHashMap);
	registeredParsersNum = 0;
	parsersInitialized = 0;	
}

void registerCommandParser(char *command, char *alias, parserFunciton parser) {
	static char msg[256];
	if (registeredParsersNum >= MAX_PARSERS_NUM) { 
		sprintf(msg, "Unable to register a command '%s'. Maximum number of parsers (%d) exceeded.", command, MAX_PARSERS_NUM);
		MessagePopup("Internal application error.", msg);
		exit(0);
	}
	
	// Add command parser to the list and add mapping from the command name/alias
	// to the index of the added element in the list.
	RegisteredParsers[registeredParsersNum] = parser;
	map_set(&commandsHashMap, command, registeredParsersNum);
	if (alias)  map_set(&commandsHashMap, alias, registeredParsersNum);
	
	registeredParsersNum++;
}

void prepareTcpCommand(char *str, int bytes){
	int i, j;
	
	j = 0;
	for (i=0; i < bytes; i++) {
		if (str[i] != 0) str[j++] = str[i];
	}

	if (bytes == MAX_RECEIVED_BYTES) str[MAX_RECEIVED_BYTES-1] = 0;
	else str[j] = 0;
}

void toupperCase(char *text) {
	char *p;
	p = text;
	while (*p != 0) {
		*p = toupper(*p);
		p++;
	}
}

void dataExchFunc(unsigned handle, char *ip)
{
	static char command[MAX_RECEIVED_BYTES * 2] = "";
	static char subcommand[MAX_RECEIVED_BYTES * 2];
	static char buf[MAX_RECEIVED_BYTES];
	static char *lfp;
	static int byteRecv;
	static char answerBuffer[1024];
	static int checkInitialization = 1;

	// Check that all necessary initializations are done
	if (checkInitialization) {
		if (!parsersInitialized) {
			MessagePopup("Runtime Error", "Command parsers are not initialized");
			exit(0);
		}
		checkInitialization = 0;	
	}
	
	byteRecv = ServerTCPRead(handle, buf, MAX_RECEIVED_BYTES, 0);
	if ( byteRecv <= 0 )
	{
		logMessage("[SERVER CLIENT] Error occured while receiving messages from the client >> %s", GetTCPSystemErrorString()); 
		return;
	}
	
	prepareTcpCommand(buf, byteRecv);
	strcpy(command, buf);
	
	//printf("%s\n",command);
	
	// Selecting all incoming commands
	while ( (lfp = strchr(command, '\n')) != NULL ) {
		lfp[0] = 0;
		strcpy(subcommand, command);
		strcpy(command, lfp+1);
		processUserCommand(subcommand, answerBuffer, ip);
		if (answerBuffer[0] != 0) ServerTCPWrite(handle, answerBuffer, strlen(answerBuffer) + 1, 0);
	}
	
	// It's likely that extremly long strings are not commands
}

parserFunciton getCommandparser(char *command) {
	int *cmdIndex;
	char * symbol;
	
	symbol = command;
	while (*symbol != 0) {
	    *symbol = toupper(*symbol);
		symbol++;
	}
	cmdIndex = map_get(&commandsHashMap, command);
	if (cmdIndex) return RegisteredParsers[*cmdIndex];
	
	return 0;
}

int processUserCommand(char *userCmd, char *answerBuffer, char *ip) {
	char cmdName[256];
	int cursor;
	static char parserAnswer[4096];
	parserFunciton parser;
	int result;
	
	sscanf(userCmd, "%s%n", cmdName, &cursor);
	
	parser = getCommandparser(cmdName);
	
	answerBuffer[0] = 0;  // Empty string (no answer)
	parserAnswer[0] = 0;   
	
	if (parser) {
		result = parser(userCmd + cursor, parserAnswer, ip);
		
		#ifdef __VERBOSE__
		    logNotification(ip, "[VERBOSE] Processing a command: \"%s\".", userCmd);    
		#endif
		
		// Check for errors
		if (result < 0) {
			logNotification(ip, "[ERROR] Incorrect command data: \"%s\". %s", userCmd, parserAnswer);
			sprintf(answerBuffer, "ERROR: [%s] %s\n", userCmd, parserAnswer); 
			return -1; // Error occurred
		}

		if (result == 0) return 0;  
		
		if (strlen(parserAnswer))
			sprintf(answerBuffer, "%s %s\n", cmdName, parserAnswer);

	} else {
		logNotification(ip, "[ERROR] Unknown command: \"%s\"", userCmd);
		sprintf(answerBuffer, "ERROR: [%s] Unknown command.\n", userCmd);	
	}
	
	return 1; // Command processed successfully (including "unknown" commands)
}

void logNotification(char *ip, char *message, ...) {

	char buf[512];
	va_list arglist;

	va_start(arglist, message);
	vsprintf(buf, message, arglist);
	va_end(arglist);

	if (ip != NULL) {
		msAddMsg(msGMS(),"%s [CLIENT] [IP: %s] %s", TimeStamp(0), ip, buf);
	} else {
		msAddMsg(msGMS(),"%s [CLIENT] [IP: unspecified] %s", TimeStamp(0), buf);  	
	}

}

//////////////////////////////////
// PARSERS
//////////////////////////////////

int cmdParserGetAllValues(char *commandBody, char *answerBuffer, char *ip) {
	FormatUbsProcessedData(&modbusBlockData.processed_data, answerBuffer);
	return 1;
}

int cmdParserGetDiValues(char *commandBody, char *answerBuffer, char *ip) {
	FormatAllDiData(modbusBlockData.processed_data.DI, answerBuffer);
	return 1;
}

int cmdParserGetDiNames(char *commandBody, char *answerBuffer, char *ip) {
	FormatAllDiNames(answerBuffer);
	return 1;
}

int cmdParserGetDqValues(char *commandBody, char *answerBuffer, char *ip) {
	FormatAllDqData(modbusBlockData.processed_data.DQ, answerBuffer);
	return 1;
}

int cmdParserGetDqNames(char *commandBody, char *answerBuffer, char *ip) {
	FormatAllDqNames(answerBuffer);
	return 1;
}

int cmdParserGetDiagnostics(char *commandBody, char *answerBuffer, char *ip) {
	FormatDiagnosticsData(&modbusBlockData.processed_data, answerBuffer);
	return 1;
}

int cmdParserGetAdcValues(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex;

	if (sscanf(commandBody, "%d", &deviceIndex) != 1) {
		strcpy(answerBuffer, "Unable to read ADC index.");
		return -1;
	}

	if (deviceIndex < 0 || deviceIndex >= ADC_NUMBER) {
		sprintf(answerBuffer, "ADC index is out of range [0, %d].", ADC_NUMBER - 1);	
		return -1;	
	}

	FormatAdcData(&modbusBlockData.processed_data, deviceIndex, answerBuffer);
	return 1;
}

int cmdParserGetAdcNames(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex;

	if (sscanf(commandBody, "%d", &deviceIndex) != 1) {
		strcpy(answerBuffer, "Unable to read ADC index.");
		return -1;
	}

	if (deviceIndex < 0 || deviceIndex >= ADC_NUMBER) {
		sprintf(answerBuffer, "ADC index is out of range [0, %d].", ADC_NUMBER - 1);
		return -1;	
	}

	FormatAdcNames(deviceIndex, answerBuffer);
	return 1;
}

int cmdParserGetAdcCoefficients(char *commandBody, char *answerBuffer, char *ip) {
    int deviceIndex;

    if (sscanf(commandBody, "%d", &deviceIndex) != 1) {
    	sprintf(answerBuffer, "Unable to read ADC index.");
    	return -1;
    }

	if (deviceIndex < 0 || deviceIndex >= ADC_NUMBER) {
		sprintf(answerBuffer, "ADC index is out of range [0, %d].", ADC_NUMBER - 1);
		return -1;		
	}
	FormatAdcCoefficients(deviceIndex, answerBuffer);
	return 1;
}

int cmdParserSetDacValue(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, channelIndex;
	double doubleValue;

	if (sscanf(commandBody, "%d %d %lf", &deviceIndex, &channelIndex, &doubleValue) != 3) {
		sprintf(answerBuffer, "Unable to set the DAC channel voltage. Expected parameters 'deviceIndex' 'channelIndex' 'voltage (V)'.");	
		return -1;
	}

	if (deviceIndex < 0 || deviceIndex >= DAC_NUMBER) {
		sprintf(answerBuffer, "DAC index (%d) is out of range [0, %d].", deviceIndex, DAC_NUMBER - 1);
		return -1;		
	}

	if (channelIndex < 0 || channelIndex >= CHANNELS_PER_DAC) {
		sprintf(answerBuffer, "DAC channel index (%d) is out of range [0, %d].", channelIndex, CHANNELS_PER_DAC - 1);
		return -1;
	}

	if (doubleValue < -10 || doubleValue > 10) {
		sprintf(answerBuffer, "Voltage (%lf) is out of range [-10 V, 10 V].", doubleValue); 
		return -1;
	}
	
	if (writeUbsDAC(modbusBlockData.connectionInfo.conversationHandle, deviceIndex, channelIndex, doubleValue)) {
		strcpy(answerBuffer, "SUCCESS");
		return 1;
	}
	
	sprintf(answerBuffer, "Unable to send a command to the UBS block.");		
	return -1;
}

int cmdParserGetDacValues(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex;

	if (sscanf(commandBody, "%d", &deviceIndex) != 1) {
		strcpy(answerBuffer, "Unable to read DAC index.");
		return -1;
	}

	if (deviceIndex < 0 || deviceIndex >= DAC_NUMBER) {
		sprintf(answerBuffer, "DAC index is out of range [0, %d].", DAC_NUMBER - 1);	
		return -1;	
	}

	FormatDacData(&modbusBlockData.processed_data, deviceIndex, answerBuffer);
	return 1;
}

int cmdParserGetDacNames(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex;

	if (sscanf(commandBody, "%d", &deviceIndex) != 1) {
		strcpy(answerBuffer, "Unable to read ADC index.");
		return -1;
	}

	if (deviceIndex < 0 || deviceIndex >= DAC_NUMBER) {
		sprintf(answerBuffer, "DAC index is out of range [0, %d].", DAC_NUMBER - 1);
		return -1;	
	}

	FormatDacNames(deviceIndex, answerBuffer);
	return 1;
}

int cmdParserGetConnectinState(char *commandBody, char *answerBuffer, char *ip) {
	sprintf(answerBuffer, "%d", modbusBlockData.connectionInfo.connected);
	return 1;
}

int cmdParserGetEvents(char *commandBody, char *answerBuffer, char *ip) {
	time_t startTimeStamp, endTimeStamp;

	if (sscanf(commandBody, "%u %u", &startTimeStamp, &endTimeStamp) != 2) {
		sprintf(answerBuffer, "Unable to get the timestamps from the request. Expected 'startTimeStamp (s)' 'endTimeStamp (s)'.");
		return -1;
	}

	if (FormatEventsData(startTimeStamp, endTimeStamp, answerBuffer)) return 1;	

	sprintf(answerBuffer, "Unable to read the events data. See server logs.");	
	return -1;
}


int cmdParserGetEventsNum(char *commandBody, char *answerBuffer, char *ip) {
    return 1;	
}


int cmdParserGetEventByIdx(char *commandBody, char *answerBuffer, char *ip) {
	return 1;
}


int cmdParserClearEventsBuffer(char *commandBody, char *answerBuffer, char *ip) {
	return 1;
}


void FormatDiNames(unsigned int diIndex, char *outputBuffer) {
	int i;
	
	outputBuffer[0] = 0;

	for (i=0; i<32; i++) {
		if (i > 0) strcat(outputBuffer, "|");
		strcat(outputBuffer, CFG_UBS_DI_NAMES[diIndex][i]);
	}
}


void FormatAllDiNames(char *outputBuffer) {
	int i;
	
	outputBuffer[0] = 0;

	for (i=0; i<DI_NUMBER; i++) {
		if (i > 0) strcat(outputBuffer, "||");
		FormatDiNames(i, outputBuffer + strlen(outputBuffer));
	}
}


void FormatDqNames(unsigned int dqIndex, char *outputBuffer) {
	int i;
	
	outputBuffer[0] = 0;

	for (i=0; i<16; i++) {
		if (i > 0) strcat(outputBuffer, "|");
		strcat(outputBuffer, CFG_UBS_DQ_NAMES[dqIndex][i]);
	}
}


void FormatAllDqNames(char *outputBuffer) {
	int i;
	
	outputBuffer[0] = 0;

	for (i=0; i<DQ_NUMBER; i++) {
		if (i > 0) strcat(outputBuffer, "||");
		FormatDqNames(i, outputBuffer + strlen(outputBuffer));
	}
}


void FormatAdcNames(unsigned int adcIndex, char *outputBuffer) {
	int i;
	
	outputBuffer[0] = 0;

	for (i=0; i<CHANNELS_PER_ADC; i++) {
		if (i > 0) strcat(outputBuffer, "|");
		strcat(outputBuffer, CFG_UBS_ADC_NAMES[adcIndex][i]);
	}
}


void FormatAdcCoefficients(unsigned int adcIndex, char *outputBuffer) {
	int i;
	
	outputBuffer[0] = 0;

	for (i=0; i<CHANNELS_PER_ADC; i++) {
		if (i > 0) strcat(outputBuffer, " ");
		sprintf(outputBuffer + strlen(outputBuffer), "%f %f", CFG_UBS_ADC_COEFF[adcIndex][i][0], CFG_UBS_ADC_COEFF[adcIndex][i][1]);
	}
}


void FormatDacNames(unsigned int dacIndex, char *outputBuffer) {
	int i;
	
	outputBuffer[0] = 0;

	for (i=0; i<CHANNELS_PER_DAC; i++) {
		if (i > 0) strcat(outputBuffer, "|");
		strcat(outputBuffer, CFG_UBS_DAC_NAMES[dacIndex][i]);
	}
}
