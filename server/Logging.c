//==============================================================================
//
// Title:       Logging.c
// Purpose:     A short description of the implementation.
//
// Created on:  17.12.2020 at 10:17:26 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files


#include <formatio.h>
#include "toolbox.h"
#include <ansi_c.h>
#include <lowlvlio.h>
#include "Logging.h"
#include "MessageStack.h"
#include "TimeMarkers.h"

//==============================================================================
// Constants
#define LOG_FILE_PREFIX "ubsServerLog_"
#define DATA_FILE_PREFIX "ubsServerData_"
#define EVENTS_FILE_PREFIX "ubsServerEvents_"

//==============================================================================
// Types

//==============================================================================
// Static global variables


//==============================================================================
// Static functions

int extractDate(char * input, int * year, int *month, int *day) {
	char inputCopy[512];
	strcpy(inputCopy, input);
	inputCopy[4] = ' ';
	inputCopy[7] = ' ';
	if (sscanf(inputCopy, "%d %d %d", year, month, day) == 3) return 1; else return 0;
}


void setOrCreateRelDirectory(const char *relDirName) {
	static char dir[500];

	GetProjectDir(dir); 
	strcat(dir, "\\"); 
	strcat(dir, relDirName);
	if (SetDir(dir) < 0) {
		if (MakeDir(dir) == 0) {
			SetDir(dir);	
		}
	}	
}


void GoToTheProjectDir(void) {
	static char dir[500];

	GetProjectDir(dir); 
	SetDir(dir);
}


void RemoveFilesFromDir(const char *dirName, const char *prefix, int expirationDays) {
	char fileName[512], *pos;
	int day, month, year, curDays;

	GetSystemDate(&month, &day, &year);
	curDays = (year-2000) * 365 + month * 30 + day;
	
	if (SetDir(dirName) == 0) {
		if ( GetFirstFile("*", 1, 0, 0, 0, 0, 0, fileName) == 0 ){
			do {
				if ( (pos = strstr(fileName, prefix)) != NULL ) {
					if (extractDate(pos + strlen(prefix), &year, &month, &day)) {
						if ( (curDays - (year-2000)*365 - month*30 - day) > expirationDays ) {
							DeleteFile(fileName);		
						}
					}
				}
			} while (GetNextFile(fileName) == 0);
		}	
	}	
}


void AppendStringToAFileHandler(const char *data, FILE * outputFile) {

	int res;
	
	if (outputFile == NULL) {
	    printf("%s [RUNTIME] Unable to write to the file using the provided handler. The handler is NULL.\n", TimeStamp(0));  
		return;
	}
	
	if ((res = fprintf(outputFile, data)) < 0) {
	    printf("%s [RUNTIME] Unable to write to the file using the provided handler. The error code is %d.\n", TimeStamp(0), res);	
	}
}


void AppendStringToAFile(const char *data, const char *directory, const char *prefix) {
	static char fileName[512];
	static int day,month,year;
	FILE * outputFile;
	int res;
	
	GetSystemDate(&month, &day, &year);
	setOrCreateRelDirectory(directory);
	sprintf(fileName, "%s%d.%02d.%02d.dat", prefix, year, month, day);    
	outputFile = fopen(fileName, "a");

	if (outputFile != NULL) {
		if ((res = fprintf(outputFile, data)) < 0) {
		    printf("%s [RUNTIME] Unable to write to the file \"%s\". The error code is %d.\n", TimeStamp(0), fileName, res);	
		}
		fclose(outputFile);

	} else {
		// TODO: maybe inform about errors	
		printf("%s [RUNTIME] Unable to open the file for writing: \"%s\"\n", TimeStamp(0), fileName);
	}	
}


//==============================================================================
// Global variables

//==============================================================================
// Global functions

void WriteLogFiles(message_stack_t messageStack, const char *logDirectory){
	static char fileName[512];
	static int day, month, year;
	FILE * outputFile;
	
	if ( msMsgsAvailable(messageStack) ) {
		////
		GetSystemDate(&month, &day, &year);
		setOrCreateRelDirectory(logDirectory);
		////
		sprintf(fileName, "%s%d.%02d.%02d.dat", LOG_FILE_PREFIX, year, month, day);    
		outputFile = fopen(fileName,"a");
		if (outputFile == NULL) {
			// Inform about errors
		} else {
			msPrintMsgs(messageStack, outputFile);
			fclose(outputFile);
		}
	}
}


void WriteDataFiles(const char *data, const char *dataDirectory) {
	static time_t curTime;
	char formattedData[1024]; 

	time(&curTime); 
	sprintf(formattedData, "%u %s\n", curTime, data);
	AppendStringToAFile(formattedData, dataDirectory, DATA_FILE_PREFIX);
}


void DeleteOldFiles(const char *logDirectory, const char *dataDirectory, const char * eventsDirectory, int expirationDays) {
	char dir[500];
	char logDir[512], dataDir[512], eventsDir[512];
	
	GetProjectDir(dir);
	sprintf(logDir, "%s\\%s", dir, logDirectory);
	sprintf(dataDir, "%s\\%s", dir, dataDirectory);
	sprintf(eventsDir, "%s\\%s", dir, eventsDirectory); 

	// Delete old log files
	RemoveFilesFromDir(logDir, LOG_FILE_PREFIX, expirationDays);
	
	// Delete old data files
	RemoveFilesFromDir(dataDir, DATA_FILE_PREFIX, expirationDays);
	
	// Delete old events files
	RemoveFilesFromDir(eventsDir, EVENTS_FILE_PREFIX, expirationDays);
}


void WriteEventsFiles(const char *data, const char *eventsDirectory) {
	char formattedData[1024];
	
	sprintf(formattedData, "%s\n", data);
	AppendStringToAFile(formattedData, eventsDirectory, EVENTS_FILE_PREFIX);
}


void copyConfigurationFile(const char *dataDir, const char *configFile) {
	int day, month, year, hours, minutes, seconds;
	char fileName[256];
	
	GetSystemDate(&month, &day, &year);
	GetSystemTime(&hours, &minutes, &seconds);

	GoToTheProjectDir();
	sprintf(fileName, "%s\\serverConfiguration_%02d.%02d.%02d_%02d-%02d-%02d.ini", dataDir, year, month, day, hours, minutes, seconds);

	CopyFile(configFile, fileName); 
}


void ReadEventsFiles(const char *eventsDirectory, time_t startTimeStamp, time_t endTimeStamp, char outputBuffer[][256], int recordsMaxNumber, int *recordsFound) {
	char eventRecord[512], fileName[512];
	int selectedRecordsNum;
	struct tm *fileTm;
	time_t fileTimeStamp, eventTimeStamp;
	int fileHandle = -1;
	
	
	setOrCreateRelDirectory(eventsDirectory);
	
	fileTimeStamp = startTimeStamp;
	selectedRecordsNum = 0; 
	
	for (fileTimeStamp = startTimeStamp; fileTimeStamp <= endTimeStamp; fileTimeStamp += 24 * 3600)  {  // Adding one day
		fileTm = localtime(&fileTimeStamp);
		
		sprintf(fileName, "%s%d.%02d.%02d.dat", EVENTS_FILE_PREFIX, fileTm->tm_year + 1900, fileTm->tm_mon + 1, fileTm->tm_mday); 
		
		if (!FileExists(fileName, 0)) continue;	
		
		if ((fileHandle = OpenFile(fileName, VAL_READ_ONLY, VAL_OPEN_AS_IS, VAL_ASCII)) == -1) {
			printf("Unable to read the event file \"%s\".\n", fileName);
			continue;
		}
		
		while (!eof(fileHandle)) {
			if (ReadLine(fileHandle, eventRecord, sizeof(eventRecord) - 1) <= 0) continue;
			
			if (sscanf(eventRecord, "%u", &eventTimeStamp) != 1) {
				printf("Unable ro read an event record from a file \"%s\".\n", fileName);
				continue;
			}
			
			if (eventTimeStamp >= startTimeStamp && eventTimeStamp <= endTimeStamp) 
				strcpy(outputBuffer[selectedRecordsNum++], eventRecord);
			
			if (selectedRecordsNum == recordsMaxNumber) break;
		}
		
		// Close the file
		CloseFile(fileHandle); fileHandle =-1;
		if (selectedRecordsNum == recordsMaxNumber) break;
	}
	
	*recordsFound = selectedRecordsNum;
}
