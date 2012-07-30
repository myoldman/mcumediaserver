/**
* @file VideoMediaSubsession.hh
* @brief Video Media Sub session for rtsp watcher
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#ifndef _VIDEO_MEDIA_SUBSESSION_HH
#define _VIDEO_MEDIA_SUBSESSION_HH

#include <UsageEnvironment.hh> 
#include <FramedSource.hh> 
#include <MyOnDemandMediaSubsession.hh> 
#include <video.h> 
#include "videostream.h"


class VideoMediaSubsession: public MyOnDemandMediaSubsession{
public:
	/**
	* @brief Create Session with input 
	* @param audioInput audio input frame gruber
	* @param videoInput video input frame gruber
	* @return setting result
	*/
	static VideoMediaSubsession*
		createNew(UsageEnvironment& env, VideoStream *videoStream, VideoCodec::Type videoCodec, int payloadType, char *sps_base64, char *pps_base64);
	
	/**
	* @brief VideoMediaSubsession End
	* @return none
	*/
	virtual void End();

protected:
	VideoMediaSubsession(UsageEnvironment& env, VideoStream *videoStream, VideoCodec::Type videoCodec, int payloadType, char *sps_base64, char *pps_base64);
	virtual ~VideoMediaSubsession();


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
	char *m_sps_base64;
	char *m_pps_base64;
	VideoStream *m_VideoStream;
	VideoCodec::Type videoCodec;
};
#endif

