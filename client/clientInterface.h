//==============================================================================
//
// Title:       clientInterface.h
// Purpose:     A short description of the interface.
//
// Created on:  28.09.2015 at 13:08:35 by OEM.
// Copyright:   OEM. All Rights Reserved.
//
//==============================================================================

#ifndef __clientInterface_H__
#define __clientInterface_H__

#include "clientData.h"


#define BTN_RED_COLOR MakeColor(255,145,145)
#define BTN_YELLOW_COLOR MakeColor(255,255,145)  
#define BTN_GREEN_COLOR MakeColor(206,255,156)
#define BTN_GRAY_COLOR MakeColor(190,190,190) 
#define BTN_LIGHT_BLUE_COLOR MakeColor(189,226,253)
#define LED_GREEN_COLOR MakeColor(100,255,100)
#define LED_YELLOW_COLOR MakeColor(255,255,145)  
#define LED_RED_COLOR MakeColor(255,50,50)
//==============================================================================
// Include files

#include "cvidef.h"


//==============================================================================
// Constants
extern int mainPanelHandle, helpPanelHandle, eventPanelHandle;

extern int clientToServerConnectionLED, serverToHardwareConnectionLED;

extern int diPanelHandles[DI_NUMBER], diPanelCallButtons[DI_NUMBER];
extern int dqPanelHandles[DQ_NUMBER], dqPanelCallButtons[DQ_NUMBER];
extern int adcPanelHandles[ADC_NUMBER], adcPanellCallButtons[ADC_NUMBER];
extern int dacPanelHandles[DAC_NUMBER], dacPanellCallButtons[DAC_NUMBER]; 

extern int diagnosticsLEDs[16];

// DI blocks gui
extern int diIndicatorsHandles[DI_NUMBER][CHANNELS_PER_DI];
extern int diEventsIndicatorsHandles[DI_NUMBER][CHANNELS_PER_DI];
extern char diTextLabels[DI_NUMBER][CHANNELS_PER_DI][256];

// DQ blocks gui
extern int dqIndicatorsHandles[DQ_NUMBER][CHANNELS_PER_DQ_ACTUAL];
extern int dqEventsIndicatorsHandles[DQ_NUMBER][CHANNELS_PER_DQ_ACTUAL];
extern char dqTextLabels[DQ_NUMBER][CHANNELS_PER_DQ_ACTUAL][256];

// ADC blocks gui
extern int adcGraphCallButtons[ADC_NUMBER][CHANNELS_PER_ADC];
extern int adcGraphPanels[ADC_NUMBER][CHANNELS_PER_ADC];
extern int adcFieldsHandles[ADC_NUMBER][CHANNELS_PER_ADC];
extern char adcTextLabels[ADC_NUMBER][CHANNELS_PER_ADC][256];
extern double adcGraphRanges[ADC_NUMBER][CHANNELS_PER_ADC][2];

// DAC blocks gui
extern int dacFieldsHandles[DAC_NUMBER][CHANNELS_PER_DAC];
extern char dacTextLabels[DAC_NUMBER][CHANNELS_PER_DAC][256];

//==============================================================================
// Global functions

int CVICALLBACK mainMenuButtonCallback (int panel, int control, int event, void *callbackData, int eventData1, int eventData2);

int clientCallbackFunction(unsigned handle, int xType, int errCode, void * callbackData);

int CVICALLBACK mainSystemCallback (int handle, int control, int event, void *callbackData, 
									int eventData1, int eventData2);

int CVICALLBACK adcGraphButtonCallback (int panel, int control, int event, void *callbackData, int eventData1, int eventData2);

int CreateBlocksGui(void);
int createBlocksPanels(void);
int createMainPanelGui(void);
int setupBlocksPanels(void);
int setupEventsPanel(void);


int SetDiNames(char *bar_separated_names);
int SetDqNames(char *bar_separated_names);
int SetAdcNames(int adcIndex, char *bar_separated_names);
int SetDacNames(int dacIndex, char *bar_separated_names);

void UpdateAllValues(void);
void UpdateAdcGraphs(void);
void UpdateConnectionStateIndicator(void);

void PrintTheEventsList(void);
void UpdateTheEventsIndicators(int eventId);

void UpdateAdcGraphTitle(int adcBlockIndex, int channelIndex);
void ShowAdcGraphWindow(int adcBlockIndex, int channelIndex);
void UpdateAdcGraphPlotRange(int adcBlockIndex, int channelIndex);
void CloseAdcGraphWindow(int adcBlockIndex, int channelIndex);
void PlaceAdcGraphWindow(int adcBlockIndex, int channelIndex, int top, int left, int width, int height);


#endif  /* ndef __clientInterface_H__ */
