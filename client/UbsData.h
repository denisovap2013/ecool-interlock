//==============================================================================
//
// Title:       UbsData.h
// Purpose:     A short description of the interface.
//
// Created on:  22.12.2020 at 13:20:37 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __UbsData_H__
#define __UbsData_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "cvidef.h"
#include <ansi_c.h>  

//==============================================================================
// Constants
#define ADC_NUMBER 4  
#define DAC_NUMBER 1
#define CHANNELS_PER_ADC 8
#define CHANNELS_PER_DAC 4
#define DI_NUMBER 2
#define DQ_NUMBER 3
#define CHANNELS_PER_DI 32 
#define CHANNELS_PER_DQ 16
#define CHANNELS_PER_DQ_ACTUAL 8
		
#define MAX_UBS_EVENTS 50
//==============================================================================
// Types
typedef struct ubs_event {
	time_t timeStamp;
	char textTimeInfo[64];
	unsigned int DI_VALUES[DI_NUMBER];
	unsigned short DQ_VALUES[DQ_NUMBER];
} ubs_event_t;

typedef struct ubs_event_list {
	ubs_event_t events[MAX_UBS_EVENTS + 1];
	int eventsNumber;
	int moreAvailable;
} ubs_event_list_t;

//==============================================================================
// External variables
extern unsigned int DI_VALUES[DI_NUMBER];
extern unsigned short DQ_VALUES[DQ_NUMBER];
extern double ADC_VALUES[ADC_NUMBER][CHANNELS_PER_ADC];
extern double DAC_VALUES[DAC_NUMBER][CHANNELS_PER_DAC];
extern unsigned short DEVICES_DIAGNOSTICS;
extern int SERVER_UBS_CONNECTED;
extern ubs_event_list_t UBS_EVENTS_LIST;  

//==============================================================================
// Global functions

int ParseValues(char *spaceSeparatedData);
int ParseConnectionState(char *textData);

int ParseEvents(char *eventsData);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __UbsData_H__ */
