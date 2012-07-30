/**
* @file AudioMediaSubsession.cpp
* @brief Audio Media Sub session for rtsp watcher
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#include <AudioMediaSubsession.hh>
#include <WavFrameSource.hh>  
#include "log.h"

AudioMediaSubsession* AudioMediaSubsession::createNew (UsageEnvironment& env, AudioStream *audioStream, AudioCodec::Type audioCodec, int payloadType)
{
	return new AudioMediaSubsession(env, audioStream, audioCodec, payloadType);
}


AudioMediaSubsession::AudioMediaSubsession (UsageEnvironment& env, AudioStream *audioStream, AudioCodec::Type audioCodec, int payloadType) : MyOnDemandMediaSubsession(env, NULL) // reuse the first source
{
	this->m_AudioStream = audioStream;
        Log("-----[%d] %s .... calling\n", gettid(), __func__);
        mediaType = "audio";
	this->audioCodec = audioCodec;
	rtpPayloadType = payloadType;
}

AudioMediaSubsession::~AudioMediaSubsession ()
{
        Log("-----[%d] %s .... calling\n", gettid(), __func__);
	if (m_AudioStream)
		m_AudioStream->SetAudioMediaSubsession(NULL);
}

void AudioMediaSubsession::End ()
{
	m_AudioStream = NULL;
}

char const*
AudioMediaSubsession::sdpLines() {
  if (fSDPLines == NULL) {
	AddressString ipAddressStr(fServerAddressForSDP);
	char const* const sdpFmt =
	"m=%s %u RTP/AVP %d\r\n"
	"c=IN IP4 %s\r\n"
	"a=control:%s\r\n";
	unsigned sdpFmtSize = strlen(sdpFmt)
	+ strlen(mediaType) + 5 /* max short len */ + 3 /* max char len */
	+ strlen(ipAddressStr.val())
	+ 20 /* max int len */
	+ strlen(trackId());
	char* sdpLines = new char[sdpFmtSize];
	sprintf(sdpLines, sdpFmt,
		mediaType, // m= <media>
		fPortNumForSDP, // m= <port>
		rtpPayloadType, // m= <fmt list>
		ipAddressStr.val(), // c= address
		trackId()); // a=control:<track-id>
	
	fSDPLines = strDup(sdpLines);
	delete[] sdpLines;
  }

  return fSDPLines;
}

void AudioMediaSubsession::getStreamParameters(unsigned clientSessionId,
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
		      void*& streamToken) {
	MyOnDemandMediaSubsession::getStreamParameters(clientSessionId,clientAddress,clientRTPPort,clientRTCPPort,tcpSocketNum,rtpChannelId,rtcpChannelId,destinationAddress,destinationTTL,isMulticast,serverRTPPort,serverRTCPPort,streamToken);
	RTPSession *rtp = m_AudioStream->InitAudioWatcher(clientSessionId);
	serverRTPPort = rtp->GetLocalPort();
	serverRTCPPort = rtp->GetLocalPort() + 1;
	Log("-----%u:[%d] %s .... calling\n", clientSessionId, gettid(), __func__);
}

void AudioMediaSubsession::startStream(unsigned clientSessionId,
						void* streamToken,
						TaskFunc* rtcpRRHandler,
						void* rtcpRRHandlerClientData,
						unsigned short& rtpSeqNum,
						unsigned& rtpTimestamp,
						ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
						void* serverRequestAlternativeByteHandlerClientData) {
	Log("-----[%d] %s .... calling\n", gettid(), __func__);
	DestinationTable::iterator it = fDestinationsHashTable.find(clientSessionId);
	if (it==fDestinationsHashTable.end())
	{
		Log("Rtsp session %u not found\n", clientSessionId);
		return;
	}
	MyDestinations* destinations = it->second;
	char* dest_addr = NULL;
	if(destinations != NULL)
	{
		dest_addr =  inet_ntoa(destinations->addr);
		Log("-----Session destionation is %s:%d\n", dest_addr, ntohs(destinations->rtpPort.num()));
	}
	
	AudioCodec::RTPMap rtpMap;
	rtpMap[rtpPayloadType] = audioCodec;
	if(m_AudioStream) {
		RTPSession *rtp = m_AudioStream->AddAudioWatcher(clientSessionId,dest_addr,ntohs(destinations->rtpPort.num()),rtpMap, audioCodec);
		rtp->SetRtcpRRHandler(rtcpRRHandler, rtcpRRHandlerClientData);
	}
}

void AudioMediaSubsession::deleteStream(unsigned clientSessionId, void*& streamToken)
{
	MyOnDemandMediaSubsession::deleteStream(clientSessionId, streamToken);
	if(m_AudioStream) {
		m_AudioStream->DeleteAudioWatcher(clientSessionId);
	}
}

