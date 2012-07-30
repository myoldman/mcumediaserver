/**
* @file H264MediaSubsession.hh
* @brief H264 Media Sub session for rtsp watcher
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#ifndef _H264_MEDIA_SUBSESSION_HH
#define _H264_MEDIA_SUBSESSION_HH

#include <UsageEnvironment.hh> 
#include <FramedSource.hh> 
#include <MyOnDemandMediaSubsession.hh> 
#include <RtspWatcher.hh> 
#include "video.h"
#include "groupvideostream.h"


class H264MediaSubsession: public MyOnDemandMediaSubsession{
public:
	/**
	* @brief Create Session with input 
	* @param audioInput audio input frame gruber
	* @param videoInput video input frame gruber
	* @return setting result
	*/
	static H264MediaSubsession*
		createNew(UsageEnvironment& env, RtspWatcher *watcher);
	
	/**
	* @brief H264Subssession End
	* @return none
	*/
	virtual void End();

protected:
	H264MediaSubsession(UsageEnvironment& env, RtspWatcher *watcher);
	virtual ~H264MediaSubsession();


protected: // redefined virtual functions
	virtual char const* sdpLines();
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
	virtual void startStream(unsigned clientSessionId, void* streamToken,
				TaskFunc* rtcpRRHandler,
				void* rtcpRRHandlerClientData,
				unsigned short& rtpSeqNum,
				unsigned& rtpTimestamp,
				ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
				void* serverRequestAlternativeByteHandlerClientData);
	virtual void deleteStream(unsigned clientSessionId, void*& streamToken);

private:
	char const* auxSDPLine();
	//GroupVideoStream	video;
	//RtspWatcher* m_RtspWatcher;

};
#endif
