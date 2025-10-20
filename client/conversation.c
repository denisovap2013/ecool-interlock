//==============================================================================
//
// Title:       conversation.c
// Purpose:     A short description of the implementation.
//
// Created on:  22.12.2020 at 10:35:45 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include <tcpsupp.h>
#include <ansi_c.h>
#include "conversation.h"
#include "MessageStack.h"
#include "TimeMarkers.h"

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
request_state_t globalRequestState = {0};
requests_queue_t globalRequestsQueue = {0};

//==============================================================================
// Global functions

int hasSameGlobalRequestsRecord(int requestID, int (*flags)[5]) {
	int i, j;
	if (globalRequestsQueue.num == 0) return 0;
	
	for (i=0; i<globalRequestsQueue.num; i++) {
		j = (globalRequestsQueue.currentPos + i) % MAX_REQUESTS_QUEUE_LENGTH;
		if (requestID != globalRequestsQueue.requestRecords[j].requestID) continue;
		if (flags == NULL) 
			return 1;
		else 
			if (memcmp(*flags, globalRequestsQueue.requestRecords[j].flags, sizeof(*flags)) == 0) 
				return 1;
	}
	return 0;
}


int hasSameGlobalRequestsRecordID(int requestID) {
	return hasSameGlobalRequestsRecord(requestID, NULL);
}


const requests_queue_record_t * popGlobalRequestQueueRecord(void) {
	requests_queue_record_t *pRecord;
	
	if (globalRequestsQueue.num == 0) return NULL;
	
	pRecord = &globalRequestsQueue.requestRecords[globalRequestsQueue.currentPos];
	globalRequestsQueue.currentPos = (globalRequestsQueue.currentPos + 1) % MAX_REQUESTS_QUEUE_LENGTH;
	globalRequestsQueue.num--;
	
	if (globalRequestsQueue.num == 0) globalRequestsQueue.currentPos = 0;

	return pRecord;
}


void clearGlobalRequestsQueue(void) {
	memset(&globalRequestsQueue, 0, sizeof(globalRequestsQueue));	
}


int appendGlobalRequestQueueRecord(int requestID, const char * requestBody, int (*flags)[5]) {
	int j;
    #ifdef __VERBOSE__ 
	char requestBodyClean[1024];
	int bodyLen;
	#endif
	
	
	#ifdef __VERBOSE__
	    strcpy(requestBodyClean, requestBody);
		if ((bodyLen = strlen(requestBodyClean)) > 0) requestBodyClean[bodyLen - 1] = 0; 
	    logMessage("[VERBOSE] Adding a global request to the queue: \"%s\"", requestBodyClean);
    #endif
	if (globalRequestsQueue.num == MAX_REQUESTS_QUEUE_LENGTH) {
        #ifdef __VERBOSE__
		    strcpy(requestBodyClean, requestBody);
		    if ((bodyLen = strlen(requestBodyClean)) > 0) requestBodyClean[bodyLen - 1] = 0;    
		    logMessage("[VERBOSE] The global requests queue is full, unable to add more requests: \"%s\"", requestBodyClean);
        #endif
		return 0;
	}
	
	// If request with the same ID and flags already exists we do not add new request 
	if (hasSameGlobalRequestsRecord(requestID, flags)) {
		#ifdef __VERBOSE__
		    strcpy(requestBodyClean, requestBody);
		    if ((bodyLen = strlen(requestBodyClean)) > 0) requestBodyClean[bodyLen - 1] = 0;    
		    logMessage("[VERBOSE] The global requests queue already has a command with the body \"%s\" and flags [%d, %d, %d, %d, %d].",
				requestBodyClean, flags[0], flags[1], flags[2], flags[3], flags[4]);
        #endif
		return 0;
	}
	
	j = (globalRequestsQueue.currentPos + globalRequestsQueue.num) % MAX_REQUESTS_QUEUE_LENGTH;
	globalRequestsQueue.num++;
	
	globalRequestsQueue.requestRecords[j].requestID = requestID;
	strcpy(globalRequestsQueue.requestRecords[j].requestBody, requestBody);
	if (flags != NULL) 
		memcpy(globalRequestsQueue.requestRecords[j].flags, *flags, sizeof(*flags));
	else
		memset(globalRequestsQueue.requestRecords[j].flags, 0, sizeof(globalRequestsQueue.requestRecords[j].flags)); 	
	
	return 1;
}


int sendRequestIfAvailable(int serverHandle) {
	const requests_queue_record_t *pRequestRecord;
	const char *command;
	
	// Check if other request is being processed
	if (globalRequestState.waiting) {
		if (!globalRequestState.finished) return 0;
		
		// If the previous request is finished, then reset the request state
		globalRequestState.waiting = 0;
		globalRequestState.finished = 0;
		globalRequestState.requestID = -1;
	}
	
	// Check if there are any other requests
	if ((pRequestRecord = popGlobalRequestQueueRecord()) == NULL) return 0;
	
	// If so, fill the request state fields
	globalRequestState.requestID = pRequestRecord->requestID;
	memcpy(globalRequestState.flags, pRequestRecord->flags, sizeof(globalRequestState.flags));
	command = pRequestRecord->requestBody;
	
	if (ClientTCPWrite(serverHandle, command, strlen(command), 10) < 0) {
		logMessage("Error! Unable to send the following command to the Interlock server: %s.\n", command);
		return 0;
	}
	
	// If data is sent successfully, set the @waiting@ flag
	globalRequestState.waiting = 1;
	return 1;
}
