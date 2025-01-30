//==============================================================================
//
// Title:       ModBusUbs.c
// Purpose:     A short description of the implementation.
//
// Created on:  10.12.2020 at 12:42:57 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include "ModBusUbs.h"
#include <tcpsupp.h>

#include "MessageStack.h"
#include "TimeMarkers.h"
#include "ServerConfigData.h"	 
#include "Logging.h"

//==============================================================================
// Constants
#define DEFAULT_DAC_RANGE_TYPE 9
#define MAX_LOG_PAGES_NUM 100

const unsigned short TRANSACTION_CODE_UBS_READ_ALL = 0xAAF0;
const unsigned short TRANSACTION_CODE_UBS_SET_DAC = 0xCC3C;
const unsigned short TRANSACTION_CODE_UBS_READ_LOG_STATE = 0x0CC0;
const unsigned short TRANSACTION_CODE_UBS_READ_LOG_PAGES = 0xC0C0;
const unsigned short TRANSACTION_CODE_UBS_RESET_LOG = 0xFCFC; 

//==============================================================================
// Types

//==============================================================================
// Static global variables
static const unsigned int ADC_RANGES_mV[10] = {0, 50, 80, 250, 500, 1000, 0, 2500, 5000, 10000};
//==============================================================================
// Static functions
static char transactionName[256];

const char * getTransactionName(int transactionID) {
	if (transactionID == TRANSACTION_CODE_UBS_READ_ALL)
		strcpy(transactionName, "TRANSACTION_CODE_UBS_READ_ALL");

	else if (transactionID == TRANSACTION_CODE_UBS_SET_DAC)
		strcpy(transactionName, "TRANSACTION_CODE_UBS_SET_DAC");

	else if (transactionID == TRANSACTION_CODE_UBS_READ_LOG_STATE)
		strcpy(transactionName, "TRANSACTION_CODE_UBS_READ_LOG_STATE");

	else if (transactionID == TRANSACTION_CODE_UBS_READ_LOG_PAGES)
		strcpy(transactionName, "TRANSACTION_CODE_UBS_READ_LOG_PAGES");

	else if (transactionID == TRANSACTION_CODE_UBS_RESET_LOG)
		strcpy(transactionName, "TRANSACTION_CODE_UBS_RESET_LOG");
		
	else
		strcpy(transactionName, "UNKNOWN_TRANSACTION");

	return transactionName;
}


void formatLogTimeInfo(time_info_t timeInfo, char * buffer) {
	sprintf(
		buffer,
		"%04d %02d %02d %02d %02d %02d %4d",
		timeInfo.year,
		timeInfo.month,
		timeInfo.day,
		timeInfo.hours,
		timeInfo.minutes,
		timeInfo.seconds,
		timeInfo.milliseconds
	);	
}


int compareTwoInts(int firstNumber, int secondNumber) {
	if (firstNumber > secondNumber) return  1;
    if (firstNumber < secondNumber) return -1;
    return 0;	
}


int compareEventsTime(const void * elem1, const void * elem2) 
{
	time_info_t *t1, *t2;
	int res;
	
	t1 = &((ubs_log_page_t *)elem1)->ubsBlockTime;
	t2 = &((ubs_log_page_t *)elem2)->ubsBlockTime;
	
	if ((res = compareTwoInts(t1->year, t2->year)) != 0) return res;
	if ((res = compareTwoInts(t1->month, t2->month)) != 0) return res; 
	if ((res = compareTwoInts(t1->day, t2->day)) != 0) return res; 
	if ((res = compareTwoInts(t1->hours, t2->hours)) != 0) return res; 
	if ((res = compareTwoInts(t1->minutes, t2->minutes)) != 0) return res; 
	if ((res = compareTwoInts(t1->seconds, t2->seconds)) != 0) return res; 
	if ((res = compareTwoInts(t1->milliseconds, t2->milliseconds)) != 0) return res; 

    return 0;
}
//==============================================================================
// Global variables

//==============================================================================
// Global functions
unsigned short getBigEndianWord(unsigned short value) {
	unsigned char bytes[2];

	bytes[0] = (value >> 8) & 0xFF;
	bytes[1] = value & 0xFF;
	
	return *(unsigned short*)bytes;
}


unsigned int getBigEndianInt(unsigned int value) {
	unsigned char bytes[4];

	bytes[0] = (value >> 24) & 0xFF;
	bytes[1] = (value >> 16) & 0xFF;
	bytes[2] = (value >> 8) & 0xFF;
	bytes[3] = value & 0xFF;
	
	return *(unsigned int*)bytes; 
}


unsigned short getWordFromBigEndian(unsigned short value) {
	unsigned char bytes[2];
	
	memcpy(bytes, &value, 2);
	return ((unsigned short)bytes[0] << 8) | (unsigned short)bytes[1];
}


unsigned int getIntFromBigEndian(unsigned int value) {
	unsigned char bytes[4];
	
	memcpy(bytes, &value, 4);
	return ((unsigned)bytes[0] << 24) | ((unsigned)bytes[1] << 16) | ((unsigned)bytes[2] << 8) | (unsigned)bytes[3];
}


void ConnectToUbsBlock(char * ip,unsigned int port, unsigned int timeout, void *callbackData) {
	int res;
	modbus_block_data_t *pModbusBlockData;
	
	pModbusBlockData = (modbus_block_data_t *)callbackData;
	logMessage("[MODBUS-UBS] Connecting to the UBS block (%s:%d)...", ip, port);
	res = ConnectToTCPServer(&pModbusBlockData->connectionInfo.conversationHandle, port, ip, ModbusUbsClientCallback, callbackData, timeout);
	
	if (res)
	{
		pModbusBlockData->connectionInfo.connected = 0;
		logMessage("[MODBUS-UBS] Error! Connection to the UBS block failed.");
	}
	else {
		pModbusBlockData->connectionInfo.connected = 1;
		logMessage("[MODBUS-UBS] Connection to the UBS block is established.");
	}
	
	return;
}


void SetDisconnectedStatus(modbus_connection_info_t * modbusConnectionInfo) {
	if (!modbusConnectionInfo->connected) return;

	modbusConnectionInfo->connected = 0;
	modbusConnectionInfo->conversationHandle = 0;
	logMessage("[MODBUS-UBS]: Error! Disconnected from the UBS block.");
	logMessage("[MODBUS-UBS] Next connection request will be within %d seconds.", UBS_RECONNECTION_DELAY);
}


void ResetLogReadingStatus(ubs_log_info_t * ubsLogInfo) {
	ubsLogInfo->currentlyReading = 0;	
	ubsLogInfo->currentPageIndex = 0;
	ubsLogInfo->numberOfPagesToRead = 0;
}


int SetLogReadingStatus(ubs_log_info_t * ubsLogInfo) {
	if (ubsLogInfo->currentlyReading) {
		logMessage("[CODE] Trying to initiate log pages reading for a second time.");
		return 0;
	}

	if (!ubsLogInfo->logState.startAddress && !ubsLogInfo->logState.endAddress) return 0;
	
	ubsLogInfo->currentlyReading = 1;	
	ubsLogInfo->currentPageIndex = 0;
	
	if (ubsLogInfo->logState.startAddress) {
		ubsLogInfo->numberOfPagesToRead = MAX_LOG_PAGES_NUM;	
	} else {
		ubsLogInfo->numberOfPagesToRead = ubsLogInfo->logState.endAddress / ubsLogInfo->logState.pageSize;	
	}
	
	logMessage("[MODBUS-UBS] States of the registry blocks changed. %d page(s) are available.", ubsLogInfo->numberOfPagesToRead);
	
	return 1;
}


int ModbusUbsClientCallback(unsigned handle, int xType, int errCode, void * callbackData)
{
	static unsigned char messageHeader[7], messageBody[254], functionID;
	static unsigned short transactionID, bytes_number;
	int msgBodySize;
	ubs_log_state_t newLogState;
	ubs_log_page_t logPage;
	modbus_block_data_t * pModbusBlockData;
	
	pModbusBlockData = (modbus_block_data_t *)callbackData;
	switch(xType)
	{
		case TCP_DISCONNECT:
			// Set the connection flag to zero; And the conversation handle to zero. (inside the callbackData object)
			SetDisconnectedStatus(&pModbusBlockData->connectionInfo);
			ResetLogReadingStatus(&pModbusBlockData->logInfo);
			break;
		case TCP_DATAREADY:
			// Read header
			if (ClientTCPRead(handle, messageHeader, 7, UBS_CONNECTION_READ_TIMEOUT) <= 0) {
				logMessage("[MODBUS-UBS]: Error! Unable to read data header from UBS over Modbus-TCP (yet it is available): %s", GetTCPSystemErrorString());

				DisconnectFromTCPServer(handle);
				SetDisconnectedStatus(&pModbusBlockData->connectionInfo);
				ResetLogReadingStatus(&pModbusBlockData->logInfo);
			} else {
				transactionID = getWordFromBigEndian(*(unsigned short*)messageHeader);
				bytes_number = getWordFromBigEndian(*(unsigned short*)(messageHeader + 4));
				
				// Read the rest of the message (minus one byte from the header's tail)
				if ( (msgBodySize = ClientTCPRead(handle, messageBody, bytes_number - 1, UBS_CONNECTION_READ_TIMEOUT)) < 0) {
					logMessage("[MODBUS-UBS]: Error! Unable to read data body from UBS over Modbus-TCP (yet it is available): %s", GetTCPSystemErrorString());
					
					DisconnectFromTCPServer(handle);
					SetDisconnectedStatus(&pModbusBlockData->connectionInfo);
					ResetLogReadingStatus(&pModbusBlockData->logInfo);
				} else {
					// Parse 
					// Parse the function ID
					functionID = messageBody[0];
					if (functionID & (1 << 7)) {
						// Error occured
						logMessage("[MODBUS-UBS]: Error! Modbus block returned the error code %d (0x%02X): %s", functionID, functionID, getTransactionName(transactionID));
						break;
					}
					
					if (transactionID == TRANSACTION_CODE_UBS_READ_ALL) {  // Reading available UBS data
						pModbusBlockData->processed_data = parseUbsData(messageBody + 2);  // skipping first two bytes	

					} else if (transactionID == TRANSACTION_CODE_UBS_SET_DAC) {  // Processing the answer after setting the DAC. 
						// TODO: maybe check the response	
					
					} else if (transactionID == TRANSACTION_CODE_UBS_READ_LOG_STATE) {  // Processing the UBS log state 
						newLogState = parseUbsLogState(messageBody + 2);
						pModbusBlockData->logInfo.logState = newLogState;
						
						if (SetLogReadingStatus(&pModbusBlockData->logInfo)) {
							requestUbsLogPages(handle, &pModbusBlockData->logInfo);  	
						}
						
					} else if (transactionID == TRANSACTION_CODE_UBS_READ_LOG_PAGES) {  // Reading Log pages
						//printf("Got LOG pages: %d bytes\n", msgBodySize - 2);
						logPage = parseLogPageData(messageBody + 2);
						
						if (logPage.err) {
							ResetLogReadingStatus(&pModbusBlockData->logInfo);  
							logMessage("[MODBUS-UBS]: Error! Unable to parse the log page data.");

						} else {
							pModbusBlockData->logInfo.logDataPages[pModbusBlockData->logInfo.currentPageIndex++] = logPage;
							
							if (pModbusBlockData->logInfo.currentPageIndex >= pModbusBlockData->logInfo.numberOfPagesToRead) {
								SaveEventsData(&pModbusBlockData->logInfo);
								ResetLogReadingStatus(&pModbusBlockData->logInfo); 
								requestUbsLogReset(handle);
							} else {
								requestUbsLogPages(handle, &pModbusBlockData->logInfo);		
							}
						}
					} else if (transactionID == TRANSACTION_CODE_UBS_RESET_LOG) {  // Reset the UBS log state 
						// TODO maybe do something
						
					} else {
						logMessage("[MODBUS-UBS]: Error! Recieved the message with unknown transaction ID (0x%04X)", transactionID);	
					}
					
					//switch
				}
			}
			break;
	}
	return 0;
}


double adcRawToVoltage_mV(unsigned short channelType, unsigned short rawValue) {
	double voltage_mv;

	if (channelType < 0 || channelType > 9) {
		logMessage("[MODBUS-UBS]: Error! Recieved the unsupported channel type (%d)", channelType);
		
		if (channelType < 0) channelType = 0; else channelType = 9;  // clamp value
	}
	
	voltage_mv = (double)(short)rawValue * ADC_RANGES_mV[channelType] / 27648;
	
	// For debug
	/*printf("channel type: %d\n", channelType);
	printf("Raw value: 0x%04X\n", rawValue);
	printf("Processed value: %lf\n", voltage_mv);*/
	
	return voltage_mv;
}


unsigned short dacVoltageToCode(double dacVoltage_volts) {
	double scaledVoltage = (dacVoltage_volts * (1000. / 10000. * 27648.));
	
	if (scaledVoltage > 27648) scaledVoltage = 27648;
	if (scaledVoltage < -27648) scaledVoltage = -27648;
	
	return (unsigned short)scaledVoltage;
}


ubs_processed_data_t parseUbsData(unsigned char * byteArray) {
	ubs_processed_data_t processedData = {0};
	int shift, i, j;
	unsigned short bigEndianWordValue;
	unsigned char bytes[4];
	
	shift = 0;
	
	// Reading DI
	for (i=0; i<DI_NUMBER; i++) {
		memcpy(bytes, byteArray + shift, 4);
		processedData.DI[i] = (unsigned)bytes[1] | ((unsigned)bytes[0] << 8) | ((unsigned)bytes[2] << 24) | ((unsigned)bytes[3] << 16);  
		shift += 4;
	}
	
	// Reading DQ
	for (i=0; i<DQ_NUMBER; i++) {
		memcpy(&bigEndianWordValue, byteArray + shift, 2);
		processedData.DQ[i] = getWordFromBigEndian(bigEndianWordValue);
		shift += 2;
	}
	
	// Reading OP
	for (i=0; i<OP_NUMBER; i++) {
		memcpy(&bigEndianWordValue, byteArray + shift, 2);
		processedData.OP[i] = getWordFromBigEndian(bigEndianWordValue);
		shift += 2;
	}
	
	// Reading diagnostics data
	memcpy(&bigEndianWordValue, byteArray + shift, 2);
	processedData.diagnostics = getWordFromBigEndian(bigEndianWordValue);
	shift += 2;
		
	// Reading ADC
	for (i=0; i < ADC_NUMBER; i++) {
		for (j=0; j < CHANNELS_PER_ADC; j++) {
			processedData.ADC[i].channelsType[j] = getWordFromBigEndian(*(unsigned short*)(byteArray + shift));
			processedData.ADC[i].channelsValuesRaw[j] = getWordFromBigEndian(*(unsigned short*)(byteArray + shift + 2));
			shift += sizeof(unsigned short) * 2;

			// Calculating the ADC voltage value
			/*printf("ADC (%d) channel (%d):\n", i, j); */  // For debug
			processedData.ADC[i].channelsValues[j] = 1e-3 * adcRawToVoltage_mV(processedData.ADC[i].channelsType[j], processedData.ADC[i].channelsValuesRaw[j]);
			processedData.ADC[i].channelsProcessedValues[j] = processedData.ADC[i].channelsValues[j] * UBS_ADC_COEFF[i][j][0] + UBS_ADC_COEFF[i][j][1];
			/*printf("Processed value: %lf\n", processedData.ADC[i].channelsProcessedValues[j]);*/  // For debug
		}
	}

	// Reading DAC
	for (i=0; i < DAC_NUMBER; i++) {
		for (j=0; j < CHANNELS_PER_DAC; j++) {
			processedData.DAC[i].channelsValuesRaw[j] = getWordFromBigEndian(*(unsigned short*)(byteArray + shift));
			shift += sizeof(unsigned short);
			
			// Calculating the DAC voltage value
			processedData.DAC[i].channelsValues[j] = 1e-3 * adcRawToVoltage_mV(DEFAULT_DAC_RANGE_TYPE, processedData.DAC[i].channelsValuesRaw[j]);
		}
	}
	
	return processedData;
}


ubs_log_state_t parseUbsLogState(unsigned char * byteArray) {
	ubs_log_state_t logState;
	
	logState.startAddress = getWordFromBigEndian(*(unsigned short*)(byteArray)); 
	logState.endAddress = getWordFromBigEndian(*(unsigned short*)(byteArray + 2));
	logState.maxAddress = getWordFromBigEndian(*(unsigned short*)(byteArray + 4));
	logState.pageSize = getWordFromBigEndian(*(unsigned short*)(byteArray + 6));
	
	return logState;
}


ubs_log_page_t parseLogPageData(unsigned char * byteArray) {
	ubs_log_page_t pageData = {0};
	struct tm * hostTimeDataStruct;
	unsigned char bytes[4];
	unsigned short bigEndianWordValue; 
	int i, shift;
	
	pageData.err = 0;  // Auxiliary parameter (if 1 - the programm will output the message to the log)
	
	// Writing the host time info
	time(&pageData.hostTimeStamp);
	hostTimeDataStruct = localtime(&pageData.hostTimeStamp);
	
	pageData.hostTime.year         = hostTimeDataStruct->tm_year + 1900;
	pageData.hostTime.month        = hostTimeDataStruct->tm_mon + 1; 
	pageData.hostTime.day          = hostTimeDataStruct->tm_mday; 
	pageData.hostTime.hours        = hostTimeDataStruct->tm_hour; 
	pageData.hostTime.minutes      = hostTimeDataStruct->tm_min; 
	pageData.hostTime.seconds      = hostTimeDataStruct->tm_sec; 
	pageData.hostTime.milliseconds = 0; 
	
	// Retrieving time stamp from the UBS module
	pageData.ubsBlockTime.year         = byteArray[2] * 256 + byteArray[3];
	pageData.ubsBlockTime.month        = byteArray[1]; 
	pageData.ubsBlockTime.day          = byteArray[0]; 
	pageData.ubsBlockTime.hours        = byteArray[4]; 
	pageData.ubsBlockTime.minutes      = byteArray[5]; 
	pageData.ubsBlockTime.seconds      = byteArray[7] & 0x3f; 
	pageData.ubsBlockTime.milliseconds = byteArray[6] * 4 + byteArray[7] / 64;
	
	// Retrieving the data for DI and DQ blocks
	shift = 8;

	// Reading DI
	for (i=0; i<DI_NUMBER; i++) {
		memcpy(bytes, byteArray + shift, 4);
		pageData.DI[i] = (unsigned)bytes[1] | ((unsigned)bytes[0] << 8) | ((unsigned)bytes[2] << 24) | ((unsigned)bytes[3] << 16);  
		shift += 4;
	}
	
	// Reading DQ
	for (i=0; i<DQ_NUMBER; i++) {
		memcpy(&bigEndianWordValue, byteArray + shift, 2);
		pageData.DQ[i] = getWordFromBigEndian(bigEndianWordValue);
		shift += 2;
	}
	
	return pageData;
}


void requestUbsData(unsigned int conversationHandle) {
	unsigned short requestBody[6];

	requestBody[0] = getBigEndianWord(TRANSACTION_CODE_UBS_READ_ALL);   // transaction ID 
	requestBody[1] = getBigEndianWord(0);								// something that must be always zero
	requestBody[2] = getBigEndianWord(6);								// word, describing the number of bytes of the rest of the message 
	requestBody[3] = getBigEndianWord((255 << 8) | 3);				 	// Unit identifier (seems like it is predefined and equal to 255) and Function code (3 - read)
	requestBody[4] = getBigEndianWord(0);								// The word defining the start address 
	requestBody[5] = getBigEndianWord(81);							 	// Word defining the number of words to read from the UBS module
	
	if (ClientTCPWrite(conversationHandle, requestBody, sizeof(requestBody), UBS_CONNECTION_SEND_TIMEOUT) <= 0) {
		logMessage("[MODBUS-UBS]: Error! Unable to send the data read request to the UBS module: %s", GetTCPSystemErrorString());	
	}
}


void requestLogState(unsigned int conversationHandle) {
	unsigned short requestBody[6];

	requestBody[0] = getBigEndianWord(TRANSACTION_CODE_UBS_READ_LOG_STATE);   // transaction ID 
	requestBody[1] = getBigEndianWord(0);								// something that must be always zero
	requestBody[2] = getBigEndianWord(6);								// word, describing the number of bytes of the rest of the message 
	requestBody[3] = getBigEndianWord((255 << 8) | 3);				 	// Unit identifier (seems like it is predefined and equal to 255) and Function code (3 - read)
	requestBody[4] = getBigEndianWord(LOG_ADDRESS);						// The word defining the start address 
	requestBody[5] = getBigEndianWord(4);							 	// Word defining the number of words to read from the UBS module
	
	if (ClientTCPWrite(conversationHandle, requestBody, sizeof(requestBody), UBS_CONNECTION_SEND_TIMEOUT) <= 0) {
		logMessage("[MODBUS-UBS]: Error! Unable to send the log state read request to the UBS module: %s", GetTCPSystemErrorString());	
	}	
}


void requestUbsLogPages(unsigned int conversationHandle, const ubs_log_info_t * logInfo) {
	unsigned short requestBody[6];

	if (!logInfo->numberOfPagesToRead || logInfo->currentPageIndex >= logInfo->numberOfPagesToRead) return;  // Nothing to read
	
	requestBody[0] = getBigEndianWord(TRANSACTION_CODE_UBS_READ_LOG_PAGES);   // transaction ID 
	requestBody[1] = getBigEndianWord(0);								// something that must be always zero
	requestBody[2] = getBigEndianWord(6);								// word, describing the number of bytes of the rest of the message 
	requestBody[3] = getBigEndianWord((255 << 8) | 3);				 	// Unit identifier (seems like it is predefined and equal to 255) and Function code (3 - read)
	
	// The word defining the start address 
	requestBody[4] = getBigEndianWord(LOG_ADDRESS + 4 + logInfo->currentPageIndex * logInfo->logState.pageSize);					
	
	requestBody[5] = getBigEndianWord(logInfo->logState.pageSize);	 			// Word defining the number of words to read from the UBS module
	
	if (ClientTCPWrite(conversationHandle, requestBody, sizeof(requestBody), UBS_CONNECTION_SEND_TIMEOUT) <= 0) {
		logMessage("[MODBUS-UBS]: Error! Unable to send the log pages read request to the UBS module: %s", GetTCPSystemErrorString());	
	}	
}


int requestUbsLogReset(unsigned int conversationHandle) {
	// Returns 0 if failed, 1 if succeeded
	unsigned short requestBodyBasePart[6];
	unsigned char requestBody[17];

	
	requestBodyBasePart[0] = getBigEndianWord(TRANSACTION_CODE_UBS_RESET_LOG);	 // transaction ID 
	requestBodyBasePart[1] = getBigEndianWord(0);								 // something that must be always zero
	requestBodyBasePart[2] = getBigEndianWord(11);								 // word, describing the number of bytes of the rest of the message 
	requestBodyBasePart[3] = getBigEndianWord((255 << 8) | 0x10);				 // Unit identifier (seems like it is predefined and equal to 255) and Function code (3 - read)
	requestBodyBasePart[4] = getBigEndianWord(LOG_ADDRESS);					 	 // The word defining the start address 
	requestBodyBasePart[5] = getBigEndianWord(2);							     // Word defining the number of words to write
	
	// Copy the data to a bytes array
	memcpy(requestBody, requestBodyBasePart, sizeof(requestBodyBasePart));
	
	requestBody[12] = 4;									 // number of bytes in the tail of the message
	
	// The following bytes will be ignored (but necessary to meet the requirements of the ModBus protocol)
	requestBody[13] = 0;
	requestBody[14] = 0;
	requestBody[15] = 0;
	requestBody[16] = 0;
	
	if (ClientTCPWrite(conversationHandle, requestBody, sizeof(requestBody), UBS_CONNECTION_SEND_TIMEOUT) < 0) {
		logMessage("[MODBUS-UBS]: Error! Unable to send the log reset command to the UBS module: %s", GetTCPSystemErrorString());
		return 0;
	}
	
	logMessage("[MODBUS-UBS] Sent the request to reset the log state.");
	
	return 1;
}


int writeUbsDAC(unsigned int conversationHandle, unsigned int dacIndex, unsigned int dacChannelIndex, double dacChannelVoltage) {
	// Returns 0 if failed, 1 if succeeded
	unsigned short requestBodyBasePart[6];
	unsigned char requestBody[15];
	unsigned short registerAddress;
	unsigned short dacVoltageCode;
	
	if (dacIndex >= DAC_NUMBER) {
		msAddMsg(
			msGMS(),
			"%s [CODE]: /-- writeUbsDAC --/ Error! DAC index is out of range: %d (largest DAC index is %d)",
			TimeStamp(0),
			dacIndex,
			DAC_NUMBER - 1
		);
		return 0;
	}
	
	if (dacChannelIndex >= CHANNELS_PER_DAC) {
		msAddMsg(
			msGMS(),
			"%s [CODE]: /-- writeUbsDAC --/ Error! DAC channel index is out of range: %d (largest DAC channel index is %d)",
			TimeStamp(0),
			dacChannelIndex,
			CHANNELS_PER_DAC - 1
		);
		return 0;	
	}
	
	if (dacChannelVoltage > 10. || dacChannelVoltage < -10) {
		msAddMsg(
			msGMS(),
			"%s [CODE]: /-- writeUbsDAC --/ Error! DAC channel voltage is out of range: %.4f (must be from -10 V to 10 V)",
			TimeStamp(0),
			dacChannelVoltage
		);
		return 0;	
	}
	
	registerAddress = 77 + dacIndex * CHANNELS_PER_DAC + dacChannelIndex;
	dacVoltageCode = dacVoltageToCode(dacChannelVoltage);
	
	requestBodyBasePart[0] = getBigEndianWord(TRANSACTION_CODE_UBS_SET_DAC);	 // transaction ID 
	requestBodyBasePart[1] = getBigEndianWord(0);								 // something that must be always zero
	requestBodyBasePart[2] = getBigEndianWord(9);								 // word, describing the number of bytes of the rest of the message 
	requestBodyBasePart[3] = getBigEndianWord((255 << 8) | 0x10);				 // Unit identifier (seems like it is predefined and equal to 255) and Function code (3 - read)
	requestBodyBasePart[4] = getBigEndianWord(registerAddress);					 // The word defining the start address 
	requestBodyBasePart[5] = getBigEndianWord(1);							     // Word defining the number of words to write
	
	// Copy the data to a bytes array
	memcpy(requestBody, requestBodyBasePart, sizeof(requestBodyBasePart));
	
	requestBody[12] = 2;									 // number of bytes in the tail of the message
	
	// Copy the DAC voltage code into the byte array
	requestBody[13] = (dacVoltageCode >> 8) & 0xFF;
	requestBody[14] = dacVoltageCode & 0xFF;
	
	if (ClientTCPWrite(conversationHandle, requestBody, sizeof(requestBody), UBS_CONNECTION_SEND_TIMEOUT) < 0) {
		logMessage("[MODBUS-UBS]: Error! Unable to send the DAC setup request to the UBS module: %s", GetTCPSystemErrorString());
		return 0;
	}	
	
	return 1;
}


void FormatUbsProcessedData(const ubs_processed_data_t *processedData, char *outputBuffer) {
	int i;
	outputBuffer[0] = 0;
	
	// Insert DI info
	FormatAllDiData(processedData->DI, outputBuffer + strlen(outputBuffer));
	strcat(outputBuffer, " ");
	
	// Insert DQ info
	FormatAllDqData(processedData->DQ, outputBuffer + strlen(outputBuffer));
	strcat(outputBuffer, " ");
	
	// Insert ADC info
	for (i=0; i<ADC_NUMBER; i++) {
		FormatAdcData(processedData, i, outputBuffer + strlen(outputBuffer));
		strcat(outputBuffer, " ");	
	}
	
	// Insert DAC info
	for (i=0; i<DAC_NUMBER; i++) {
		FormatDacData(processedData, i, outputBuffer + strlen(outputBuffer));
		strcat(outputBuffer, " ");	
	}
	
	// Insert Diagnostics data
	FormatDiagnosticsData(processedData, outputBuffer + strlen(outputBuffer));
}


int FormatAdcData(const ubs_processed_data_t *processedData, int adcIndex, char *outputBuffer) {
	int i;
	char formattedValue[64];
	
	outputBuffer[0] = 0;
	
	if (adcIndex < 0 || adcIndex >= ADC_NUMBER) {
		msAddMsg(
			msGMS(),
			"%s [CODE]: /-- FormatAdcData --/ Error! ADC index is out of range: %d (largest ADC index is %d)",
			TimeStamp(0),
			adcIndex,
			ADC_NUMBER - 1
		);
		return 0;	
	}
	
	for (i=0; i<CHANNELS_PER_ADC; i++) {
		if (i>0) strcat(outputBuffer, " ");
		sprintf(formattedValue, "%.8e", processedData->ADC[adcIndex].channelsProcessedValues[i]); 
        strcat(outputBuffer, formattedValue); 			
	}
	
	return 1;
}


int FormatDacData(const ubs_processed_data_t *processedData, int dacIndex, char *outputBuffer) {
	int i;
	char formattedValue[64];
	
	outputBuffer[0] = 0;
	
	if (dacIndex < 0 || dacIndex >= DAC_NUMBER) {
		msAddMsg(
			msGMS(),
			"%s [CODE]: /-- FormatAdcData --/ Error! DAC index is out of range: %d (largest DAC index is %d)",
			TimeStamp(0),
			dacIndex,
			DAC_NUMBER - 1
		);
		return 0;	
	}
	
	for (i=0; i<CHANNELS_PER_DAC; i++) {
		if (i>0) strcat(outputBuffer, " ");
		sprintf(formattedValue, "%.6lf", processedData->DAC[dacIndex].channelsValues[i]); 
        strcat(outputBuffer, formattedValue); 			
	}
	
	return 1;
}


void FormatAllDiData(const unsigned int *pDiData, char *outputBuffer) {
	int i;
	char formattedValue[64];
	
	outputBuffer[0] = 0;
	
	for (i=0; i<DI_NUMBER; i++) {
		if (i>0) strcat(outputBuffer, " ");
		sprintf(formattedValue, "%08X", pDiData[i]); 
        strcat(outputBuffer, formattedValue); 			
	}
}


void FormatAllDqData(const unsigned short *pDqData, char *outputBuffer) {
	int i;
	char formattedValue[64];
	
	outputBuffer[0] = 0;
	
	for (i=0; i<DQ_NUMBER; i++) {
		if (i>0) strcat(outputBuffer, " ");
		sprintf(formattedValue, "%04X", pDqData[i]); 
        strcat(outputBuffer, formattedValue); 			
	}
}


void FormatDiagnosticsData(const ubs_processed_data_t *processedData, char *outputBuffer) {
	sprintf(outputBuffer, "%04X", processedData->diagnostics);
}


void FormatLogPageData(const ubs_log_page_t *logPageData, char *outputBuffer) {
	sprintf(outputBuffer, "%10u ", logPageData->hostTimeStamp);
	
	// Writing host time info
	formatLogTimeInfo(logPageData->hostTime, outputBuffer + strlen(outputBuffer));
	strcat(outputBuffer, " ");
	
	// Writing UBS block event time
	formatLogTimeInfo(logPageData->ubsBlockTime, outputBuffer + strlen(outputBuffer));
	strcat(outputBuffer, " ");
	
	// Insert DI info
	FormatAllDiData(logPageData->DI, outputBuffer + strlen(outputBuffer));
	strcat(outputBuffer, " ");
	
	// Insert DQ info
	FormatAllDqData(logPageData->DQ, outputBuffer + strlen(outputBuffer));
}


void SaveEventsData(ubs_log_info_t * ubsLogInfo) {
	char eventPageData[512];
	int i;

	qsort(
		ubsLogInfo->logDataPages,
		ubsLogInfo->numberOfPagesToRead,
		sizeof(ubs_log_page_t),
		compareEventsTime
	);
	
	for (i=0; i < ubsLogInfo->numberOfPagesToRead; i++) {
		FormatLogPageData(&ubsLogInfo->logDataPages[i], eventPageData);
		WriteEventsFiles(eventPageData, FILE_EVENTS_DIRECTORY);	
	}
}


int FormatEventsData(time_t startTimeStamp, time_t endTimeStamp, char *outputBuffer) {
	#define MAX_RECORDS 40
	char eventsRecords[MAX_RECORDS + 1][256];	
	int recordsFound, tooManyRecords;
	int intBuff, dataPos, auxPos, scanRes;
	int i, j;
	char *stringPos, textBuffer[256];
	time_t eventTimeStamp;

	// ReadEventsFiles(const char *eventsDirectory, time_t startTimeStamp, time_t endTimeStamp, char **outputBuffer, int recordsMaxNumber, int *recordsFound) 
	ReadEventsFiles(FILE_EVENTS_DIRECTORY, startTimeStamp, endTimeStamp, eventsRecords, MAX_RECORDS + 1, &recordsFound);
	
	if (recordsFound > MAX_RECORDS) {
		tooManyRecords = 1;
		recordsFound = MAX_RECORDS;
	} else {
		tooManyRecords = 0;	
	}
	
	sprintf(outputBuffer, "%d %d", recordsFound, tooManyRecords);

	for (i=0; i<recordsFound; i++) {
		// Reading the eventTimeStamp and skipping the rest of time info
		scanRes = sscanf(
			eventsRecords[i],
			"%u %d %d %d %d %d %d %d %d %d %d %d %d %d %d%n",
			&eventTimeStamp,
			&intBuff, &intBuff, &intBuff, &intBuff, &intBuff, &intBuff, &intBuff,  // Skip 7 values of the server local time 
			&intBuff, &intBuff, &intBuff, &intBuff, &intBuff, &intBuff, &intBuff,  // Skip 7 values of the UBS block local time
			&dataPos
		);
		
		if (scanRes != 15) {
			logMessage("[EVENTS]: Error! Unable to parse the events record (missing time markers): \"%s\"", eventsRecords[i]);  
			return 0;
		}
		
		stringPos = eventsRecords[i] + dataPos;
		
		// Check that the rest of the record contains the necessary data
		for (j=0; j<DI_NUMBER; j++) {
			if (sscanf(stringPos, "%X%n", &intBuff, &auxPos) != 1) {
				logMessage("[EVENTS]: Error! Unable to parse the events record (missing state of DI-%d): \"%s\"", j, eventsRecords[i]);  
				return 0;		
			}
			stringPos += auxPos;
		}
		
		for (j=0; j<DQ_NUMBER; j++) {
			if (sscanf(stringPos, "%X%n", &intBuff, &auxPos) != 1) {
				logMessage("[EVENTS]: Error! Unable to parse the events record (missing state of DQ-%d): \"%s\"", j, eventsRecords[i]);  
				return 0;		
			}
			stringPos += auxPos;
		}
		
		// Format extracted data
		sprintf(textBuffer, " %u%s", eventTimeStamp, eventsRecords[i] + dataPos);
		strcat(outputBuffer, textBuffer);
	}
	
	return 1;
}
