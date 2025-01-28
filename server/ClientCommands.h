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
#define UBS_CMD_GET_ALLVALUES "UBS:GET:ALLVALUES"

#define UBS_CMD_GET_DI_VALUES "UBS:GET:DI:VALUES"
#define UBS_CMD_GET_DI_NAMES  "UBS:GET:DI:NAMES"
#define UBS_CMD_GET_DQ_VALUES "UBS:GET:DQ:VALUES"
#define UBS_CMD_GET_DQ_NAMES  "UBS:GET:DQ:NAMES"

#define UBS_CMD_GET_DIAGNOSTICS "UBS:GET:DIAGNOSTICS"

#define UBS_CMD_GET_ADC_VALUES "UBS:GET:ADC:VALUES"
#define UBS_CMD_GET_ADC_NAMES "UBS:GET:ADC:NAMES"
#define UBS_CMD_GET_ADC_COEFFICIENTS "UBS:GET:ADC:COEFFS"

#define UBS_CMD_SET_DAC_VALUE "UBS:SET:DAC:VALUE"
#define UBS_CMD_GET_DAC_VALUES "UBS:GET:DAC:VALUES"
#define UBS_CMD_GET_DAC_NAMES "UBS:GET:DAC:NAMES"
		
#define UBS_CMD_GET_CONNECTION_STATE "UBS:GET:CONNECTIONSTATE"

#define UBS_CMD_GET_EVENTS "UBS:GET:EVENTS"
//==============================================================================
// Types

//==============================================================================
// External variables

//==============================================================================
// Global functions

void PrepareAnswerForClient(const char * command, const modbus_block_data_t * modbusBlockData, char *answer);

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
