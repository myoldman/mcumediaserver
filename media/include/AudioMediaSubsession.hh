/**
* @file WavMediaSubsession.hh
* @brief Wav Media Sub session for rtsp watcher
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#ifndef _AUDIO_MEDIA_SUBSESSION_HH
#define _AUDIO_MEDIA_SUBSESSION_HH

#include <UsageEnvironment.hh> 
#include <FramedSource.hh> 
#include <MyOnDemandMediaSubsession.hh> 
#include "audio.h"
#include "audiostream.h"

class AudioMediaSubsession: public MyOnDemandMediaSubsession{
public:
	/**
	* @brief Create Session with input 
	* @param env live555 usage env
	* @param watcher rtsp watcher instance
	* @return setting result
	*/
	static AudioMediaSubsession*
		createNew(UsageEnvironment& env, AudioStream *audioStream, AudioCodec::Type audioCodec, int payloadType);
	
	/**
	* @brief AudioMediaSubsession End
	* @return none
	*/
	virtual void End();

protected:
	AudioMediaSubsession(UsageEnvironment& env, AudioStream *audioStream, AudioCodec::Type audioCodec, int payloadType);
	virtual ~AudioMediaSubsession();


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
	AudioStream	*m_AudioStream;
	AudioCodec::Type audioCodec;

};

#endif
