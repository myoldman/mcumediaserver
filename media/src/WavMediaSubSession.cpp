/**
* @file WavMediaSubsession.cpp
* @brief Wav Media Sub session for rtsp watcher
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#include <WavMediaSubsession.hh>
#include <WavFrameSource.hh>  
#include "log.h"
#include "rtpsession.h"

WavMediaSubsession* WavMediaSubsession::createNew (UsageEnvironment &env, RtspWatcher *watcher)
{
	return new WavMediaSubsession(env, watcher);
}


WavMediaSubsession::WavMediaSubsession (UsageEnvironment &env, RtspWatcher *watcher) : MyOnDemandMediaSubsession(env, watcher) // reuse the first source
{
        Log("-----[%d] %s .... calling\n", gettid(), __func__);
        mediaType = "audio";
	rtpPayloadType = 0;
	audio.Init(watcher->GetAudioInput(), NULL);
	audio.SetAudioCodec(watcher->GetAudioCodec());
}

WavMediaSubsession::~WavMediaSubsession ()
{
        Log("-----[%d] %s .... calling\n", gettid(), __func__);
	if (m_Watcher)
		m_Watcher->SetWavMediaSubsession(NULL);
}

void WavMediaSubsession::End ()
{
	audio.End();
}

char const*
WavMediaSubsession::sdpLines() {
  if (fSDPLines == NULL) {
	AddressString ipAddressStr(fServerAddressForSDP);
	char const* const sdpFmt =
	"m=%s %u RTP/AVP %d\r\n"
	"c=IN IP4 %s\r\n"
//	"b=AS:%u\r\n"
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
//		64, // b=AS:<bandwidth>
		trackId()); // a=control:<track-id>
	
	fSDPLines = strDup(sdpLines);
	delete[] sdpLines;
  }

  return fSDPLines;
}

void WavMediaSubsession::getStreamParameters(unsigned clientSessionId,
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
	RTPSession *rtp = audio.StartReceivingRtcp(clientSessionId);
	serverRTPPort = rtp->GetLocalPort();
	serverRTCPPort = rtp->GetLocalPort() + 1;
	Log("-----%u:[%d] %s .... calling\n", clientSessionId, gettid(), __func__);
}

void WavMediaSubsession::startStream(unsigned clientSessionId,
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
	rtpMap[rtpPayloadType] = m_Watcher->GetAudioCodec();
	RTPSession *rtp = audio.StartSending(clientSessionId,dest_addr,ntohs(destinations->rtpPort.num()),rtpMap, NULL);
	rtp->SetRtcpRRHandler(rtcpRRHandler, rtcpRRHandlerClientData);
	//m_Watcher->StartSendingAudio(clientSessionId,dest_addr,ntohs(destinations->rtpPort.num()),rtpMap);
}

void WavMediaSubsession::deleteStream(unsigned clientSessionId, void*& streamToken)
{
	MyOnDemandMediaSubsession::deleteStream(clientSessionId, streamToken);
	audio.StopSending(clientSessionId);
	fParentSession->decrementReferenceCount();
	//m_Watcher->StopSendingAudio(clientSessionId);
}

