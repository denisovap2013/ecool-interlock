//==============================================================================
//
// Title:       Logging.h
// Purpose:     A short description of the interface.
//
// Created on:  17.12.2020 at 10:17:26 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __Logging_H__
#define __Logging_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "MessageStack.h" 

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// External variables

//==============================================================================
// Global functions

void WriteLogFiles(message_stack_t messageStack, const char *log_directory);
void WriteDataFiles(const char *data, const char *dataDirectory);
void DeleteOldFiles(const char *logDirectory, const char *dataDirectory, const char * eventsDirectory, int expirationDays);
void copyConfigurationFile(const char *dataDir, const char *configFile);
void WriteEventsFiles(const char *data, const char *eventsDirectory);
void ReadEventsFiles(const char *eventsDirectory, time_t startTimeStamp, time_t endTimeStamp, char outputBuffer[][256], int recordsMaxNumber, int *recordsFound);


#ifdef __cplusplus
    }
#endif

#endif  /* ndef __Logging_H__ */
