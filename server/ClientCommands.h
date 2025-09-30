//==============================================================================
//
// Title:       ClientCommands.h
// Purpose:     A short description of the interface.
//
// Created on:  21.12.2020 at 10:11:35 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __ClientCommands_H__
#define __ClientCommands_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "cvidef.h"
#include "ModBusUbs.h" 

//==============================================================================
// Constants
#define MAX_PARSERS_NUM 128

#define CMD_P_ "INTERLOCK:"
#define CMD_A_ "UBS:"

#define CMD_P_GET_ALLVALUES "INTERLOCK:GET:ALLVALUES"
#define CMD_A_GET_ALLVALUES "UBS:GET:ALLVALUES"

#define CMD_P_GET_DI_VALUES "INTERLOCK:GET:DI:VALUE"
#define CMD_A_GET_DI_VALUES "UBS:GET:DI:VALUE"
#define CMD_P_GET_DI_NAMES  "INTERLOCK:GET:DI:NAMES"
#define CMD_A_GET_DI_NAMES  "UBS:GET:DI:NAMES"
#define CMD_P_GET_DQ_VALUES "INTERLOCK:GET:DQ:VALUES"
#define CMD_A_GET_DQ_VALUES "UBS:GET:DQ:VALUES"
#define CMD_P_GET_DQ_NAMES  "INTERLOCK:GET:DQ:NAMES"
#define CMD_A_GET_DQ_NAMES  "UBS:GET:DQ:NAMES"

#define CMD_P_GET_DIAGNOSTICS "INTERLOCK:GET:DIAGNOSTICS"
#define CMD_A_GET_DIAGNOSTICS "UBS:GET:DIAGNOSTICS"

#define CMD_P_GET_ADC_VALUES "INTERLOCK:GET:ADC:VALUES"
#define CMD_A_GET_ADC_VALUES "UBS:GET:ADC:VALUES"
#define CMD_P_GET_ADC_NAMES "INTERLOCK:GET:ADC:NAMES"
#define CMD_A_GET_ADC_NAMES "UBS:GET:ADC:NAMES"
#define CMD_P_GET_ADC_COEFFICIENTS "INTERLOCK:GET:ADC:COEFFS"
#define CMD_A_GET_ADC_COEFFICIENTS "UBS:GET:ADC:COEFFS"

#define CMD_P_SET_DAC_VALUE "INTERLOCK:SET:DAC:VALUE"
#define CMD_A_SET_DAC_VALUE "UBS:SET:DAC:VALUE"
#define CMD_P_GET_DAC_VALUES "INTERLOCK:GET:DAC:VALUES"
#define CMD_A_GET_DAC_VALUES "UBS:GET:DAC:VALUES"
#define CMD_P_GET_DAC_NAMES "INTERLOCK:GET:DAC:NAMES"
#define CMD_A_GET_DAC_NAMES "UBS:GET:DAC:NAMES"

#define CMD_P_GET_CONNECTION_STATE "INTERLOCK:GET:CONNECTIONSTATE"
#define CMD_A_GET_CONNECTION_STATE "UBS:GET:CONNECTIONSTATE"

#define CMD_P_GET_EVENTS "INTERLOCK:GET:EVENTS"
#define CMD_A_GET_EVENTS "UBS:GET:EVENTS"
		
#define CMD_P_GET_EVENTS_NUM "INTERLOCK:GET:EVENTS:NUM"
#define CMD_P_GET_EVENT_BY_IDX "INTERLOCK:GET:EVENT:BYIDX"
#define CMD_P_CLEAR_EVENTS_BUFFER "INTERLOCK:SET:EVENTS:CLEAR"

//==============================================================================
// Types
typedef int (*parserFunciton)(char *commandBody, char *answerBuffer, char *ip);

//==============================================================================
// External variables

//==============================================================================
// Global functions
void InitCommandParsers(void);
void ReleaseCommandParsers(void);
void registerCommandParser(char * command, char *alias, parserFunciton parser);
parserFunciton getCommandparser(char *command);
void dataExchFunc(unsigned handle, char *ip);  

// Commands parsers (cmdBody, answerBuffer, ip)

int cmdUnknownCommandParser(char *, char *, char *);

int cmdParserGetAllValues(char *, char *, char *);
int cmdParserGetDiValues(char *, char *, char *);
int cmdParserGetDiNames(char *, char *, char *);
int cmdParserGetDqValues(char *, char *, char *);
int cmdParserGetDqNames(char *, char *, char *);

int cmdParserGetDiagnostics(char *, char *, char *);

int cmdParserGetAdcValues(char *, char *, char *);
int cmdParserGetAdcNames(char *, char *, char *);
int cmdParserGetAdcCoefficients(char *, char *, char *);

int cmdParserSetDacValue(char *, char *, char *);
int cmdParserGetDacValues(char *, char *, char *);
int cmdParserGetDacNames(char *, char *, char *);

int cmdParserGetConnectinState(char *, char *, char *);

int cmdParserGetEvents(char *, char *, char *);

int cmdParserGetEventsNum(char *, char *, char *); 
int cmdParserGetEventByIdx(char *, char *, char *); 
int cmdParserClearEventsBuffer(char *, char *, char *); 

void FormatDiNames(unsigned int diIndex, char *outputBuffer);
void FormatAllDiNames(char *outputBuffer);
void FormatDqNames(unsigned int dqIndex, char *outputBuffer);
void FormatAllDqNames(char *outputBuffer);
void FormatAdcNames(unsigned int adcIndex, char *outputBuffer);
void FormatAdcCoefficients(unsigned int adcIndex, char *outputBuffer);
void FormatDacNames(unsigned int dacIndex, char *outputBuffer);


#ifdef __cplusplus
    }
#endif

#endif  /* ndef __ClientCommands_H__ */
