//==============================================================================
//
//==============================================================================

#ifndef __TimeMarkers_H__
#define __TimeMarkers_H__

#define MAX_RECORDS_NUMBER 20

typedef struct timeScheduleRecord_t {
	int active;
	time_t timeActivated;
	unsigned int timeInterval; // milliseconds		 //seconds ?
	void (*func)(int);
} timeScheduleRecord;

typedef struct timeSchedule_t {
	int numberOfRecords;
	timeScheduleRecord records[MAX_RECORDS_NUMBER];
} timeSchedule;

extern timeSchedule globalSchedule;
extern char globalSharedTimeMarker[30];

////////////////////////////////////////////////////////////////////// 
void releaseSchedule(void);

int addRecordToSchedule(int active, int immediate, unsigned int timeInterval, void (*func)(int));

int activateScheduleRecord(int record,int immediate);

int resetScheduleRecord(int record);

int deactivateScheduleRecord(int record);

void processScheduleEvents(void);
//////////////////////////////////////////////////////////////////////
const char * TimeStamp(char *time);

#endif  /* ndef __TimeMarkers_H__ */
