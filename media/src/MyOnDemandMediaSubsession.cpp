/**
* @file H264MediaSubsession.cpp
* @brief H264 Media Sub session for rtsp watcher
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#include <MyOnDemandMediaSubsession.hh>
#include "Base64.hh"
#include "log.h"
#include <netinet/in.h>


MyOnDemandMediaSubsession::MyOnDemandMediaSubsession (UsageEnvironment &env, RtspWatcher *watcher) : ServerMediaSubsession(env) // reuse the first source
{
        Log("-----[%d] %s .... calling\n", gettid(), __func__);
        fSDPLines = NULL;
	m_Watcher = watcher;
	fFmtpSDPLine = NULL;
	fDestinationsHashTable.clear();
}

MyOnDemandMediaSubsession::~MyOnDemandMediaSubsession ()
{
        Log("-----[%d] %s .... calling\n", gettid(), __func__);
        if (fSDPLines) 
		free(fSDPLines);
	if (fFmtpSDPLine)
		free(fFmtpSDPLine);
	// Clean out the destinations hash table:
	for (DestinationTable::iterator it=fDestinationsHashTable.begin(); it!=fDestinationsHashTable.end(); ++it)
	{
		MyDestinations* destinations = it->second;;
		delete destinations;
	}	
}

void MyOnDemandMediaSubsession::End()
{
	Log("-----[%d] %s .... calling\n", gettid(), __func__);
}

void MyOnDemandMediaSubsession
::getStreamParameters(unsigned clientSessionId,
		      netAddressBits clientAddress,
		      Port const& clientRTPPort,
		      Port const& clientRTCPPort,
		      int tcpSocketNum,
		      unsigned char rtpChannelId,
		      unsigned char rtcpChannelId,
		      netAddressBits& destinationAddress,
		      u_int8_t& /*destinationTTL*/,
		      Boolean& isMulticast,
		      Port& serverRTPPort,
		      Port& serverRTCPPort,
		      void*& streamToken) {
	if (destinationAddress == 0) destinationAddress = clientAddress;
	struct in_addr destinationAddr; 
	destinationAddr.s_addr = destinationAddress;
	isMulticast = False;
	char* dest_addr = inet_ntoa(destinationAddr);
	Log("-----%u:[%d] %s .... calling %s:%d:%d\n", clientSessionId, gettid(), __func__,dest_addr, ntohs(clientRTPPort.num()), ntohs(clientRTCPPort.num()));
	// Record these destinations as being for this client session id:
	MyDestinations* destinations = new MyDestinations(destinationAddr, clientRTPPort, clientRTCPPort);
	fDestinationsHashTable[clientSessionId] = destinations;
}

void MyOnDemandMediaSubsession::pauseStream(unsigned /*clientSessionId*/,
						void* streamToken) {
	 Log("-----[%d] %s .... calling\n", gettid(), __func__);
}

void MyOnDemandMediaSubsession::seekStream(unsigned /*clientSessionId*/,
					       void* streamToken, double& seekNPT, double streamDuration, u_int64_t& numBytes) {
	 Log("-----[%d] %s .... calling\n", gettid(), __func__);	
}

void MyOnDemandMediaSubsession::setStreamScale(unsigned /*clientSessionId*/,
						   void* streamToken, float scale) {
	 Log("-----[%d] %s .... calling\n", gettid(), __func__);
}

FramedSource* MyOnDemandMediaSubsession::getStreamSource(void* streamToken) {
	Log("-----[%d] %s .... calling\n", gettid(), __func__);
	return NULL;
}

void MyOnDemandMediaSubsession::deleteStream(unsigned clientSessionId,
						 void*& streamToken) {
	 Log("-----[%d] %s .... calling\n", gettid(), __func__);
	// Look up (and remove) the destinations for this client session:

	DestinationTable::iterator it = fDestinationsHashTable.find(clientSessionId);

	if (it==fDestinationsHashTable.end())
	{
		Log("Rtsp session %u not found\n", clientSessionId);
		return;
	}

	MyDestinations* destinations = it->second;
   	
	fDestinationsHashTable.erase(it);
	
	delete destinations;
}

