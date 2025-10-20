//==============================================================================
//
// 
//==============================================================================

//==============================================================================
// Include files

#include "TCP_Connection.h"

#include <Windows.h>
#include <userint.h>
#include <tcpsupp.h>
#include "MessageStack.h"
#include "TimeMarkers.h" 

// TCP_connection server_interface functions
int tcpConnection_InitServerInterface(tcpConnection_ServerInterface_t * tcpSI)
{
	int i;
	if (!tcpSI) 
	{
		logMessage("[CODE] /-/ tcpConnection_InitServerInterface /-/: Error! Wrong pointer of the server interface.");
		return TCP_CONNECTION_ERROR; 
	}
	tcpSI->server_active = 1;
	tcpSI->clientsNum = 0;
	tcpSI->bgdFuncs = 0;
	tcpSI->dataExchangeFunc = 0;
	for (i=0; i<TCP_CONNECTION_MAX_CLIENTS; i++)
	{
		tcpSI->clients[i] = 0;	
	}
	return 0;
}
int tcpConnection_ClientNumberFromHandle(tcpConnection_ServerInterface_t * tcpSI, unsigned handle)
{
	int i;; 
	if (!tcpSI) 
	{
		logMessage("[CODE] /-/ tcpConnection_ClientNumberFromHandle /-/: Error! Wrong pointer of the server interface.");
		return TCP_CONNECTION_ERROR; 
	}
	for (i=0; i<TCP_CONNECTION_MAX_CLIENTS; i++)
	{
		if (tcpSI->clients[i] == handle)
			return i;
	}
	return -1;
}
int tcpConnection_SetBackgroundFunction(tcpConnection_ServerInterface_t * tcpSI, void (*bgdFunc)(void))
{
	if (!tcpSI) 
	{
		logMessage("[CODE] /-/ tcpConnection_SetBackgroundFunction /-/: Error! Wrong pointer of the server interface.");
		return TCP_CONNECTION_ERROR; 
	}
	tcpSI->bgdFuncs = bgdFunc;
	return 0;
}

int tcpConnection_SetDataExchangeFunction(tcpConnection_ServerInterface_t * tcpSI, void (*dataExchangeFunc)(unsigned handle, char *ip))
{
	if (!tcpSI) 
	{
		logMessage("[CODE] /-/ tcpConnection_SetDataExchangeFunction /-/: Error! Wrong pointer of the server interface.");
		return TCP_CONNECTION_ERROR;
	}
	tcpSI->dataExchangeFunc = dataExchangeFunc;
	return 0;	
}

// TCP_connection server functions
int tcpConnection_ServerCallback(unsigned handle, int xType, int errCode, void * callbackData)
{
	char buf[256];
	int clientNum;
	tcpConnection_ServerInterface_t * tcpSI = (tcpConnection_ServerInterface_t*)callbackData;
	
	GetTCPPeerAddr(handle,buf,256);
	switch(xType)
	{
		case TCP_CONNECT:
			clientNum = tcpConnection_AddClient(tcpSI,handle);
			if (clientNum >= 0)
			{
				logMessage("[CLIENT] A client (%d) [IP: %s] has connected.", clientNum,buf);
			}
			else
			{
				logMessage("[CLIENT] Sending the disconnect request to the client [IP: %s].", buf);   
				DisconnectTCPClient(handle);	
			}
			break;
		case TCP_DISCONNECT:
			clientNum = tcpConnection_DeleteClient(tcpSI,handle);
			if (clientNum >= 0)
			{
				logMessage("[CLIENT] A client (%d) [IP: %s] has disconnected.", clientNum,buf);
			}
			else
			{
				logMessage("[CLIENT] Undefined connection [IP: %s] is closed", buf);	
			}
			//tcpServerInterface->server_active = 0;
			//DisconnectTCPClient(handle);
			break;
		case TCP_DATAREADY:
			if (tcpSI->dataExchangeFunc)
			{
				tcpSI->dataExchangeFunc(handle, buf);	
			}
			break;
	}
	return 0;
}


int tcpConnection_RunServer(int Port, tcpConnection_ServerInterface_t * tcpSI )
{
	int registered;
	static clock_t t1,t2; 
	
	if (!tcpSI) 
	{
		logMessage("[CODE] /-/ tcpConnection_RunServer /-/: Error! Wrong pointer of the server interface.");
		return TCP_CONNECTION_ERROR; 
	}
	
	registered = RegisterTCPServer(Port,tcpConnection_ServerCallback,(void*)tcpSI);
	if ( registered < 0)
	{
		logMessage("[SERVER] Error! Unable to create a TCP server (%d).", Port);
		logMessage(GetTCPErrorString(registered));
		tcpSI->server_active = 0;
		//return TCP_CONNECTION_ERROR;
	}
	else
	{
		logMessage("[SERVER] The server (%d) has been successfully created.", Port);
		tcpSI->server_active = 1;
	}
	tcpSI->server_active = 1; 
	//while(tcpSI->server_active)
	while (tcpSI->server_active) {   
		// background processing
		if (tcpSI->bgdFuncs) {
			tcpSI->bgdFuncs();
		}
		
		// ProcessTCPEvents();
		ProcessSystemEvents();
	}
	UnregisterTCPServer(Port);
	
	return TCP_CONNECTION_CLOSE;
}

int tcpConnection_AddClient(tcpConnection_ServerInterface_t * tcpSI, unsigned handle)
{
	int i;

	if (!tcpSI) 
	{
		logMessage("[CODE] /-/ tcpConnection_AddClient /-/: Error! Wrong pointer of the server interface.");
		return TCP_CONNECTION_ERROR; 
	}
	if (handle == 0)
	{
		logMessage("[DATA] /-/ tcpConnection_AddClient /-/: Error! Wrong handle of a client.");
		return TCP_CONNECTION_ERROR; 	
	}
	if (tcpSI->clientsNum >= TCP_CONNECTION_MAX_CLIENTS)
	{
		logMessage("[SERVER] Error! Unable to accept more clients."); 
		return TCP_CONNECTION_ERROR; 
	}
	for(i=0; i<TCP_CONNECTION_MAX_CLIENTS; i++)
	{
		if (tcpSI->clients[i] == handle)
		{
			logMessage("[DATA] /-/ tcpConnection_AddClient /-/: Error! Unable to add a new client. Specified client handle is already in use.");
			return TCP_CONNECTION_ERROR; 
		}
	}
	
	for(i=0; i<TCP_CONNECTION_MAX_CLIENTS; i++)
	{
		if (tcpSI->clients[i] == 0)
		{
			tcpSI->clients[i] = handle;
			tcpSI->clientsNum++;  
			return i;
		}
	}
	return 0;
}

int tcpConnection_DeleteClient(tcpConnection_ServerInterface_t * tcpSI, unsigned handle)
{
	int i;
	if (!tcpSI) 
	{
		logMessage("[CODE] /-/ tcpConnection_DeleteClient /-/: Error! Wrong pointer of the server interface.");
		return TCP_CONNECTION_ERROR; 
	}
	i = tcpConnection_ClientNumberFromHandle(tcpSI,handle);
	if (i < 0)
	{
		logMessage("[DATA] /-/ tcpConnection_DeleteClient /-/: Error! Unable to delete the client. The client with the specified handle doesn't registered."); 
		return TCP_CONNECTION_ERROR;
	}
	tcpSI->clients[i] = 0;
	tcpSI->clientsNum--;
	return i;
}
