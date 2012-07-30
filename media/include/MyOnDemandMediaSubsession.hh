/**
* @file H264MediaSubsession.hh
* @brief H264 Media Sub session for rtsp watcher
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#ifndef _MY_ON_DEMAND_MEDIA_SUBSESSION_HH
#define _MY_ON_DEMAND_MEDIA_SUBSESSION_HH

#include <UsageEnvironment.hh> 
#include <FramedSource.hh> 
#include <ServerMediaSession.hh> 
#include <RtspWatcher.hh> 

// A class that represents the state of an ongoing stream.  This is used only internally, in the implementation of
// "OnDemandServerMediaSubsession", but we expose the definition here, in case subclasses of "OnDemandServerMediaSubsession"
// want to access it.
class MyDestinations {
public:
  MyDestinations(struct in_addr const& destAddr,
               Port const& rtpDestPort,
               Port const& rtcpDestPort)
    : isTCP(False), addr(destAddr), rtpPort(rtpDestPort), rtcpPort(rtcpDestPort) {
  }
  MyDestinations(int tcpSockNum, unsigned char rtpChanId, unsigned char rtcpChanId)
    : isTCP(True), rtpPort(0) /*dummy*/, rtcpPort(0) /*dummy*/,
      tcpSocketNum(tcpSockNum), rtpChannelId(rtpChanId), rtcpChannelId(rtcpChanId) {
  }

public:
  Boolean isTCP;
  struct in_addr addr;
  Port rtpPort;
  Port rtcpPort;
  int tcpSocketNum;
  unsigned char rtpChannelId, rtcpChannelId;
};

class MyOnDemandMediaSubsession: public ServerMediaSubsession{

public:// new virtual functions
	virtual void End();

protected:
	MyOnDemandMediaSubsession(UsageEnvironment& env, RtspWatcher *watcher);
	virtual ~MyOnDemandMediaSubsession();


protected: // redefined virtual functions
	virtual void getStreamParameters(unsigned clientSessionId,
					netAddressBits clientAddress,
					Port const& clientRTPPort,
					Port const& clientRTCPPort,
					int tcpSocketNum,
					unsigned char rtpChannelId,
					unsigned char rtcpChannelId,
					netAddressBits& destinationAddress,
					u_int8_t& destinationTTL,
					Boolean& isMulticast,
					Port& serverRTPPort,
					Port& serverRTCPPort,
					void*& streamToken);
	virtual void pauseStream(unsigned clientSessionId, void* streamToken);
	virtual void seekStream(unsigned clientSessionId, void* streamToken, double& seekNPT, double streamDuration, u_int64_t& numBytes);
	virtual void setStreamScale(unsigned clientSessionId, void* streamToken, float scale);
	virtual FramedSource* getStreamSource(void* streamToken);
	virtual void deleteStream(unsigned clientSessionId, void*& streamToken);

protected:
	// key= sessionid value=rtp session
	typedef std::map<unsigned,MyDestinations*> DestinationTable;
	DestinationTable fDestinationsHashTable; // indexed by client session id
	RtspWatcher* m_Watcher;
	char* fSDPLines;
	const char* mediaType;
	char* fFmtpSDPLine;
	unsigned char rtpPayloadType;

};


#endif

