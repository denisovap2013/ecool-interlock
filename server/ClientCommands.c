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
    // registerCommandParser(CMD_NAME_CONSTANT, CMD_NAME_ALIAS, cmdParserFunction);
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
	static char parserAnswer[1024];
	parserFunciton parser;
	int result;
	
	sscanf(userCmd, "%s%n", cmdName, &cursor);
	
	parser = getCommandparser(cmdName);
	
	answerBuffer[0] = 0;  // Empty string (no answer)
	parserAnswer[0] = 0;   
	
	if (parser) {
		result = parser(userCmd + cursor, parserAnswer, ip);
		
		// Check for errors
		if (result < 0) {
			logNotification(ip, "[ERROR] Incorrect command data: \"%s\"", userCmd);
			sprintf(answerBuffer, "!%s\n", userCmd); 
			return -1; // Error occurred
		}

		if (result == 0) return 0;  
		
		if (strlen(parserAnswer))
			sprintf(answerBuffer, "%s %s\n", cmdName, parserAnswer);

	} else {
		logNotification(ip, "[ERROR] Unknown command: \"%s\"", userCmd);
		sprintf(answerBuffer, "?%s\n", userCmd);	
	}
	
	return 1; // Command processed successfully (including "unknown" commands)
}

void logNotification(char *ip, char *message, ...) {

	char buf[512];
	va_list arglist;

	va_start(arglist, message);
	vsprintf(buf, message, arglist);
	va_end(arglist);

	msAddMsg(msGMS(),"%s [CLIENT] [IP: %s] %s", TimeStamp(0), ip, buf);

}

//////////////////////////////////
// PARSERS
//////////////////////////////////



void PrepareAnswerForClient(const char * command, const modbus_block_data_t * modbusBlockData, char *answer) {
	static char dataBuffer[3000];
	int deviceIndex, channelIndex;
	time_t startTimeStamp, endTimeStamp;
	double doubleValue;
	
	// CMD_A_GET_ALLVALUES
	if (strstr(command, CMD_A_GET_ALLVALUES) == command) {
		FormatUbsProcessedData(&modbusBlockData->processed_data, dataBuffer);
		sprintf(answer, "%s %s\n", CMD_A_GET_ALLVALUES, dataBuffer);
		return;
	}
	
	// =======
	// DI & DQ
	// =======
	// CMD_A_GET_DI_VALUES
	if (strstr(command, CMD_A_GET_DI_VALUES) == command) {
		FormatAllDiData(modbusBlockData->processed_data.DI, dataBuffer);
		sprintf(answer, "%s %s\n", CMD_A_GET_DI_VALUES, dataBuffer);
		return;
	}
	
	// CMD_A_GET_DI_NAMES
	if (strstr(command, CMD_A_GET_DI_NAMES) == command) {
		FormatAllDiNames(dataBuffer);
		sprintf(answer, "%s %s\n", CMD_A_GET_DI_NAMES, dataBuffer);
		return;
	}
	
	// CMD_A_GET_DQ_VALUES
	if (strstr(command, CMD_A_GET_DQ_VALUES) == command) {
		FormatAllDqData(modbusBlockData->processed_data.DQ, dataBuffer);
		sprintf(answer, "%s %s\n", CMD_A_GET_DQ_VALUES, dataBuffer);
		return;
	}		  
	
	// CMD_A_GET_DQ_NAMES 
	if (strstr(command, CMD_A_GET_DQ_NAMES) == command) {
		FormatAllDqNames(dataBuffer);
		sprintf(answer, "%s %s\n", CMD_A_GET_DQ_NAMES, dataBuffer);
		return;
	}
	
	// ===
	// ADC
	// ===
	// CMD_A_GET_ADC_VALUES  
	if (strstr(command, CMD_A_GET_ADC_VALUES) == command) {
		if (sscanf(command + strlen(CMD_A_GET_ADC_VALUES), "%d", &deviceIndex) == 1) {
			if (deviceIndex < 0 || deviceIndex >= ADC_NUMBER) {
				sprintf(answer, "ERROR:[%s] ADC index is out of range [0, %d].\n", command, ADC_NUMBER - 1);		
			} else {
				FormatAdcData(&modbusBlockData->processed_data, deviceIndex, dataBuffer);
				sprintf(answer, "%s %s\n", CMD_A_GET_ADC_VALUES, dataBuffer);
			}
		} else {
			sprintf(answer, "ERROR:[%s] Unable to read ADC index.\n", command);	
		}

		return;
	}
	
	// CMD_A_GET_ADC_NAMES  
	if (strstr(command, CMD_A_GET_ADC_NAMES) == command) {
		if (sscanf(command + strlen(CMD_A_GET_ADC_NAMES), "%d", &deviceIndex) == 1) {
			if (deviceIndex < 0 || deviceIndex >= ADC_NUMBER) {
				sprintf(answer, "ERROR:[%s] ADC index is out of range [0, %d].\n", command, ADC_NUMBER - 1);		
			} else {
				FormatAdcNames(deviceIndex, dataBuffer);
				sprintf(answer, "%s %s\n", CMD_A_GET_ADC_NAMES, dataBuffer);
			}
		} else {
			sprintf(answer, "ERROR:[%s] Unable to read ADC index.\n", command);	
		}
		
		return;
	}
	
	// CMD_A_GET_ADC_COEFFICIENTS  
	if (strstr(command, CMD_A_GET_ADC_COEFFICIENTS) == command) {
		if (sscanf(command + strlen(CMD_A_GET_ADC_COEFFICIENTS), "%d", &deviceIndex) == 1) {
			if (deviceIndex < 0 || deviceIndex >= ADC_NUMBER) {
				sprintf(answer, "ERROR:[%s] ADC index is out of range [0, %d].\n", command, ADC_NUMBER - 1);		
			} else {
				FormatAdcCoefficients(deviceIndex, dataBuffer);
				sprintf(answer, "%s %s\n", CMD_A_GET_ADC_COEFFICIENTS, dataBuffer);
			}
		} else {
			sprintf(answer, "ERROR:[%s] Unable to read ADC index.\n", command);	
		}
		
		return;
	}
	
	// ===
	// DAC
	// ===
	// CMD_A_GET_DAC_VALUES
	if (strstr(command, CMD_A_GET_DAC_VALUES) == command) {
		if (sscanf(command + strlen(CMD_A_GET_DAC_VALUES), "%d", &deviceIndex) == 1) {
			if (deviceIndex < 0 || deviceIndex >= DAC_NUMBER) {
				sprintf(answer, "ERROR:[%s] DAC index is out of range [0, %d].\n", command, DAC_NUMBER - 1);		
			} else {
				FormatDacData(&modbusBlockData->processed_data, deviceIndex, dataBuffer);
				sprintf(answer, "%s %s\n", CMD_A_GET_DAC_VALUES, dataBuffer);
			}
		} else {
			sprintf(answer, "ERROR:[%s] Unable to read DAC index.\n", command);	
		}
		
		return;
	}
	
	// CMD_A_GET_DAC_NAMES
	if (strstr(command, CMD_A_GET_DAC_NAMES) == command) {
		if (sscanf(command + strlen(CMD_A_GET_DAC_NAMES), "%d", &deviceIndex) == 1) {
			if (deviceIndex < 0 || deviceIndex >= DAC_NUMBER) {
				sprintf(answer, "ERROR:[%s] DAC index is out of range [0, %d].\n", command, DAC_NUMBER - 1);		
			} else {
				FormatDacNames(deviceIndex, dataBuffer);
				sprintf(answer, "%s %s\n", CMD_A_GET_DAC_NAMES, dataBuffer);
			}
		} else {
			sprintf(answer, "ERROR:[%s] Unable to read DAC index.\n", command);	
		}
		
		return;
	}
	
	// CMD_A_SET_DAC_VALUE  
	if (strstr(command, CMD_A_SET_DAC_VALUE) == command) {
		if (sscanf(command + strlen(CMD_A_SET_DAC_VALUE), "%d %d %lf", &deviceIndex, &channelIndex, &doubleValue) == 3) {
			if (deviceIndex < 0 || deviceIndex >= DAC_NUMBER) {
				sprintf(answer, "ERROR:[%s] DAC index (%d) is out of range [0, %d].\n", command, deviceIndex, DAC_NUMBER - 1);	
				
			} else if (channelIndex < 0 || channelIndex >= CHANNELS_PER_DAC) {
				sprintf(answer, "ERROR:[%s] DAC channel index (%d) is out of range [0, %d].\n", command, channelIndex, CHANNELS_PER_DAC - 1);
				
			} else if (doubleValue < -10 || doubleValue > 10) {
				sprintf(answer, "ERROR:[%s] Voltage (%lf) is out of range [-10 V, 10 V].\n", command, doubleValue); 
				
			} else {
			
				if (writeUbsDAC(modbusBlockData->connectionInfo.conversationHandle, deviceIndex, channelIndex, doubleValue)) {
					sprintf(answer, "%s SUCCESS\n", CMD_A_SET_DAC_VALUE, dataBuffer); 	
				} else {
					sprintf(answer, "ERROR:[%s] Unable to send a command to the UBS block.\n", command);		
				}
				
			}
		} else {
			sprintf(answer, "ERROR:[%s] Unable to set the DAC channel voltage. Expected 'deviceIndex' 'channelIndex' 'voltage (V)'.\n", command);	
		}
		
		return;
	} 
	
	// ===========
	// Diagnostics
	// ===========
	// CMD_A_GET_DIAGNOSTICS
	if (strstr(command, CMD_A_GET_DIAGNOSTICS) == command) {
		FormatDiagnosticsData(&modbusBlockData->processed_data, dataBuffer);
		sprintf(answer, "%s %s\n", CMD_A_GET_DIAGNOSTICS, dataBuffer);
		return;
	}
	
	// CMD_A_GET_CONNECTION_STATE
	if (strstr(command, CMD_A_GET_CONNECTION_STATE) == command) {
		sprintf(answer, "%s %d\n", CMD_A_GET_CONNECTION_STATE, modbusBlockData->connectionInfo.connected);
		return;
	}
	
	// CMD_A_GET_EVENTS
	if (strstr(command, CMD_A_GET_EVENTS) == command) {
		if (sscanf(command + strlen(CMD_A_GET_EVENTS), "%u %u", &startTimeStamp, &endTimeStamp) == 2) {
			if (FormatEventsData(startTimeStamp, endTimeStamp, dataBuffer)) {
				sprintf(answer, "%s %s\n", CMD_A_GET_EVENTS, dataBuffer);	
			} else {
				sprintf(answer, "ERROR:[%s] Unable to read the events data. See server logs.\n", command);	
			}
			  
		} else {
			sprintf(answer, "ERROR:[%s] Unable to get the timestamps from the request. Expected 'startTimeStamp (s)' 'endTimeStamp (s)'.\n", command);	
		}
		return;
	}
	
	// Unknown command
	sprintf(answer, "ERROR:[%s] Unknown command.\n", command);
	return;
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




