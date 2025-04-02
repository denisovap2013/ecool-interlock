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
#define UBS_CMD_GET_ALLVALUES "UBS:GET:ALLVALUES"

#define UBS_CMD_GET_DI_VALUES "UBS:GET:DI:VALUES"
#define UBS_CMD_GET_DI_NAMES  "UBS:GET:DI:NAMES"
#define UBS_CMD_GET_DQ_VALUES "UBS:GET:DQ:VALUES"
#define UBS_CMD_GET_DQ_NAMES  "UBS:GET:DQ:NAMES"

#define UBS_CMD_GET_DIAGNOSTICS "UBS:GET:DIAGNOSTICS"

#define UBS_CMD_GET_ADC_VALUES "UBS:GET:ADC:VALUES"
#define UBS_CMD_GET_ADC_NAMES "UBS:GET:ADC:NAMES"
#define UBS_CMD_GET_ADC_COEFFICIENTS "UBS:GET:ADC:COEFFS"

#define UBS_CMD_SET_DAC_VALUE "UBS:GET:DAC:VALUE"
#define UBS_CMD_GET_DAC_VALUES "UBS:GET:DAC:VALUES"
#define UBS_CMD_GET_DAC_NAMES "UBS:GET:DAC:NAMES"
		
#define UBS_CMD_GET_CONNECTION_STATE "UBS:GET:CONNECTIONSTATE"
		
#define UBS_CMD_GET_EVENTS "UBS:GET:EVENTS"  


// Commands IDs
#define NO_REQUEST_ID 0
#define UBS_CMD_GET_ALLVALUES_ID 1

#define UBS_CMD_GET_DI_VALUES_ID 2
#define UBS_CMD_GET_DI_NAMES_ID 3
#define UBS_CMD_GET_DQ_VALUES_ID 4
#define UBS_CMD_GET_DQ_NAMES_ID 5

#define UBS_CMD_GET_DIAGNOSTICS_ID 6

#define UBS_CMD_GET_ADC_VALUES_ID 7
#define UBS_CMD_GET_ADC_NAMES_ID 8
#define UBS_CMD_GET_ADC_COEFFICIENTS_ID 9

#define UBS_CMD_SET_DAC_VALUE_ID 10
#define UBS_CMD_GET_DAC_VALUES_ID 11
#define UBS_CMD_GET_DAC_NAMES_ID 12
		
#define UBS_CMD_GET_CONNECTION_STATE_ID 13
#define UBS_CMD_GET_EVENTS_ID 14
		
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
