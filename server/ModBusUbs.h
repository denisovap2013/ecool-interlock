//==============================================================================
//
// Title:       ModBusUbs.h
// Purpose:     A short description of the interface.
//
// Created on:  10.12.2020 at 12:42:57 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __ModBusUbs_H__
#define __ModBusUbs_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "cvidef.h"
#include <time.h>

//==============================================================================
// Constants
#define ADC_NUMBER 4
#define DAC_NUMBER 1
#define CHANNELS_PER_ADC 8
#define CHANNELS_PER_DAC 4
#define DI_NUMBER 2
#define DQ_NUMBER 3
#define OP_NUMBER 5
		
//==============================================================================
// Types
typedef struct modbus_connection_info
{
	unsigned int conversationHandle;
	int connected;  // 1 means OK, 0 - disconnected
} modbus_connection_info_t;


typedef struct adc_data {
	double channelsValues[CHANNELS_PER_ADC];  // Volts
	double channelsProcessedValues[CHANNELS_PER_ADC];  // After linear transformation
	unsigned short channelsValuesRaw[CHANNELS_PER_ADC];
	unsigned short channelsType[CHANNELS_PER_ADC];
} adc_data_t;


typedef struct dac_data {
	double channelsValues[CHANNELS_PER_DAC];
	unsigned short channelsValuesRaw[CHANNELS_PER_DAC];
} dac_data_t;


typedef struct ubs_processed_data {
	unsigned int DI[DI_NUMBER];
	unsigned short DQ[DQ_NUMBER];
	unsigned short OP[OP_NUMBER];
	unsigned short diagnostics;
	adc_data_t ADC[ADC_NUMBER];
	dac_data_t DAC[DAC_NUMBER];
} ubs_processed_data_t;


typedef struct time_info {
	int year;
	int month;
	int day;
	int hours;
	int minutes;
	int seconds;
	int milliseconds;
	
} time_info_t;


typedef struct ubs_log_page {
	int err;
	unsigned int hostTimeStamp;
	time_info_t hostTime;
	time_info_t ubsBlockTime;
	unsigned int DI[DI_NUMBER];
	unsigned short DQ[DQ_NUMBER];
} ubs_log_page_t;


typedef struct ubs_log_state {
	unsigned short startAddress;
	unsigned short endAddress;
	unsigned short maxAddress;
	unsigned short pageSize;	
} ubs_log_state_t;


typedef struct ubs_log_info {
	ubs_log_state_t logState;
	
	// Parameters to control the LOG data flow.
	int currentlyReading;
	int currentPageIndex;
	int numberOfPagesToRead;
	
	// Data
	ubs_log_page_t logDataPages[100];
} ubs_log_info_t;


typedef struct modbus_block_data {
	ubs_processed_data_t processed_data;
	ubs_log_info_t logInfo;
	modbus_connection_info_t connectionInfo;
} modbus_block_data_t;
//==============================================================================
// External variables

//==============================================================================
// Global functions

unsigned short getBigEndianWord(unsigned short value);
unsigned int getBigEndianInt(unsigned int value); 
unsigned short getWordFromBigEndian(unsigned short value);
unsigned int getIntFromBigEndian(unsigned int value);

void ConnectToUbsBlock(char *ip,unsigned int port, unsigned int timeout, void *callbackData);
void SetDisconnectedStatus(modbus_connection_info_t * modbusConnectionInfo);
void ResetLogReadingStatus(ubs_log_info_t * ubsLogInfo);  
int SetLogReadingStatus(ubs_log_info_t * ubsLogInfo);
int ModbusUbsClientCallback(unsigned handle, int xType, int errCode, void * callbackData);

void requestUbsData(unsigned int conversationHandle);
void requestLogState(unsigned int conversationHandle);
void requestUbsLogPages(unsigned int conversationHandle, const ubs_log_info_t * logInfo);
int requestUbsLogReset(unsigned int conversationHandle);
int writeUbsDAC(unsigned int conversationHandle, unsigned int dacIndex, unsigned int dacChannelIndex, double dacChannelVoltage);

ubs_processed_data_t parseUbsData(unsigned char * byteArray);
ubs_log_state_t parseUbsLogState(unsigned char * byteArray);
ubs_log_page_t parseLogPageData(unsigned char * byteArray);

double adcRawToVoltage_mV(unsigned short channelType, unsigned short rawValue);
unsigned short dacVoltageToCode(double dacVoltage);

void SaveEventsData(ubs_log_info_t * ubsLogInfo);

void FormatUbsProcessedData(const ubs_processed_data_t *processedData, char *outputBuffer);
int FormatAdcData(const ubs_processed_data_t *processedData, int adcIndex, char *outputBuffer);
int FormatDacData(const ubs_processed_data_t *processedData, int dacIndex, char *outputBuffer);
void FormatAllDiData(const unsigned int *pDiData, char *outputBuffer);
void FormatAllDqData(const unsigned short *pDqData, char *outputBuffer);
void FormatDiagnosticsData(const ubs_processed_data_t *processedData, char *outputBuffer);
void FormatLogPageData(const ubs_log_page_t *logPageData, char *outputBuffer);		  
int FormatEventsData(time_t startTimeStamp, time_t endTimeStamp, char *outputBuffer);


#ifdef __cplusplus
    }
#endif

#endif  /* ndef __ModBusUbs_H__ */
