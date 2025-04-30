//==============================================================================
//
// Title:       conversation.h
// Purpose:     A short description of the interface.
//
// Created on:  22.12.2020 at 10:35:45 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __conversation_H__
#define __conversation_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "cvidef.h"

//==============================================================================
// Constants
		
// Commands
#define CMD_GET_ALLVALUES "INTERLOCK:GET:ALLVALUES"

#define CMD_GET_DI_VALUES "INTERLOCK:GET:DI:VALUES"
#define CMD_GET_DI_NAMES  "INTERLOCK:GET:DI:NAMES"
#define CMD_GET_DQ_VALUES "INTERLOCK:GET:DQ:VALUES"
#define CMD_GET_DQ_NAMES  "INTERLOCK:GET:DQ:NAMES"

#define CMD_GET_DIAGNOSTICS "INTERLOCK:GET:DIAGNOSTICS"

#define CMD_GET_ADC_VALUES "INTERLOCK:GET:ADC:VALUES"
#define CMD_GET_ADC_NAMES "INTERLOCK:GET:ADC:NAMES"
#define CMD_GET_ADC_COEFFICIENTS "INTERLOCK:GET:ADC:COEFFS"

#define CMD_SET_DAC_VALUE "INTERLOCK:GET:DAC:VALUE"
#define CMD_GET_DAC_VALUES "INTERLOCK:GET:DAC:VALUES"
#define CMD_GET_DAC_NAMES "INTERLOCK:GET:DAC:NAMES"
		
#define CMD_GET_CONNECTION_STATE "INTERLOCK:GET:CONNECTIONSTATE"
		
#define CMD_GET_EVENTS "INTERLOCK:GET:EVENTS"  


// Commands IDs
#define NO_REQUEST_ID 0
#define CMD_GET_ALLVALUES_ID 1

#define CMD_GET_DI_VALUES_ID 2
#define CMD_GET_DI_NAMES_ID 3
#define CMD_GET_DQ_VALUES_ID 4
#define CMD_GET_DQ_NAMES_ID 5

#define CMD_GET_DIAGNOSTICS_ID 6

#define CMD_GET_ADC_VALUES_ID 7
#define CMD_GET_ADC_NAMES_ID 8
#define CMD_GET_ADC_COEFFICIENTS_ID 9

#define CMD_SET_DAC_VALUE_ID 10
#define CMD_GET_DAC_VALUES_ID 11
#define CMD_GET_DAC_NAMES_ID 12
		
#define CMD_GET_CONNECTION_STATE_ID 13
#define CMD_GET_EVENTS_ID 14
		
//==============================================================================
// Types

#define MAX_REQUESTS_QUEUE_LENGTH 30		

typedef struct requests_queue_record {
	int requestID;
	char requestBody[256];
	int flags[5];
	
} requests_queue_record_t;


typedef struct requests_queue {
	requests_queue_record_t requestRecords[MAX_REQUESTS_QUEUE_LENGTH];
	int num;
	int currentPos;
	
} requests_queue_t;


typedef struct request_state {
	int waiting;
	int finished;
	int requestID; 
	int flags[5];
} request_state_t;

//==============================================================================
// External variables
extern request_state_t globalRequestState;
extern requests_queue_t globalRequestsQueue;
//==============================================================================
// Global functions
const requests_queue_record_t * popGlobalRequestQueueRecord(void);
int appendGlobalRequestQueueRecord(int requestID, const char * requestBody, int (*flags)[5]);
void clearGlobalRequestsQueue(void);
int hasSameGlobalRequestsRecord(int requestID, int (*flags)[5]);
int hasSameGlobalRequestsRecordID(int requestID);

int sendRequestIfAvailable(int serverHandle);


#ifdef __cplusplus
    }
#endif

#endif  /* ndef __conversation_H__ */
