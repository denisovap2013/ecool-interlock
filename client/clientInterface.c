//==============================================================================
//
// Title:       clientInterface.c
// Purpose:     A short description of the implementation.
//
// Created on:  28.09.2015 at 13:08:35 by OEM.
// Copyright:   OEM. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include <utility.h>
#include <userint.h>
#include <ansi_c.h>

#include "clientInterface.h"
#include "ECoolInterlockClient.h"
#include "clientData.h"
#include "TimeMarkers.h"
#include "MessageStack.h"
#include "clientConfiguration.h"
#include "conversation.h" 

//==============================================================================
// Constants

#define BLOCK_BUTTON_HEIGHT 20 
#define BLOCK_BUTTON_WIDTH  160
#define BLOCK_GROUPS_SPACE 15
#define BLOCK_ELEMENT_STEP 20
#define ADC_BUTTON_HEIGHT 20
#define ADC_BUTTON_WIDTH 20  
#define ADC_FIELD_WIDTH 80
#define DI_ITEM_STEP 20
#define ADC_ITEM_STEP 22
#define DIAGNOSTICS_LED_WIDTH 20

#define WAITING_INFO "Waiting the info from the server"

//==============================================================================
// Global variables
int mainPanelHandle, helpPanelHandle, eventPanelHandle;

int clientToServerConnectionLED, serverToHardwareConnectionLED;

int diPanelHandles[DI_NUMBER], diPanelCallButtons[DI_NUMBER];
int dqPanelHandles[DQ_NUMBER], dqPanelCallButtons[DQ_NUMBER];
int adcPanelHandles[ADC_NUMBER], adcPanelCallButtons[ADC_NUMBER];
int dacPanelHandles[DAC_NUMBER], dacPanelCallButtons[DAC_NUMBER];

int diagnosticsLEDs[16];
int helpBtnHandle, eventBtnHandle;

// DI blocks gui
int diIndicatorsHandles[DI_NUMBER][CHANNELS_PER_DI];
int diEventsIndicatorsHandles[DI_NUMBER][CHANNELS_PER_DI];
char diTextLabels[DI_NUMBER][CHANNELS_PER_DI][256];

// DQ blocks gui
int dqIndicatorsHandles[DQ_NUMBER][CHANNELS_PER_DQ_ACTUAL];
int dqEventsIndicatorsHandles[DQ_NUMBER][CHANNELS_PER_DQ_ACTUAL]; 
char dqTextLabels[DQ_NUMBER][CHANNELS_PER_DQ_ACTUAL][256];

// ADC blocks gui
int adcGraphCallButtons[ADC_NUMBER][CHANNELS_PER_ADC];
int adcGraphPanels[ADC_NUMBER][CHANNELS_PER_ADC];
int adcFieldsHandles[ADC_NUMBER][CHANNELS_PER_ADC]; 
char adcTextLabels[ADC_NUMBER][CHANNELS_PER_ADC][256];

// DAC blocks gui
int dacFieldsHandles[DAC_NUMBER][CHANNELS_PER_DAC];
char dacTextLabels[DAC_NUMBER][CHANNELS_PER_DAC][256];


//==============================================================================
// Static functions
int createButton(int panelHandle, int x, int y, int width, int height, char *label, int color) {
	int buttonHandle;
	
	buttonHandle = NewCtrl(panelHandle, CTRL_SQUARE_COMMAND_BUTTON_LS, label, y, x);
	SetCtrlAttribute(panelHandle, buttonHandle, ATTR_HEIGHT, height);
	SetCtrlAttribute(panelHandle, buttonHandle, ATTR_WIDTH, width); 
	SetCtrlAttribute(panelHandle, buttonHandle, ATTR_CMD_BUTTON_COLOR, color);	
	return buttonHandle;
}

int createMainPanelButton(int x, int y, int width, int height, char *label, int color) {
	int buttonHandle;
	
	buttonHandle = createButton(mainPanelHandle, x, y, width, height, label, color);
	// installing callback
	InstallCtrlCallback(mainPanelHandle, buttonHandle, mainMenuButtonCallback, NULL);
	
	return buttonHandle;
}


int createDiagnosticsLED(int x, int y, int width, int height) {
	int ledHandle;
	
	ledHandle = NewCtrl(mainPanelHandle, CTRL_SQUARE_LED, "", y, x);
	SetCtrlAttribute(mainPanelHandle, ledHandle, ATTR_HEIGHT, height);
	SetCtrlAttribute(mainPanelHandle, ledHandle, ATTR_WIDTH, width); 
	SetCtrlAttribute(mainPanelHandle, ledHandle, ATTR_OFF_COLOR, 0xEEEEEE);
	SetCtrlVal(mainPanelHandle, ledHandle, 1);
	
	return ledHandle;
}


time_t getFromTimeStamp(void) {
	struct tm fromTime;

	GetCtrlVal(eventPanelHandle, eventPanel_fromYear, &fromTime.tm_year);
	GetCtrlVal(eventPanelHandle, eventPanel_fromMonth, &fromTime.tm_mon);
	GetCtrlVal(eventPanelHandle, eventPanel_fromDay, &fromTime.tm_mday);
	GetCtrlVal(eventPanelHandle, eventPanel_fromHour, &fromTime.tm_hour);
	GetCtrlVal(eventPanelHandle, eventPanel_fromMinute, &fromTime.tm_min);
	GetCtrlVal(eventPanelHandle, eventPanel_fromSecond, &fromTime.tm_sec);
	fromTime.tm_year -= 1900;
	fromTime.tm_mon -= 1;
	fromTime.tm_isdst = 0;	
	
	return mktime(&fromTime);
}


time_t getToTimeStamp(void) {
	struct tm toTime;

	GetCtrlVal(eventPanelHandle, eventPanel_toYear, &toTime.tm_year);
	GetCtrlVal(eventPanelHandle, eventPanel_toMonth, &toTime.tm_mon);
	GetCtrlVal(eventPanelHandle, eventPanel_toDay, &toTime.tm_mday);
	GetCtrlVal(eventPanelHandle, eventPanel_toHour, &toTime.tm_hour);
	GetCtrlVal(eventPanelHandle, eventPanel_toMinute, &toTime.tm_min);
	GetCtrlVal(eventPanelHandle, eventPanel_toSecond, &toTime.tm_sec);
	toTime.tm_year -= 1900;
	toTime.tm_mon -= 1;
	toTime.tm_isdst = 0;	
	
	return mktime(&toTime);
}


// Global functions

int CreateBlocksGui(void) {
	int res;
	
	// Create panels
	if ( (res = createBlocksPanels()) < 0 ) return res;
	
	// Add buttons onto the main panel
	if ( (res = createMainPanelGui()) < 0 ) return res; 
	
	// Setup the GUI of the blocks panels
	if ( (res = setupBlocksPanels()) < 0 ) return res; 
	
	// Setup the GUI of the Events Panle
	if ( (res = setupEventsPanel()) < 0 ) return res;
	
	return 0;
}


int createBlocksPanels(void) {
	int i;
	char title[256];

	// DI blocks
	for (i=0; i<DI_NUMBER; i++) {
		if ( (diPanelHandles[i] = LoadPanel (0, "ECoolInterlockClient.uir", BlockPanel)) < 0 ) return diPanelHandles[i];
		sprintf(title, "DI block %d", i);
		SetPanelAttribute(diPanelHandles[i], ATTR_TITLE, title);
	}
	
	// DQ blocks
	for (i=0; i<DQ_NUMBER; i++) {
		if ( (dqPanelHandles[i] = LoadPanel (0, "ECoolInterlockClient.uir", BlockPanel)) < 0 ) return dqPanelHandles[i];
		sprintf(title, "DQ block %d", i);
		SetPanelAttribute(dqPanelHandles[i], ATTR_TITLE, title);
	}
	
	// ADC blocks
	for (i=0; i<ADC_NUMBER; i++) {
		if ( (adcPanelHandles[i] = LoadPanel (0, "ECoolInterlockClient.uir", BlockPanel)) < 0 ) return adcPanelHandles[i];
		sprintf(title, "DQ block %d", i);
		SetPanelAttribute(adcPanelHandles[i], ATTR_TITLE, title);
	}
	
	// DAC blocks
	for (i=0; i<DAC_NUMBER; i++) {
		if ( (dacPanelHandles[i] = LoadPanel (0, "ECoolInterlockClient.uir", BlockPanel)) < 0 ) return dacPanelHandles[i];
		sprintf(title, "DQ block %d", i);
		SetPanelAttribute(dacPanelHandles[i], ATTR_TITLE, title);
	}
	
	return 0;
}


int createMainPanelGui(void) {
	int i, yPos, xPos, diagIndexShift, windowWidth;
	char text[256];

	yPos = 5;
	xPos = 5;
	diagIndexShift = 0;
	
	windowWidth = 10 + BLOCK_BUTTON_WIDTH + DIAGNOSTICS_LED_WIDTH + 2;
	
	// Help button
	helpBtnHandle = createMainPanelButton(xPos, yPos, windowWidth - 10, BLOCK_BUTTON_HEIGHT, "Цветовые обозначения", VAL_WHITE);
	yPos += BLOCK_BUTTON_HEIGHT + 5; 
	
	// Event window button
	eventBtnHandle = createMainPanelButton(xPos, yPos, windowWidth - 10, BLOCK_BUTTON_HEIGHT, "Просмотр событий", VAL_WHITE);
	yPos += BLOCK_BUTTON_HEIGHT + 5; 
	
	// Client-Server connection indicator
	clientToServerConnectionLED = NewCtrl(mainPanelHandle, CTRL_SQUARE_LED_LS, "Clinet-Server: offline", yPos, xPos); 
	SetCtrlAttribute(mainPanelHandle, clientToServerConnectionLED, ATTR_OFF_COLOR, VAL_RED);
	SetCtrlAttribute(mainPanelHandle, clientToServerConnectionLED, ATTR_ON_COLOR, VAL_GREEN); 
	SetCtrlAttribute(mainPanelHandle, clientToServerConnectionLED, ATTR_LABEL_TOP, yPos + 3);  
	SetCtrlAttribute(mainPanelHandle, clientToServerConnectionLED, ATTR_LABEL_LEFT, 45);
	
	yPos += 23;
	
	// Server-Interlock connection indicator
	serverToHardwareConnectionLED = NewCtrl(mainPanelHandle, CTRL_SQUARE_LED_LS, "Interlock (Modbus): offline", yPos, xPos); 
	SetCtrlAttribute(mainPanelHandle, serverToHardwareConnectionLED, ATTR_OFF_COLOR, VAL_RED);
	SetCtrlAttribute(mainPanelHandle, serverToHardwareConnectionLED, ATTR_ON_COLOR, VAL_GREEN); 
	SetCtrlAttribute(mainPanelHandle, serverToHardwareConnectionLED, ATTR_LABEL_TOP, yPos + 3);  
	SetCtrlAttribute(mainPanelHandle, serverToHardwareConnectionLED, ATTR_LABEL_LEFT, 45);
	
	yPos += 35;
	
	// DI blocks call buttons
	for (i=0; i<DI_NUMBER; i++) {
		// Device diagnostics indicator
		diagnosticsLEDs[diagIndexShift] = createDiagnosticsLED(xPos, yPos, DIAGNOSTICS_LED_WIDTH, BLOCK_BUTTON_HEIGHT);

		// buttons style
		sprintf(text,"DI block %d",i); 
		diPanelCallButtons[i] = createMainPanelButton(xPos + DIAGNOSTICS_LED_WIDTH + 2, yPos, BLOCK_BUTTON_WIDTH, BLOCK_BUTTON_HEIGHT, text, BTN_GRAY_COLOR);
		
		yPos += BLOCK_BUTTON_HEIGHT;
		diagIndexShift += 1;
		
	}
	
	// Empty space for dividing different blocks groups
	yPos += BLOCK_GROUPS_SPACE;
	
	// DQ blocks call buttons
	for (i=0; i<DQ_NUMBER; i++) {
		// Device diagnostics indicator
		diagnosticsLEDs[diagIndexShift] = createDiagnosticsLED(xPos, yPos, DIAGNOSTICS_LED_WIDTH, BLOCK_BUTTON_HEIGHT);
		
		// buttons style
		sprintf(text,"DQ block %d",i); 
		dqPanelCallButtons[i] = createMainPanelButton(xPos + DIAGNOSTICS_LED_WIDTH + 2, yPos, BLOCK_BUTTON_WIDTH, BLOCK_BUTTON_HEIGHT, text, BTN_GRAY_COLOR); 
		
		yPos += BLOCK_BUTTON_HEIGHT;
		diagIndexShift += 1;   
	} 
	
	// Empty space for dividing different blocks groups
	yPos += BLOCK_GROUPS_SPACE; 
	
	// ADC blocks call buttons
	for (i=0; i<ADC_NUMBER; i++) {
		// Device diagnostics indicator
		diagnosticsLEDs[diagIndexShift] = createDiagnosticsLED(xPos, yPos, DIAGNOSTICS_LED_WIDTH, BLOCK_BUTTON_HEIGHT); 

		// buttons style
		sprintf(text,"ADC %d",i); 
		adcPanelCallButtons[i] = createMainPanelButton(xPos + DIAGNOSTICS_LED_WIDTH + 2, yPos, BLOCK_BUTTON_WIDTH, BLOCK_BUTTON_HEIGHT, text, BTN_LIGHT_BLUE_COLOR); 
		
		yPos += BLOCK_BUTTON_HEIGHT;
		diagIndexShift += 1;
		
	}
	
	// Empty space for dividing different blocks groups
	yPos += BLOCK_GROUPS_SPACE ;
	
	// DAC blocks call buttons
	for (i=0; i<DAC_NUMBER; i++) {
		// Device diagnostics indicator
		diagnosticsLEDs[diagIndexShift] = createDiagnosticsLED(xPos, yPos, DIAGNOSTICS_LED_WIDTH, BLOCK_BUTTON_HEIGHT);

		// buttons style
		sprintf(text,"DAC %d",i); 
		dacPanelCallButtons[i] = createMainPanelButton(xPos + DIAGNOSTICS_LED_WIDTH + 2, yPos, BLOCK_BUTTON_WIDTH, BLOCK_BUTTON_HEIGHT, text, BTN_LIGHT_BLUE_COLOR); 
		
		yPos += BLOCK_BUTTON_HEIGHT;
		diagIndexShift += 1;
	}

	// Update the size of the main panel
	SetPanelAttribute(mainPanelHandle, ATTR_HEIGHT, yPos + 5);
	SetPanelAttribute(mainPanelHandle, ATTR_WIDTH, windowWidth);
	
	return 0;
}


int CVICALLBACK mainMenuButtonCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	
	int i;

	switch (event)
	{
		case EVENT_COMMIT:
			if (control == helpBtnHandle) {
				DisplayPanel(helpPanelHandle);
				break;
			}
			
			if (control == eventBtnHandle) {
				DisplayPanel(eventPanelHandle);
				break;
			}
			
			// Search for DI block button
			for (i=0; i<DI_NUMBER; i++) {
				if (diPanelCallButtons[i] == control) {
					DisplayPanel(diPanelHandles[i]);
					break;
				}	
			}
			
			// Search for DQ block button
			for (i=0; i<DQ_NUMBER; i++) {
				if (dqPanelCallButtons[i] == control) {
					DisplayPanel(dqPanelHandles[i]);
					break;
				}	
			}
			
			// Search for ADC block button
			for (i=0; i<ADC_NUMBER; i++) {
				if (adcPanelCallButtons[i] == control) {
					DisplayPanel(adcPanelHandles[i]);
					break;
				}	
			}
			
			// Search for DAC block button
			for (i=0; i<DAC_NUMBER; i++) {
				if (dacPanelCallButtons[i] == control) {
					DisplayPanel(dacPanelHandles[i]);
					break;
				}	
			}
			
			break;
	}
	return 0;
}


int setupBlocksPanels(void) {
	int i, j, yPos;
	
	// Setup DI blocks panels
	for (i=0; i<DI_NUMBER; i++) {
		yPos = 5;
		for (j=0; j<CHANNELS_PER_DI; j++) {
			diIndicatorsHandles[i][j] = NewCtrl(diPanelHandles[i], CTRL_ROUND_LED_LS, WAITING_INFO, yPos, 5);	
			SetCtrlAttribute(diPanelHandles[i], diIndicatorsHandles[i][j], ATTR_LABEL_TOP, yPos);  
			SetCtrlAttribute(diPanelHandles[i], diIndicatorsHandles[i][j], ATTR_LABEL_LEFT, 30); 
			
			if (CFG_DI_MASKS[i] & (1 << j)) {
				SetCtrlAttribute(diPanelHandles[i], diIndicatorsHandles[i][j], ATTR_OFF_COLOR, LED_RED_COLOR);
				SetCtrlAttribute(diPanelHandles[i], diIndicatorsHandles[i][j], ATTR_ON_COLOR, LED_GREEN_COLOR);	
			} else {
				SetCtrlAttribute(diPanelHandles[i], diIndicatorsHandles[i][j], ATTR_OFF_COLOR, VAL_BLACK);
				SetCtrlAttribute(diPanelHandles[i], diIndicatorsHandles[i][j], ATTR_ON_COLOR, LED_YELLOW_COLOR);
			}
			
			
			yPos += DI_ITEM_STEP;
		}
		SetPanelAttribute(diPanelHandles[i], ATTR_HEIGHT, yPos + 5);
		SetPanelAttribute(diPanelHandles[i], ATTR_WIDTH, 250); 
	}
	
	// Setup DQ blocks panels
	for (i=0; i<DQ_NUMBER; i++) {
		yPos = 5;
		for (j=0; j<CHANNELS_PER_DQ_ACTUAL; j++) {
			dqIndicatorsHandles[i][j] = NewCtrl(dqPanelHandles[i], CTRL_ROUND_LED_LS, WAITING_INFO, yPos, 5);	
			SetCtrlAttribute(dqPanelHandles[i], dqIndicatorsHandles[i][j], ATTR_LABEL_TOP, yPos);  
			SetCtrlAttribute(dqPanelHandles[i], dqIndicatorsHandles[i][j], ATTR_LABEL_LEFT, 30); 
			SetCtrlAttribute(dqPanelHandles[i], dqIndicatorsHandles[i][j], ATTR_OFF_COLOR, LED_RED_COLOR);
			SetCtrlAttribute(dqPanelHandles[i], dqIndicatorsHandles[i][j], ATTR_ON_COLOR, LED_GREEN_COLOR);
			
			if (CFG_DQ_MASKS[i] & (1 << (j * 2))) {
				SetCtrlAttribute(dqPanelHandles[i], dqIndicatorsHandles[i][j], ATTR_OFF_COLOR, LED_RED_COLOR);
				SetCtrlAttribute(dqPanelHandles[i], dqIndicatorsHandles[i][j], ATTR_ON_COLOR, LED_GREEN_COLOR);	
			} else {
				SetCtrlAttribute(dqPanelHandles[i], dqIndicatorsHandles[i][j], ATTR_OFF_COLOR, VAL_BLACK);
				SetCtrlAttribute(dqPanelHandles[i], dqIndicatorsHandles[i][j], ATTR_ON_COLOR, LED_YELLOW_COLOR);
			}
			yPos += DI_ITEM_STEP;
		}
		SetPanelAttribute(dqPanelHandles[i], ATTR_HEIGHT, yPos + 5);
		SetPanelAttribute(dqPanelHandles[i], ATTR_WIDTH, 250); 
	}
	
	// Setup ADC blocks panels
	for (i=0; i<ADC_NUMBER; i++) {
		yPos = 5; 
		for (j=0; j<CHANNELS_PER_ADC; j++) {
			adcGraphPanels[i][j] = -1;	
			adcGraphCallButtons[i][j] = NewCtrl(adcPanelHandles[i], CTRL_SQUARE_COMMAND_BUTTON_LS, "Gr", yPos, 5);
			SetCtrlAttribute(adcPanelHandles[i], adcGraphCallButtons[i][j], ATTR_HEIGHT, ADC_BUTTON_HEIGHT);
			SetCtrlAttribute(adcPanelHandles[i], adcGraphCallButtons[i][j], ATTR_WIDTH, ADC_BUTTON_WIDTH); 
			SetCtrlAttribute(adcPanelHandles[i], adcGraphCallButtons[i][j], ATTR_CMD_BUTTON_COLOR, MakeColor(239,235,250));
			// Labels
			adcFieldsHandles[i][j] = NewCtrl(adcPanelHandles[i],CTRL_NUMERIC, WAITING_INFO, yPos, 5 + ADC_ITEM_STEP);
			
			SetCtrlAttribute(adcPanelHandles[i], adcFieldsHandles[i][j], ATTR_WIDTH, ADC_FIELD_WIDTH);  
			SetCtrlAttribute(adcPanelHandles[i], adcFieldsHandles[i][j], ATTR_CTRL_MODE, VAL_INDICATOR); 
			SetCtrlAttribute(adcPanelHandles[i], adcFieldsHandles[i][j], ATTR_PRECISION, 4);  
			SetCtrlAttribute(adcPanelHandles[i], adcFieldsHandles[i][j], ATTR_LABEL_TOP, yPos);  
			SetCtrlAttribute(adcPanelHandles[i], adcFieldsHandles[i][j], ATTR_LABEL_LEFT, 5 + ADC_ITEM_STEP + ADC_FIELD_WIDTH + 2); 
			// Callback
			InstallCtrlCallback(adcPanelHandles[i], adcGraphCallButtons[i][j], adcGraphButtonCallback, NULL);
			
			yPos += ADC_ITEM_STEP;
		}
		SetPanelAttribute(adcPanelHandles[i], ATTR_HEIGHT, yPos + 5);
		SetPanelAttribute(adcPanelHandles[i], ATTR_WIDTH, 300); 
	}
	
	// Setup DAC blocks panels
	for (i=0; i<DAC_NUMBER; i++) {
		yPos = 5; 
		for (j=0; j<CHANNELS_PER_DAC; j++) {
			dacFieldsHandles[i][j] = NewCtrl(dacPanelHandles[i], CTRL_NUMERIC, WAITING_INFO, yPos, 5);
			
			SetCtrlAttribute(dacPanelHandles[i], dacFieldsHandles[i][j], ATTR_WIDTH, ADC_FIELD_WIDTH);  
			SetCtrlAttribute(dacPanelHandles[i], dacFieldsHandles[i][j], ATTR_CTRL_MODE, VAL_INDICATOR); 
			SetCtrlAttribute(dacPanelHandles[i], dacFieldsHandles[i][j], ATTR_PRECISION, 4);  
			SetCtrlAttribute(dacPanelHandles[i], dacFieldsHandles[i][j], ATTR_LABEL_TOP, yPos);  
			SetCtrlAttribute(dacPanelHandles[i], dacFieldsHandles[i][j], ATTR_LABEL_LEFT, 5 + ADC_FIELD_WIDTH + 2); 
			
			yPos += ADC_ITEM_STEP;
		}
		SetPanelAttribute(dacPanelHandles[i], ATTR_HEIGHT, yPos + 5);
		SetPanelAttribute(dacPanelHandles[i], ATTR_WIDTH, 300); 
	}
	
	return 0;
}


int setupEventsPanel(void) {
	int i, j, yPos, xPos;
	char sectionTitle[256];
	
	// Setup DI blocks panels
	xPos = 240;

	for (i=0; i<DI_NUMBER; i++) {
		yPos = 5;
		sprintf(sectionTitle, "DI block %d", i);
		NewCtrl(eventPanelHandle, CTRL_TEXT_MSG, sectionTitle, yPos, xPos);
		yPos += 20;

		for (j=0; j<CHANNELS_PER_DI; j++) {
			diEventsIndicatorsHandles[i][j] = NewCtrl(eventPanelHandle, CTRL_ROUND_LED_LS, WAITING_INFO, yPos, xPos);	
			SetCtrlAttribute(eventPanelHandle, diEventsIndicatorsHandles[i][j], ATTR_LABEL_TOP, yPos);  
			SetCtrlAttribute(eventPanelHandle, diEventsIndicatorsHandles[i][j], ATTR_LABEL_LEFT, xPos + 25); 
			
			if (CFG_DI_MASKS[i] & (1 << j)) {
				SetCtrlAttribute(eventPanelHandle, diEventsIndicatorsHandles[i][j], ATTR_OFF_COLOR, LED_RED_COLOR);
				SetCtrlAttribute(eventPanelHandle, diEventsIndicatorsHandles[i][j], ATTR_ON_COLOR, LED_GREEN_COLOR);	
			} else {
				SetCtrlAttribute(eventPanelHandle, diEventsIndicatorsHandles[i][j], ATTR_OFF_COLOR, VAL_BLACK);
				SetCtrlAttribute(eventPanelHandle, diEventsIndicatorsHandles[i][j], ATTR_ON_COLOR, LED_YELLOW_COLOR);
			}
			
			
			yPos += DI_ITEM_STEP;
		}
		xPos += 250 + 8;

	}
	
	// Setup DQ blocks panels
	yPos = 5;
	for (i=0; i<DQ_NUMBER; i++) {
		sprintf(sectionTitle, "DQ block %d", i);
		NewCtrl(eventPanelHandle, CTRL_TEXT_MSG, sectionTitle, yPos, xPos);
		yPos += 20;
		
		for (j=0; j<CHANNELS_PER_DQ_ACTUAL; j++) {
			dqEventsIndicatorsHandles[i][j] = NewCtrl(eventPanelHandle, CTRL_ROUND_LED_LS, WAITING_INFO, yPos, xPos);	
			SetCtrlAttribute(eventPanelHandle, dqEventsIndicatorsHandles[i][j], ATTR_LABEL_TOP, yPos);  
			SetCtrlAttribute(eventPanelHandle, dqEventsIndicatorsHandles[i][j], ATTR_LABEL_LEFT, xPos + 25); 
			SetCtrlAttribute(eventPanelHandle, dqEventsIndicatorsHandles[i][j], ATTR_OFF_COLOR, LED_RED_COLOR);
			SetCtrlAttribute(eventPanelHandle, dqEventsIndicatorsHandles[i][j], ATTR_ON_COLOR, LED_GREEN_COLOR);
			
			if (CFG_DQ_MASKS[i] & (1 << (j * 2))) {
				SetCtrlAttribute(eventPanelHandle, dqEventsIndicatorsHandles[i][j], ATTR_OFF_COLOR, LED_RED_COLOR);
				SetCtrlAttribute(eventPanelHandle, dqEventsIndicatorsHandles[i][j], ATTR_ON_COLOR, LED_GREEN_COLOR);	
			} else {
				SetCtrlAttribute(eventPanelHandle, dqEventsIndicatorsHandles[i][j], ATTR_OFF_COLOR, VAL_BLACK);
				SetCtrlAttribute(eventPanelHandle, dqEventsIndicatorsHandles[i][j], ATTR_ON_COLOR, LED_YELLOW_COLOR);
			}
			yPos += DI_ITEM_STEP;
		}
		yPos += DI_ITEM_STEP;

	}

	SetPanelAttribute(eventPanelHandle, ATTR_WIDTH, 240 + (250 + 8) * 3); 
	
	UpdateTheEventsIndicators(-1);
	
	return 0;
}


int CVICALLBACK adcGraphButtonCallback (int panel, int control, int event, void *callbackData, int eventData1, int eventData2) {
	int i,j;
	char graphTitle[256];
	
	switch (event) {
		case EVENT_COMMIT:
			for (i=0; i<ADC_NUMBER; i++) {
				if (adcPanelHandles[i] == panel) break;	
			}

			if (i >= ADC_NUMBER) break;
			
			for (j=0; j<CHANNELS_PER_ADC; j++) {
				if (adcGraphCallButtons[i][j] == control) {
					if (adcGraphPanels[i][j] < 0) {
						adcGraphPanels[i][j] = LoadPanel(0, "ECoolInterlockClient.uir", Graph);
						if (adcGraphPanels[i][j] < 0) break;
						sprintf(graphTitle, "ADC %d - channel %d: %s", i+1, j, adcTextLabels[i][j]);
						SetPanelAttribute(adcGraphPanels[i][j], ATTR_TITLE, graphTitle);
						DisplayPanel(adcGraphPanels[i][j]);
					}
					DisplayPanel(adcGraphPanels[i][j]);
					break;
				}
			}
			break;
	}
	return 0;
}


int SetDiNames(char *bar_separated_names) {
	char *deviceNamesStartPos[DI_NUMBER];
	char names_data[8192];
	char *pos, *namePos;
	int i, j;
	
	strcpy(names_data, bar_separated_names);
	
	if ((pos = strstr(names_data, "\n")) != NULL) pos[0] = 0;
	
	deviceNamesStartPos[0] = names_data;
	for (i=1; i<DI_NUMBER; i++) {
		pos = strstr(deviceNamesStartPos[i - 1], "||");
		if (!pos) {
			msAddMsg(msGMS(), "%s Error! Unable to parse DI blocks names. Some of the dividers are missing ('||') (between block %d and %d).\n", TimeStamp(0), i-1, i);
			return -1;
		}
		
		deviceNamesStartPos[i] = pos + 2;  // after double tabulation
		pos[0] = 0;  // Each device names data will be zero-ending string
	}
	
	// Set names of each DI devices
	
	for (i=0; i<DI_NUMBER; i++) {
		namePos = deviceNamesStartPos[i];
		for (j=0; j<CHANNELS_PER_DI; j++) {
			if (j < CHANNELS_PER_DI - 1) {
				pos = strstr(namePos, "|");

				if (!pos) {
					msAddMsg(msGMS(), "%s Error! Unable to parse DI blocks names. Some of the dividers are missing ('|') (DI block %d, between channels %d and %d).\n", TimeStamp(0), i, j, j+1); 
					return -1;
				}
				pos[0] = 0;
			}

			strcpy(diTextLabels[i][j], namePos);
			SetCtrlAttribute(diPanelHandles[i], diIndicatorsHandles[i][j], ATTR_LABEL_TEXT, diTextLabels[i][j]);
			SetCtrlAttribute(eventPanelHandle, diEventsIndicatorsHandles[i][j], ATTR_LABEL_TEXT, diTextLabels[i][j]); 
			
			namePos = pos + 1; 
		}
	}
	
	return 0;
}


int SetDqNames(char *bar_separated_names) {
	char *deviceNamesStartPos[DQ_NUMBER];
	char names_data[8192];
	char *pos, *namePos;
	int i, j;
	
	strcpy(names_data, bar_separated_names);
	if ((pos = strstr(names_data, "\n")) != NULL) pos[0] = 0;
	
	deviceNamesStartPos[0] = names_data;
	for (i=1; i<DQ_NUMBER; i++) {
		pos = strstr(deviceNamesStartPos[i - 1], "||");
		if (!pos) {
			msAddMsg(msGMS(), "%s Error! Unable to parse DQ blocks names. Some of the dividers are missing ('||') (between block %d and %d).\n", TimeStamp(0), i-1, i);
			return -1;
		}
		
		deviceNamesStartPos[i] = pos + 2;  // after double tabulation
		pos[0] = 0;  // Each device names data will be zero-ending string
	}
	
	// Set names of each DQ devices
	
	for (i=0; i<DQ_NUMBER; i++) {
		namePos = deviceNamesStartPos[i];
		for (j=0; j<CHANNELS_PER_DQ; j++) {
			if (j < CHANNELS_PER_DQ - 1) {
				pos = strstr(namePos, "|");

				if (!pos) {
					msAddMsg(msGMS(), "%s Error! Unable to parse DQ blocks names. Some of the dividers are missing ('|') (DQ block %d, between channels %d and %d).\n", TimeStamp(0), i, j, j+1); 
					return -1;
				};
				pos[0] = 0;
			}

			if (j % 2 == 0) {
				strcpy(dqTextLabels[i][j / 2], namePos);
				SetCtrlAttribute(dqPanelHandles[i], dqIndicatorsHandles[i][j / 2], ATTR_LABEL_TEXT, dqTextLabels[i][j / 2]);
				SetCtrlAttribute(eventPanelHandle, dqEventsIndicatorsHandles[i][j / 2], ATTR_LABEL_TEXT, dqTextLabels[i][j / 2]);
			}
			
			namePos = pos + 1; 
		}
	}
	
	return 0;	
}


int SetAdcNames(int adcIndex, char *bar_separated_names) {
	char names_data[8192];
	char *pos, *namePos;
	int i;

	if (adcIndex < 0 || adcIndex >= ADC_NUMBER) return -1;
	
	strcpy(names_data, bar_separated_names);
	if ((pos = strstr(names_data, "\n")) != NULL) pos[0] = 0;
	namePos = names_data; 
	
	for (i=0; i<CHANNELS_PER_ADC; i++) {
		if (i < CHANNELS_PER_ADC - 1) {
			pos = strstr(namePos, "|");

			if (!pos) {
				msAddMsg(msGMS(), "%s Error! Unable to parse ADC channels names. Some of the dividers are missing ('|') (ADC %d, between channels %d and %d).\n", TimeStamp(0), adcIndex, i, i+1);
				return -1;
			}
			pos[0] = 0;
		}

		strcpy(adcTextLabels[adcIndex][i], namePos);
		SetCtrlAttribute(adcPanelHandles[adcIndex], adcFieldsHandles[adcIndex][i], ATTR_LABEL_TEXT, adcTextLabels[adcIndex][i]);
		
		namePos = pos + 1; 
	}
	
	return 0;
}


int SetDacNames(int dacIndex, char *bar_separated_names) {
	char names_data[8192];
	char *pos, *namePos;
	int i;

	if (dacIndex < 0 || dacIndex >= DAC_NUMBER) return -1;
	
	strcpy(names_data, bar_separated_names); 
	if ((pos = strstr(names_data, "\n")) != NULL) pos[0] = 0;
	namePos = names_data; 
	
	for (i=0; i<CHANNELS_PER_DAC; i++) {
		if (i < CHANNELS_PER_DAC - 1) {
			pos = strstr(namePos, "|");

			if (!pos) {
				msAddMsg(msGMS(), "%s Error! Unable to parse DAC channels names. Some of the dividers are missing ('|') (DAC %d, between channels %d and %d).\n", TimeStamp(0), dacIndex, i, i+1);
				return -1;
			}
			pos[0] = 0;
		}

		strcpy(dacTextLabels[dacIndex][i], namePos);
		SetCtrlAttribute(dacPanelHandles[dacIndex], dacFieldsHandles[dacIndex][i], ATTR_LABEL_TEXT, dacTextLabels[dacIndex][i]);
		
		namePos = pos + 1; 
	}
	
	return 0;	
}


void UpdateAllValues(void) {
	int i, j;
	
	// Update DI values
	for (i=0; i<DI_NUMBER; i++) {
		for(j=0; j<CHANNELS_PER_DI; j++) {
			// SetCtrlVal(diPanelHandles[i], diIndicatorsHandles[i][j], DI_VALUES[i] & (1 << (CHANNELS_PER_DI - j - 1)));
			SetCtrlVal(diPanelHandles[i], diIndicatorsHandles[i][j], DI_VALUES[i] & (1 << j)); 
		}
		
		if (DI_VALUES[i] == CFG_DI_MASKS[i]) {
			if (CFG_DI_MASKS[i] == 0)
				SetCtrlAttribute(mainPanelHandle, diPanelCallButtons[i], ATTR_CMD_BUTTON_COLOR, BTN_GRAY_COLOR);
			else
				SetCtrlAttribute(mainPanelHandle, diPanelCallButtons[i], ATTR_CMD_BUTTON_COLOR, BTN_GREEN_COLOR); 
	
		} else if ((DI_VALUES[i] & CFG_DI_MASKS[i]) == CFG_DI_MASKS[i]) {
			SetCtrlAttribute(mainPanelHandle, diPanelCallButtons[i], ATTR_CMD_BUTTON_COLOR, BTN_YELLOW_COLOR);	
		} else {
			SetCtrlAttribute(mainPanelHandle, diPanelCallButtons[i], ATTR_CMD_BUTTON_COLOR, BTN_RED_COLOR); 	
		}
		
	}
	
	// Update DQ values
	for (i=0; i<DQ_NUMBER; i++) {
		for(j=0; j<CHANNELS_PER_DQ_ACTUAL; j++) {
			// SetCtrlVal(dqPanelHandles[i], dqIndicatorsHandles[i][j], DQ_VALUES[i] & (1 << (CHANNELS_PER_DQ - j - 1)));
			SetCtrlVal(dqPanelHandles[i], dqIndicatorsHandles[i][j], DQ_VALUES[i] & (1 << (j * 2)));
		}
		
		if (DQ_VALUES[i] == CFG_DQ_MASKS[i]) {
			if (CFG_DQ_MASKS[i] == 0)
				SetCtrlAttribute(mainPanelHandle, dqPanelCallButtons[i], ATTR_CMD_BUTTON_COLOR, BTN_GRAY_COLOR);
			else
				SetCtrlAttribute(mainPanelHandle, dqPanelCallButtons[i], ATTR_CMD_BUTTON_COLOR, BTN_GREEN_COLOR);  

		} else if ((DQ_VALUES[i] & CFG_DQ_MASKS[i]) == CFG_DQ_MASKS[i]) {
			SetCtrlAttribute(mainPanelHandle, dqPanelCallButtons[i], ATTR_CMD_BUTTON_COLOR, BTN_YELLOW_COLOR);	
		} else {
			SetCtrlAttribute(mainPanelHandle, dqPanelCallButtons[i], ATTR_CMD_BUTTON_COLOR, BTN_RED_COLOR); 	
		}
	}
	
	// Update ADC values
	for (i=0; i<ADC_NUMBER; i++) {
		for(j=0; j<CHANNELS_PER_ADC; j++) {
			SetCtrlVal(adcPanelHandles[i], adcFieldsHandles[i][j], ADC_VALUES[i][j]);
		}
	}
	
	// Update DAC values
	for (i=0; i<DAC_NUMBER; i++) {
		for(j=0; j<CHANNELS_PER_DAC; j++) {
			SetCtrlVal(dacPanelHandles[i], dacFieldsHandles[i][j], DAC_VALUES[i][j]);
		}
	}
	
	// Update diagnostics LEDs
	for (i=0; i<10; i++) {
		SetCtrlVal(mainPanelHandle, diagnosticsLEDs[i], DEVICES_DIAGNOSTICS & (1 << (15 - i)));	
	}
	return;
}


void UpdateAdcGraphs(void) {
	int i, j;
	for (i=0; i<ADC_NUMBER; i++) {
		for (j=0; j<CHANNELS_PER_ADC; j++) {
			if (adcGraphPanels[i][j] < 0) continue;
			
			PlotStripChartPoint(adcGraphPanels[i][j], Graph_GRAPH, ADC_VALUES[i][j]);
			SetCtrlVal(adcGraphPanels[i][j],Graph_currentValue, ADC_VALUES[i][j]);
		}
	}
}


void UpdateConnectionStateIndicator(void) {
	SetCtrlVal(mainPanelHandle, serverToHardwareConnectionLED, SERVER_HARDWARE_CONNECTED);
	if (SERVER_HARDWARE_CONNECTED)
		SetCtrlAttribute(mainPanelHandle, serverToHardwareConnectionLED, ATTR_LABEL_TEXT, "Interlock (Modbus): online");
	else
		SetCtrlAttribute(mainPanelHandle, serverToHardwareConnectionLED, ATTR_LABEL_TEXT, "Interlock (Modbus): offline");
}


int CVICALLBACK _toCur (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	unsigned int y, m, d, _h, _m, _s;   
	switch (event)
	{
		case EVENT_COMMIT:
			GetSystemDate(&m, &d, &y);
			GetSystemTime(&_h, &_m, &_s);
			switch(control) {
				case eventPanel_cur1:
					SetCtrlVal(panel, eventPanel_fromYear, y);
					SetCtrlVal(panel, eventPanel_fromMonth, m);
					SetCtrlVal(panel, eventPanel_fromDay, d);
					SetCtrlVal(panel, eventPanel_fromHour, _h);
					SetCtrlVal(panel, eventPanel_fromMinute, _m);
					SetCtrlVal(panel, eventPanel_fromSecond, _s);
					break;
				case eventPanel_cur2:
					SetCtrlVal(panel, eventPanel_toYear, y);
					SetCtrlVal(panel, eventPanel_toMonth, m);
					SetCtrlVal(panel, eventPanel_toDay, d);
					SetCtrlVal(panel, eventPanel_toHour, _h);
					SetCtrlVal(panel, eventPanel_toMinute, _m);
					SetCtrlVal(panel, eventPanel_toSecond, _s);
					break;
			}
			break;
	}
	return 0;
}


int  CVICALLBACK requestEvents(int panel, int control, int event, void *callbackData, int eventData1, int eventData2) {
	char command[128];

	switch (event)
	{
		case EVENT_COMMIT:
			sprintf(command, "%s %u %u\n", CMD_GET_EVENTS, getFromTimeStamp(), getToTimeStamp());
			appendGlobalRequestQueueRecord(CMD_GET_EVENTS_ID, command, NULL);
			break;
	}
	return 0;		
}


int  CVICALLBACK clearEventsList(int panel, int control, int event, void *callbackData, int eventData1, int eventData2) {
	char command[128];

	switch (event)
	{
		case EVENT_COMMIT:
			ClearListCtrl(eventPanelHandle, eventPanel_LISTBOX);  
			UpdateTheEventsIndicators(-1);
			break;
	}
	return 0;		
}


int CVICALLBACK selectEvent (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	char command[128];
	int eventIndex;
	
	switch (event)
	{
		case EVENT_COMMIT:
			if (INTERLOCK_EVENTS_LIST.eventsNumber == 0) return 0;

			eventIndex = eventData1;
			
			if (eventIndex < INTERLOCK_EVENTS_LIST.eventsNumber) {
				UpdateTheEventsIndicators(eventIndex);
				
			} else if (eventIndex == INTERLOCK_EVENTS_LIST.eventsNumber) {
				sprintf(
					command,
					"%s %u %u\n",
					CMD_GET_EVENTS,
					INTERLOCK_EVENTS_LIST.events[INTERLOCK_EVENTS_LIST.eventsNumber - 1].timeStamp,
					getToTimeStamp()
				);
				appendGlobalRequestQueueRecord(CMD_GET_EVENTS_ID, command, NULL);

			} else {
				MessagePopup("Selected index is out of range", "How the f**k did you do this?");	
			}

			break;
	}
	return 0;
}


void PrintTheEventsList(void) {
	int i;
	
	ClearListCtrl(eventPanelHandle, eventPanel_LISTBOX);
	
	if (INTERLOCK_EVENTS_LIST.eventsNumber == 0) {
	
		InsertListItem(eventPanelHandle, eventPanel_LISTBOX, 0, "No events available.", 0); 
		UpdateTheEventsIndicators(-1);
		return;
	}

	for (i=0; i<INTERLOCK_EVENTS_LIST.eventsNumber; i++) {
		InsertListItem(eventPanelHandle, eventPanel_LISTBOX, i, INTERLOCK_EVENTS_LIST.events[i].textTimeInfo, 0);		
	}
	
	if (INTERLOCK_EVENTS_LIST.moreAvailable) {
		InsertListItem(eventPanelHandle, eventPanel_LISTBOX, INTERLOCK_EVENTS_LIST.eventsNumber, "More available ...", 0);	
	}
}



void UpdateTheEventsIndicators(int eventId) {
	int i, j;

	if (eventId < 0) {
		// No events
		// Set all indicators to zero and make dimmed
	
		// Update DI values
		for (i=0; i<DI_NUMBER; i++) {
			for(j=0; j<CHANNELS_PER_DI; j++) {
				SetCtrlVal(eventPanelHandle, diEventsIndicatorsHandles[i][j], 0);
				SetCtrlAttribute(eventPanelHandle, diEventsIndicatorsHandles[i][j], ATTR_DIMMED, 1);
			}
		}
	
		// Update DQ values
		for (i=0; i<DQ_NUMBER; i++) {
			for(j=0; j<CHANNELS_PER_DQ_ACTUAL; j++) {;
				SetCtrlVal(eventPanelHandle, dqEventsIndicatorsHandles[i][j], 0);
				SetCtrlAttribute(eventPanelHandle, dqEventsIndicatorsHandles[i][j], ATTR_DIMMED, 1);
			}
		}

	} else {
		// Update DI values
		for (i=0; i<DI_NUMBER; i++) {
			for(j=0; j<CHANNELS_PER_DI; j++) {
				SetCtrlVal(eventPanelHandle, diEventsIndicatorsHandles[i][j], INTERLOCK_EVENTS_LIST.events[eventId].DI_VALUES[i] & (1 << j));
				SetCtrlAttribute(eventPanelHandle, diEventsIndicatorsHandles[i][j], ATTR_DIMMED, 0); 
			}
		}
	
		// Update DQ values
		for (i=0; i<DQ_NUMBER; i++) {
			for(j=0; j<CHANNELS_PER_DQ_ACTUAL; j++) {
				SetCtrlVal(eventPanelHandle, dqEventsIndicatorsHandles[i][j], INTERLOCK_EVENTS_LIST.events[eventId].DQ_VALUES[i] & (1 << (j * 2)));
				SetCtrlAttribute(eventPanelHandle, dqEventsIndicatorsHandles[i][j], ATTR_DIMMED, 0); 
			}
		}	
	}
}
