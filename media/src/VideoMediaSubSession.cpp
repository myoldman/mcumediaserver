/**
* @file VideoMediaSubsession.cpp
* @brief Video Media Sub session for rtsp watcher
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#include <VideoMediaSubsession.hh>
#include "Base64.hh"
#include <H264FrameSource.hh>  
#include "log.h"
#include <netinet/in.h>

VideoMediaSubsession* VideoMediaSubsession::createNew (UsageEnvironment &env, VideoStream *videoStream, VideoCodec::Type videoCodec, int payloadType,  char *sps_base64, char *pps_base64)
{
	return new VideoMediaSubsession(env, videoStream, videoCodec, payloadType, sps_base64, pps_base64);
}


VideoMediaSubsession::VideoMediaSubsession (UsageEnvironment &env, VideoStream *videoStream, VideoCodec::Type videoCodec, int payloadType, char *sps_base64, char *pps_base64) : MyOnDemandMediaSubsession(env, NULL) // reuse the first source
{
	m_VideoStream = videoStream;
        Log("-----[%d] %s .... calling\n", gettid(), __func__);
	mediaType = "video";
	this->videoCodec = videoCodec;
	rtpPayloadType = payloadType;
	m_sps_base64 = NULL;
	m_pps_base64 = NULL;
	if(sps_base64 != NULL) {
		m_sps_base64 = new char[strlen(sps_base64) + 1];
		strcpy(m_sps_base64, sps_base64);
	}

	if(pps_base64 != NULL) {
		m_pps_base64 = new char[strlen(pps_base64) + 1];
		strcpy(m_pps_base64, pps_base64);
	}
}

VideoMediaSubsession::~VideoMediaSubsession ()
{
        Log("-----[%d] %s .... calling\n", gettid(), __func__);
	if (m_VideoStream != NULL)
		m_VideoStream->SetVideoMediaSubsession(NULL);

	if (m_sps_base64 != NULL)
		delete[] m_sps_base64;
	
	if (m_pps_base64 != NULL)
		delete[] m_pps_base64;
	
}

void VideoMediaSubsession::End()
{
	m_VideoStream = NULL;
	//video.End();
}

char const* VideoMediaSubsession::auxSDPLine() {
  if(m_sps_base64 == NULL)
	return NULL;

  unsigned int spsSize = 0;
   // Set up the "a=fmtp:" SDP line for this stream:
  unsigned char* sps = base64Decode(m_sps_base64, spsSize);

  u_int32_t profile_level_id;
  if (spsSize < 4) { // sanity check
    profile_level_id = 0;
  } else {
    profile_level_id = (sps[1]<<16)|(sps[2]<<8)|sps[3]; // profile_idc|constraint_setN_flag|level_idc
  }

  char const* fmtpFmt =
    "a=fmtp:%d packetization-mode=1"
    ";profile-level-id=%06X"
    ";sprop-parameter-sets=%s,%s\r\n";
  unsigned fmtpFmtSize = strlen(fmtpFmt)
    + 3 /* max char len */
    + 6 /* 3 bytes in hex */
    + strlen(m_sps_base64) + strlen(m_pps_base64);
  char* fmtp = new char[fmtpFmtSize];
  sprintf(fmtp, fmtpFmt,
          rtpPayloadType,
	  profile_level_id,
          m_sps_base64, m_pps_base64);
  if(sps != NULL)
  {
  	delete[] sps;
  }
  if(fFmtpSDPLine)
  {
  	delete[] fFmtpSDPLine; 
	fFmtpSDPLine = NULL;
  }
  fFmtpSDPLine = fmtp;

  return fFmtpSDPLine;
}

char const*
VideoMediaSubsession::sdpLines() {
  if (fSDPLines == NULL) {
	AddressString ipAddressStr(fServerAddressForSDP);
	char rtpmapLine[64] = {0};
	const char* rtpmapLinefmt = "a=rtpmap:%d %s/90000\r\n";
	sprintf(rtpmapLine, rtpmapLinefmt, rtpPayloadType, VideoCodec::GetNameFor(videoCodec));
	char const* rangeLine = rangeSDPLine();
	char* sdpLines = NULL;
	if(videoCodec == VideoCodec::H264)
	{
		char const* const sdpFmt =
		"m=%s %u RTP/AVP %d\r\n"
		"c=IN IP4 %s\r\n"
		"%s"
		"%s"
		"%s"
		"a=control:%s\r\n";
		unsigned sdpFmtSize = strlen(sdpFmt)
		+ strlen(mediaType) + 5 /* max short len */ + 3 /* max char len */
		+ strlen(ipAddressStr.val())
		+ 20 /* max int len */
		+ strlen(rtpmapLine)
		+ strlen(rangeLine)
		+ strlen(auxSDPLine())
		+ strlen(trackId());
		sdpLines = new char[sdpFmtSize];
		sprintf(sdpLines, sdpFmt,
			mediaType, // m= <media>
			fPortNumForSDP, // m= <port>
			rtpPayloadType, // m= <fmt list>
			ipAddressStr.val(), // c= address
			rtpmapLine, // a=rtpmap:... (if present)
			rangeLine, // a=range:... (if present)
			auxSDPLine(), // optional extra SDP line
			trackId()); // a=control:<track-id>
	}
	else
	{
		char const* const sdpFmt =
		"m=%s %u RTP/AVP %d\r\n"
		"c=IN IP4 %s\r\n"
		"%s"
		"%s"
		"a=control:%s\r\n";
		unsigned sdpFmtSize = strlen(sdpFmt)
		+ strlen(mediaType) + 5 /* max short len */ + 3 /* max char len */
		+ strlen(ipAddressStr.val())
		+ 20 /* max int len */
		+ strlen(rtpmapLine)
		+ strlen(rangeLine)
		+ strlen(trackId());
		sdpLines = new char[sdpFmtSize];
		sprintf(sdpLines, sdpFmt,
			mediaType, // m= <media>
			fPortNumForSDP, // m= <port>
			rtpPayloadType, // m= <fmt list>
			ipAddressStr.val(), // c= address
			rtpmapLine, // a=rtpmap:... (if present)
			rangeLine, // a=range:... (if present)
			trackId()); // a=control:<track-id>		
	}
	delete[] (char*)rangeLine; 
	
	fSDPLines = strDup(sdpLines);
	delete[] sdpLines;
  }

  return fSDPLines;
}

void VideoMediaSubsession::getStreamParameters(unsigned clientSessionId,
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
	RTPSession *rtp = m_VideoStream->InitVideoWatcher(clientSessionId);
	serverRTPPort = rtp->GetLocalPort();
	serverRTCPPort = rtp->GetLocalPort() + 1;
	Log("-----%u:[%d] %s .... calling\n", clientSessionId, gettid(), __func__);
}

void VideoMediaSubsession::startStream(unsigned clientSessionId,
						void* streamToken,
						TaskFunc* rtcpRRHandler,
						void* rtcpRRHandlerClientData,
						unsigned short& rtpSeqNum,
						unsigned& rtpTimestamp,
						ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
						void* serverRequestAlternativeByteHandlerClientData) {
	Log("-----%u:[%d] %s .... calling\n", clientSessionId, gettid(), __func__);
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
		VideoCodec::RTPMap rtpMap;
		rtpMap[rtpPayloadType] = videoCodec;
		if(m_VideoStream) {
			RTPSession *rtp = m_VideoStream->AddVideoWatcher(clientSessionId,dest_addr,ntohs(destinations->rtpPort.num()),rtpMap, videoCodec);
			rtp->SetRtcpRRHandler(rtcpRRHandler, rtcpRRHandlerClientData);
		}
	}	
}

void VideoMediaSubsession::deleteStream(unsigned clientSessionId, void*& streamToken)
{
	MyOnDemandMediaSubsession::deleteStream(clientSessionId, streamToken);
	if(m_VideoStream) {
		m_VideoStream->DeleteVideoWatcher(clientSessionId);
	}
}
