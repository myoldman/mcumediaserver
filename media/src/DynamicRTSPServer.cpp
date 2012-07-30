/**
* @file DynamicRTSPServer.cpp
* @brief Ourself ondemand rtsp server implement
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#include "DynamicRTSPServer.hh"
#include <liveMedia.hh>
#include <string.h>
#include "log.h"

DynamicRTSPServer*
DynamicRTSPServer::createNew(UsageEnvironment& env, Port ourPort,
			     UserAuthenticationDatabase* authDatabase,
			     unsigned reclamationTestSeconds) {
	int ourSocket = setUpOurSocket(env, ourPort);
	if (ourSocket == -1) return NULL;

	return new DynamicRTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds);
}

DynamicRTSPServer::DynamicRTSPServer(UsageEnvironment& env, int ourSocket,
				     Port ourPort,
				     UserAuthenticationDatabase* authDatabase, unsigned reclamationTestSeconds)
  : RTSPServerSupportingHTTPStreaming(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds) {
}

DynamicRTSPServer::~DynamicRTSPServer() {
}

ServerMediaSession *DynamicRTSPServer::lookupServerMediaSession(char const* streamName) {
  	// First, check whether the specified "streamName" exists as a local file:
	Log("-----DynamicRTSPServer lookupServerMediaSession %s\n", streamName);
  	// Next, check whether we already have a "ServerMediaSession" for this file:
  	ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(streamName);
  	Boolean smsExists = sms != NULL;

	if (smsExists) {
		return sms;
	}
	return NULL;
}

