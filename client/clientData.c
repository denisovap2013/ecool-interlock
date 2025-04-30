//==============================================================================
// Include files

#include <ansi_c.h>
#include "clientData.h"
#include "TimeMarkers.h"
#include "MessageStack.h"

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
unsigned int DI_VALUES[DI_NUMBER] = {0};
unsigned short DQ_VALUES[DQ_NUMBER] = {0};
double ADC_VALUES[ADC_NUMBER][CHANNELS_PER_ADC] = {0};
double DAC_VALUES[DAC_NUMBER][CHANNELS_PER_DAC] = {0};
unsigned short DEVICES_DIAGNOSTICS = 0;
int SERVER_UBS_CONNECTED = 0;
ubs_event_list_t UBS_EVENTS_LIST = {0};
//==============================================================================
// Global functions


int ParseValues(char *spaceSeparatedData) {
	int i, j, shift, pos;
	
	shift = 0;

	for (i=0; i<DI_NUMBER; i++) {
		if (sscanf(spaceSeparatedData + shift, "%X%n", &DI_VALUES[i], &pos) != 1) {
			msAddMsg(msGMS(), "%s Error! Unable to parse DI-%d hex data.\n", TimeStamp(0), i);
			return -1;
		}
		shift += pos;
	}
	
	for (i=0; i<DQ_NUMBER; i++) {
		if (sscanf(spaceSeparatedData + shift, "%hX%n", &DQ_VALUES[i], &pos) != 1) {
			msAddMsg(msGMS(), "%s Error! Unable to parse DQ-%d hex data.\n", TimeStamp(0), i);
			return -1;
		}
		shift += pos;
	}
	
	for (i=0; i<ADC_NUMBER; i++) {
		for(j=0; j<CHANNELS_PER_ADC; j++) {
			if (sscanf(spaceSeparatedData + shift, "%lf%n", &ADC_VALUES[i][j], &pos) != 1) {
				msAddMsg(msGMS(), "%s Error! Unable to parse ADC-%d (channel %d) decimal data.\n", TimeStamp(0), i, j);
				return -1;
			}
			shift += pos;	
		}
		
	}
	
	for (i=0; i<DAC_NUMBER; i++) {
		for(j=0; j<CHANNELS_PER_DAC; j++) {
			if (sscanf(spaceSeparatedData + shift, "%lf%n", &DAC_VALUES[i][j], &pos) != 1) {
				msAddMsg(msGMS(), "%s Error! Unable to parse DAC-%d (channel %d) decimal data.\n", TimeStamp(0), i, j);
				return -1;
			}
			shift += pos;	
		}
		
	}
	
	if (sscanf(spaceSeparatedData + shift, "%hX%n", &DEVICES_DIAGNOSTICS, &pos) != 1) {
		msAddMsg(msGMS(), "%s Error! Unable to parse diagnostics hex data.\n", TimeStamp(0));
		return -1;
	}
	
	return 0;
}


int ParseConnectionState(char *textData) {
	if (sscanf(textData, "%d", &SERVER_UBS_CONNECTED) != 1) {
		msAddMsg(msGMS(), "%s Error! Unable to parse server status info.\n", TimeStamp(0));
		return -1;
	}
	return 0;
}


int ParseEvents(char *eventsData) {
	int i, j, pos;
	char *dataPos;
	
	dataPos = eventsData;
	
	// Get the number of events and flag indicating if more data is available
	if (sscanf(dataPos, "%d%n", &UBS_EVENTS_LIST.eventsNumber, &pos) != 1) {
		msAddMsg(msGMS(), "%s Error! Unable to parse the number of events.\n", TimeStamp(0));
		return -1;
	}
	dataPos += pos;
	
	// If no events available, skip further reading
	if (UBS_EVENTS_LIST.eventsNumber == 0) {
		UBS_EVENTS_LIST.moreAvailable = 0;
		return 0;
	}
	
	if (sscanf(dataPos, "%d%n", &UBS_EVENTS_LIST.moreAvailable, &pos) != 1) {
		msAddMsg(msGMS(), "%s Error! Unable to parse the flag indicating if more data is available.\n", TimeStamp(0));
		return -1;
	}
	dataPos += pos;
	
	// For each event parse the info
	for (j=0; j<UBS_EVENTS_LIST.eventsNumber; j++) {
		if (sscanf(dataPos, "%u%n", &UBS_EVENTS_LIST.events[j].timeStamp, &pos) != 1) {
			msAddMsg(msGMS(), "%s Error! Unable to parse timestamp data for event %d (counting from zero).\n", TimeStamp(0), j);
			return -1;
		}
		dataPos += pos;
		
		for (i=0; i<DI_NUMBER; i++) {
			if (sscanf(dataPos, "%X%n", &UBS_EVENTS_LIST.events[j].DI_VALUES[i], &pos) != 1) {
				msAddMsg(msGMS(), "%s Error! Unable to parse DI-%d hex data for event %d (counting from zero).\n", TimeStamp(0), i, j);
				return -1;
			}
			dataPos += pos;
		}
	
		for (i=0; i<DQ_NUMBER; i++) {
			if (sscanf(dataPos, "%hX%n", &UBS_EVENTS_LIST.events[j].DQ_VALUES[i], &pos) != 1) {
				msAddMsg(msGMS(), "%s Error! Unable to parse DQ-%d hex data for event %d (counting from zero).\n", TimeStamp(0), i, j);
				return -1;
			}
			dataPos += pos;
		}
		
		// Turn the timestamp into the text representation
		strftime(
			UBS_EVENTS_LIST.events[j].textTimeInfo,
			sizeof(UBS_EVENTS_LIST.events[j].textTimeInfo),
			"%Y.%m.%d - %X",
			localtime(&UBS_EVENTS_LIST.events[j].timeStamp)
		);
	}

	return 0;
}
