/**
* @file H264MediaSubsession.cpp
* @brief H264 Media Sub session for rtsp watcher
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#include <H264MediaSubsession.hh>
#include "Base64.hh"
#include <H264FrameSource.hh>  
#include "log.h"
#include <netinet/in.h>

H264MediaSubsession* H264MediaSubsession::createNew (UsageEnvironment &env, RtspWatcher *watcher)
{
	return new H264MediaSubsession(env, watcher);
}


H264MediaSubsession::H264MediaSubsession (UsageEnvironment &env, RtspWatcher *watcher) : MyOnDemandMediaSubsession(env, watcher) // reuse the first source
{
        Log("-----[%d] %s .... calling\n", gettid(), __func__);
	mediaType = "video";
	rtpPayloadType = 96;
	//video.Init(watcher->GetVideoInput(), NULL);
	//video.SetVideoCodec(watcher->GetVideoCodec(),CIF,watcher->GetVideoFPS(),watcher->GetVideoBitrate(),watcher->GetVideoQuality(),watcher->GetVideoFillLevel(),watcher->GetVideoIntraPeriod());
}

H264MediaSubsession::~H264MediaSubsession ()
{
        Log("-----[%d] %s .... calling\n", gettid(), __func__);
	if (m_Watcher)
		m_Watcher->SetH264MediaSubsession(NULL);
	
}

void H264MediaSubsession::End()
{
	m_Watcher = NULL;
	//video.End();
}

char const* H264MediaSubsession::auxSDPLine() {
	           
  u_int8_t sps[] = {103,100,00,12,172,182,11,04,180,32,00,00,03,00,32,00,00,03,03,209,226,133,92}; 
  unsigned spsSize = 23;
  u_int8_t pps[] = {104,234,195,203,34,192}; 
  unsigned ppsSize = 6;;
  u_int32_t profile_level_id;
  if (spsSize < 4) { // sanity check
    profile_level_id = 0;
  } else {
    profile_level_id = (sps[1]<<16)|(sps[2]<<8)|sps[3]; // profile_idc|constraint_setN_flag|level_idc
  }
  
  // Set up the "a=fmtp:" SDP line for this stream:
  char* sps_base64 = base64Encode((char*)sps, spsSize);
  char* pps_base64 = base64Encode((char*)pps, ppsSize);
  char const* fmtpFmt =
    "a=fmtp:%d packetization-mode=1"
    ";profile-level-id=%06X"
    ";sprop-parameter-sets=%s,%s\r\n";
  unsigned fmtpFmtSize = strlen(fmtpFmt)
    + 3 /* max char len */
    + 6 /* 3 bytes in hex */
    + strlen(sps_base64) + strlen(pps_base64);
  char* fmtp = new char[fmtpFmtSize];
  sprintf(fmtp, fmtpFmt,
          rtpPayloadType,
	  profile_level_id,
          sps_base64, pps_base64);
  delete[] sps_base64;
  delete[] pps_base64;

  if(fFmtpSDPLine)
  {
  	delete[] fFmtpSDPLine; 
	fFmtpSDPLine = NULL;
  }
  fFmtpSDPLine = fmtp;

  return fFmtpSDPLine;
}

char const*
H264MediaSubsession::sdpLines() {
  if (fSDPLines == NULL) {
	AddressString ipAddressStr(fServerAddressForSDP);
	const char* rtpmapLine = "a=rtpmap:96 H264/90000\r\n";
	char const* rangeLine = rangeSDPLine();
	char const* const sdpFmt =
	"m=%s %u RTP/AVP %d\r\n"
	"c=IN IP4 %s\r\n"
	"b=AS:%u\r\n"
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
	char* sdpLines = new char[sdpFmtSize];
	sprintf(sdpLines, sdpFmt,
		mediaType, // m= <media>
		fPortNumForSDP, // m= <port>
		rtpPayloadType, // m= <fmt list>
		ipAddressStr.val(), // c= address
		m_Watcher->GetVideoBitrate(), // b=AS:<bandwidth>
		rtpmapLine, // a=rtpmap:... (if present)
		rangeLine, // a=range:... (if present)
		auxSDPLine(), // optional extra SDP line
		trackId()); // a=control:<track-id>
	
	delete[] (char*)rangeLine; 
	
	fSDPLines = strDup(sdpLines);
	delete[] sdpLines;
  }

  return fSDPLines;
}

void H264MediaSubsession::getStreamParameters(unsigned clientSessionId,
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
	RTPSession *rtp = m_Watcher->StartRecvingVideoRtcp(clientSessionId);
	serverRTPPort = rtp->GetLocalPort();
	serverRTCPPort = rtp->GetLocalPort() + 1;
	Log("-----%u:[%d] %s .... calling\n", clientSessionId, gettid(), __func__);
}

void H264MediaSubsession::startStream(unsigned clientSessionId,
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

	VideoCodec::RTPMap rtpMap;
	rtpMap[rtpPayloadType] = m_Watcher->GetVideoCodec();
	//video.StartSending(clientSessionId,dest_addr,ntohs(destinations->rtpPort.num()),rtpMap);
	RTPSession *rtp = m_Watcher->StartSendingVideo(clientSessionId,dest_addr,ntohs(destinations->rtpPort.num()),rtpMap);
	rtp->SetRtcpRRHandler(rtcpRRHandler, rtcpRRHandlerClientData);
}

void H264MediaSubsession::deleteStream(unsigned clientSessionId, void*& streamToken)
{
	MyOnDemandMediaSubsession::deleteStream(clientSessionId, streamToken);
	//video.StopSending(clientSessionId);
	if(m_Watcher)
		m_Watcher->StopSendingVideo(clientSessionId);
	fParentSession->decrementReferenceCount();
}
