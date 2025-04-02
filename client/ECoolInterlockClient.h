/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2021. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  BlockMenu                        1       /* callback function: mainPanelCallback */
#define  BlockMenu_TIMER                  2       /* callback function: tick */

#define  BlockPanel                       2       /* callback function: blockPanelCallback */

#define  DESCR                            3       /* callback function: helpPanelCallback */
#define  DESCR_LED_4                      2
#define  DESCR_LED_3                      3
#define  DESCR_LED_2                      4
#define  DESCR_LED                        5
#define  DESCR_COMMANDBUTTON_4            6
#define  DESCR_COMMANDBUTTON_3            7
#define  DESCR_COMMANDBUTTON_2            8
#define  DESCR_COMMANDBUTTON              9
#define  DESCR_TEXTMSG_2                  10
#define  DESCR_TEXTMSG_3                  11
#define  DESCR_TEXTMSG                    12
#define  DESCR_LED_6                      13
#define  DESCR_LED_5                      14

#define  eventPanel                       4       /* callback function: blockPanelCallback */
#define  eventPanel_REPORT_BUTTON_2       2       /* callback function: clearEventsList */
#define  eventPanel_REPORT_BUTTON         3       /* callback function: requestEvents */
#define  eventPanel_toSecond              4
#define  eventPanel_toMinute              5
#define  eventPanel_toHour                6
#define  eventPanel_toDay                 7
#define  eventPanel_toMonth               8
#define  eventPanel_fromSecond            9
#define  eventPanel_fromMinute            10
#define  eventPanel_toYear                11
#define  eventPanel_fromHour              12
#define  eventPanel_fromDay               13
#define  eventPanel_fromMonth             14
#define  eventPanel_cur2                  15      /* callback function: _toCur */
#define  eventPanel_cur1                  16      /* callback function: _toCur */
#define  eventPanel_fromYear              17
#define  eventPanel_TEXTMSG               18
#define  eventPanel_TEXTMSG_2             19
#define  eventPanel_DECORATION_2          20
#define  eventPanel_DECORATION            21
#define  eventPanel_LISTBOX               22      /* callback function: selectEvent */

#define  Graph                            5       /* callback function: graphPanelCallback */
#define  Graph_minValue                   2       /* callback function: graphVerticalRange */
#define  Graph_maxValue                   3       /* callback function: graphVerticalRange */
#define  Graph_currentValue               4
#define  Graph_GRAPH                      5
#define  Graph_ClearPlotBtn               6       /* callback function: clearPlotCallback */
#define  Graph_FitBtn                     7       /* callback function: fitPlotButtonCallback */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK _toCur(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK blockPanelCallback(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clearEventsList(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clearPlotCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK fitPlotButtonCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK graphPanelCallback(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK graphVerticalRange(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK helpPanelCallback(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK mainPanelCallback(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK requestEvents(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK selectEvent(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK tick(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
