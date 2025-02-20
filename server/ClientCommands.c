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

#include <ansi_c.h>
#include "ClientCommands.h"
#include "ModBusUbs.h"
#include "ServerConfigData.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// Static global variables

//==============================================================================
// Static functions

//==============================================================================
// Global variables

//==============================================================================
// Global functions

void PrepareAnswerForClient(const char * command, const modbus_block_data_t * modbusBlockData, char *answer) {
	static char dataBuffer[3000];
	int deviceIndex, channelIndex;
	time_t startTimeStamp, endTimeStamp;
	double doubleValue;
	
	// UBS_CMD_GET_ALLVALUES
	if (strstr(command, UBS_CMD_GET_ALLVALUES) == command) {
		FormatUbsProcessedData(&modbusBlockData->processed_data, dataBuffer);
		sprintf(answer, "%s %s\n", UBS_CMD_GET_ALLVALUES, dataBuffer);
		return;
	}
	
	// =======
	// DI & DQ
	// =======
	// UBS_CMD_GET_DI_VALUES
	if (strstr(command, UBS_CMD_GET_DI_VALUES) == command) {
		FormatAllDiData(modbusBlockData->processed_data.DI, dataBuffer);
		sprintf(answer, "%s %s\n", UBS_CMD_GET_DI_VALUES, dataBuffer);
		return;
	}
	
	// UBS_CMD_GET_DI_NAMES
	if (strstr(command, UBS_CMD_GET_DI_NAMES) == command) {
		FormatAllDiNames(dataBuffer);
		sprintf(answer, "%s %s\n", UBS_CMD_GET_DI_NAMES, dataBuffer);
		return;
	}
	
	// UBS_CMD_GET_DQ_VALUES
	if (strstr(command, UBS_CMD_GET_DQ_VALUES) == command) {
		FormatAllDqData(modbusBlockData->processed_data.DQ, dataBuffer);
		sprintf(answer, "%s %s\n", UBS_CMD_GET_DQ_VALUES, dataBuffer);
		return;
	}		  
	
	// UBS_CMD_GET_DQ_NAMES 
	if (strstr(command, UBS_CMD_GET_DQ_NAMES) == command) {
		FormatAllDqNames(dataBuffer);
		sprintf(answer, "%s %s\n", UBS_CMD_GET_DQ_NAMES, dataBuffer);
		return;
	}
	
	// ===
	// ADC
	// ===
	// UBS_CMD_GET_ADC_VALUES  
	if (strstr(command, UBS_CMD_GET_ADC_VALUES) == command) {
		if (sscanf(command + strlen(UBS_CMD_GET_ADC_VALUES), "%d", &deviceIndex) == 1) {
			if (deviceIndex < 0 || deviceIndex >= ADC_NUMBER) {
				sprintf(answer, "ERROR:[%s] ADC index is out of range [0, %d].\n", command, ADC_NUMBER - 1);		
			} else {
				FormatAdcData(&modbusBlockData->processed_data, deviceIndex, dataBuffer);
				sprintf(answer, "%s %s\n", UBS_CMD_GET_ADC_VALUES, dataBuffer);
			}
		} else {
			sprintf(answer, "ERROR:[%s] Unable to read ADC index.\n", command);	
		}

		return;
	}
	
	// UBS_CMD_GET_ADC_NAMES  
	if (strstr(command, UBS_CMD_GET_ADC_NAMES) == command) {
		if (sscanf(command + strlen(UBS_CMD_GET_ADC_NAMES), "%d", &deviceIndex) == 1) {
			if (deviceIndex < 0 || deviceIndex >= ADC_NUMBER) {
				sprintf(answer, "ERROR:[%s] ADC index is out of range [0, %d].\n", command, ADC_NUMBER - 1);		
			} else {
				FormatAdcNames(deviceIndex, dataBuffer);
				sprintf(answer, "%s %s\n", UBS_CMD_GET_ADC_NAMES, dataBuffer);
			}
		} else {
			sprintf(answer, "ERROR:[%s] Unable to read ADC index.\n", command);	
		}
		
		return;
	}
	
	// UBS_CMD_GET_ADC_COEFFICIENTS  
	if (strstr(command, UBS_CMD_GET_ADC_COEFFICIENTS) == command) {
		if (sscanf(command + strlen(UBS_CMD_GET_ADC_COEFFICIENTS), "%d", &deviceIndex) == 1) {
			if (deviceIndex < 0 || deviceIndex >= ADC_NUMBER) {
				sprintf(answer, "ERROR:[%s] ADC index is out of range [0, %d].\n", command, ADC_NUMBER - 1);		
			} else {
				FormatAdcCoefficients(deviceIndex, dataBuffer);
				sprintf(answer, "%s %s\n", UBS_CMD_GET_ADC_COEFFICIENTS, dataBuffer);
			}
		} else {
			sprintf(answer, "ERROR:[%s] Unable to read ADC index.\n", command);	
		}
		
		return;
	}
	
	// ===
	// DAC
	// ===
	// UBS_CMD_GET_DAC_VALUES
	if (strstr(command, UBS_CMD_GET_DAC_VALUES) == command) {
		if (sscanf(command + strlen(UBS_CMD_GET_DAC_VALUES), "%d", &deviceIndex) == 1) {
			if (deviceIndex < 0 || deviceIndex >= DAC_NUMBER) {
				sprintf(answer, "ERROR:[%s] DAC index is out of range [0, %d].\n", command, DAC_NUMBER - 1);		
			} else {
				FormatDacData(&modbusBlockData->processed_data, deviceIndex, dataBuffer);
				sprintf(answer, "%s %s\n", UBS_CMD_GET_DAC_VALUES, dataBuffer);
			}
		} else {
			sprintf(answer, "ERROR:[%s] Unable to read DAC index.\n", command);	
		}
		
		return;
	}
	
	// UBS_CMD_GET_DAC_NAMES
	if (strstr(command, UBS_CMD_GET_DAC_NAMES) == command) {
		if (sscanf(command + strlen(UBS_CMD_GET_DAC_NAMES), "%d", &deviceIndex) == 1) {
			if (deviceIndex < 0 || deviceIndex >= DAC_NUMBER) {
				sprintf(answer, "ERROR:[%s] DAC index is out of range [0, %d].\n", command, DAC_NUMBER - 1);		
			} else {
				FormatDacNames(deviceIndex, dataBuffer);
				sprintf(answer, "%s %s\n", UBS_CMD_GET_DAC_NAMES, dataBuffer);
			}
		} else {
			sprintf(answer, "ERROR:[%s] Unable to read DAC index.\n", command);	
		}
		
		return;
	}
	
	// UBS_CMD_SET_DAC_VALUE  
	if (strstr(command, UBS_CMD_SET_DAC_VALUE) == command) {
		if (sscanf(command + strlen(UBS_CMD_SET_DAC_VALUE), "%d %d %lf", &deviceIndex, &channelIndex, &doubleValue) == 3) {
			if (deviceIndex < 0 || deviceIndex >= DAC_NUMBER) {
				sprintf(answer, "ERROR:[%s] DAC index (%d) is out of range [0, %d].\n", command, deviceIndex, DAC_NUMBER - 1);	
				
			} else if (channelIndex < 0 || channelIndex >= CHANNELS_PER_DAC) {
				sprintf(answer, "ERROR:[%s] DAC channel index (%d) is out of range [0, %d].\n", command, channelIndex, CHANNELS_PER_DAC - 1);
				
			} else if (doubleValue < -10 || doubleValue > 10) {
				sprintf(answer, "ERROR:[%s] Voltage (%lf) is out of range [-10 V, 10 V].\n", command, doubleValue); 
				
			} else {
			
				if (writeUbsDAC(modbusBlockData->connectionInfo.conversationHandle, deviceIndex, channelIndex, doubleValue)) {
					sprintf(answer, "%s SUCCESS\n", UBS_CMD_SET_DAC_VALUE, dataBuffer); 	
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
	// UBS_CMD_GET_DIAGNOSTICS
	if (strstr(command, UBS_CMD_GET_DIAGNOSTICS) == command) {
		FormatDiagnosticsData(&modbusBlockData->processed_data, dataBuffer);
		sprintf(answer, "%s %s\n", UBS_CMD_GET_DIAGNOSTICS, dataBuffer);
		return;
	}
	
	// UBS_CMD_GET_CONNECTION_STATE
	if (strstr(command, UBS_CMD_GET_CONNECTION_STATE) == command) {
		sprintf(answer, "%s %d\n", UBS_CMD_GET_CONNECTION_STATE, modbusBlockData->connectionInfo.connected);
		return;
	}
	
	// UBS_CMD_GET_EVENTS
	if (strstr(command, UBS_CMD_GET_EVENTS) == command) {
		if (sscanf(command + strlen(UBS_CMD_GET_EVENTS), "%u %u", &startTimeStamp, &endTimeStamp) == 2) {
			if (FormatEventsData(startTimeStamp, endTimeStamp, dataBuffer)) {
				sprintf(answer, "%s %s\n", UBS_CMD_GET_EVENTS, dataBuffer);	
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




