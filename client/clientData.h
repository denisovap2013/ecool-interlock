#ifndef __clientData_H__
#define __clientData_H__

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
		
#define MAX_INTERLOCK_EVENTS 50
//==============================================================================
// Types
typedef struct interlock_event {
	time_t timeStamp;
	char textTimeInfo[64];
	unsigned int DI_VALUES[DI_NUMBER];
	unsigned short DQ_VALUES[DQ_NUMBER];
} interlock_event_t;

typedef struct interlock_event_list {
	interlock_event_t events[MAX_INTERLOCK_EVENTS + 1];
	int eventsNumber;
	int moreAvailable;
} interlock_event_list_t;

//==============================================================================
// External variables
extern unsigned int DI_VALUES[DI_NUMBER];
extern unsigned short DQ_VALUES[DQ_NUMBER];
extern double ADC_VALUES[ADC_NUMBER][CHANNELS_PER_ADC];
extern double DAC_VALUES[DAC_NUMBER][CHANNELS_PER_DAC];
extern unsigned short DEVICES_DIAGNOSTICS;
extern int SERVER_HARDWARE_CONNECTED;
extern interlock_event_list_t INTERLOCK_EVENTS_LIST;  

//==============================================================================
// Global functions

int ParseValues(char *spaceSeparatedData);
int ParseConnectionState(char *textData);

int ParseEvents(char *eventsData);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __clientData_H__ */
