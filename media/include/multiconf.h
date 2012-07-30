#ifndef _MULTICONF_H_
#define _MULTICONF_H_
#include "videomixer.h"
#include "audiomixer.h"
#include "textmixer.h"
#include "videomixer.h"
#include "participant.h"
#include "FLVEncoder.h"
#include "broadcastsession.h"
#include "mp4player.h"
#include "mp4recorder.h"
#include "audioencoder.h"
#include "textencoder.h"
#include <map>
#include <string>
// rtsp addon by liuhong
//#include "RtspWatcher.hh"
#include "groupvideostream.h"
#include "groupaudiostream.h"

class MultiConf : public Participant::Listener
{

public:
	typedef std::map<std::string,MediaStatistics> ParticipantStatistics;
	
public:
	class BroadcastStream : public RTMPStream
	{
		public:
			BroadcastStream(DWORD streamId,MultiConf *conf);
			virtual ~BroadcastStream();
			virtual bool Play(std::wstring& url);
			virtual bool Seek(DWORD time);
			virtual bool Publish(std::wstring& url);
			virtual bool Close();
		private:
			MultiConf *conf;
			DWORD confId;
	};

	class Listener
	{
	public:
		virtual void onParticipantRequestFPU(MultiConf *conf,int partId,void *param) = 0;
	};
	
public:
	MultiConf(const std::wstring& tag, int confId);
	~MultiConf();

	int Init();
	int End();

	void SetListener(Listener *listener,void* param);

	int CreateMosaic(Mosaic::Type comp,int size);
	int SetMosaicOverlayImage(int mosaicId,const char* filename);
	int ResetMosaicOverlay(int mosaicId);
	int DeleteMosaic(int mosaicId);
	int CreateParticipant(int mosaicId,std::wstring name,Participant::Type type);
	int StartRecordingParticipant(int partId,const char* filename);
	int StopRecordingParticipant(int partId);
	int SendFPU(int partId);
	int SetMute(int partId,MediaFrame::Type media,bool isMuted);
	ParticipantStatistics* GetParticipantStatistic(int partId);
	int SetParticipantMosaic(int partId,int mosaicId);
	int DeleteParticipant(int partId);

	int CreatePlayer(int privateId,std::wstring name);
	int StartPlaying(int playerId,const char* filename,bool loop);
	int StopPlaying(int playerId);
	int DeletePlayer(int playerId);

	int SetCompositionType(int mosaicId,Mosaic::Type comp,int size);
	int SetMosaicSlot(int mosaicId,int num,int id);
	int AddMosaicParticipant(int mosaicId,int partId);
	int RemoveMosaicParticipant(int mosaicId,int partId);
	
	//Video
	int SetVideoCodec(int partId,int codec,int mode,int fps,int bitrate,int quality=0, int fillLevel=0,int intraPeriod = 0);
	RTPSession *StartSendingVideo(int partId,char *sendVideoIp,int sendVideoPort,VideoCodec::RTPMap& rtpMap);
	int StopSendingVideo(int partId);
	int StartReceivingVideo(int partId,VideoCodec::RTPMap& rtpMap);
	int StopReceivingVideo(int partId);
	int IsSendingVideo(int partId);
	int IsReceivingVideo(int partId);

	//Audio
	int SetAudioCodec(int partId,int codec);
	RTPSession *StartSendingAudio(int partId,char *sendAudioIp,int sendAudioPort,AudioCodec::RTPMap& rtpMap);
	int StopSendingAudio(int partId);
	int StartReceivingAudio(int partId,AudioCodec::RTPMap& rtpMap);
	int StopReceivingAudio(int partId);
	int IsSendingAudio(int partId);
	int IsReceivingAudio(int partId);

	//Text
	int SetTextCodec(int partId,int codec);
	int StartSendingText(int partId,char *sendTextIp,int sendTextPort,TextCodec::RTPMap& rtpMap);
	int StopSendingText(int partId);
	int StartReceivingText(int partId,TextCodec::RTPMap& rtpMap);
	int StopReceivingText(int partId);
	int IsSendingText(int partId);
	int IsReceivingText(int partId);

	int  StartBroadcaster();
	int  StopBroadcaster();

	bool  AddBroadcastToken(const std::wstring &token);
	bool  ConsumeBroadcastToken(const std::wstring &token);
	
	bool  AddParticipantInputToken(int partId,const std::wstring &token);
	bool  AddParticipantOutputToken(int partId,const std::wstring &token);
	Participant* ConsumeParticipantInputToken(const std::wstring &token);
	Participant* ConsumeParticipantOutputToken(const std::wstring &token);
	int GetNumParticipants() { return participants.size(); }
	std::wstring& GetTag() { return tag;	}

	/** Participants event */
	void onRequestFPU(Participant *part);

	// rtsp addon by liuhong
	RTPSession *StartRecvingVideoRtcp(int partId);
//	int StopRecvingVideoRtcp(int partId);
	RTPSession *StartRecvingAudioRtcp(int partId);
//	int StopRecvingAudioRtcp(int partId);
protected:
	bool AddBroadcastReceiver(RTMPStream *receiver);
	bool RemoveBroadcastReceiver(RTMPStream *receiver);

private:
	Participant *GetParticipant(int partId);
	Participant *GetParticipant(int partId,Participant::Type type);
	void DestroyParticipant(int partId,Participant* part);
private:
	typedef std::map<int,Participant*> Participants;
	typedef std::set<std::wstring> BroadcastTokens;
	typedef std::map<std::wstring,DWORD> ParticipantTokens;
	typedef std::map<int, MP4Player*> Players;
	
private:
	ParticipantTokens	inputTokens;
	ParticipantTokens	outputTokens;
	BroadcastTokens		tokens;
	//Atributos
	int		inited;
	int		maxId;
	std::wstring	tag;

	Listener *listener;
	void* param;

	//Los mixers
	VideoMixer videoMixer;
	AudioMixer audioMixer;
	TextMixer  textMixer;

	//Lists
	Participants		participants;
	Players			players;

	int			watcherId;
	int			broadcastId;
	FLVEncoder		flvEncoder;
	// rtsp addon by liuhong
	//RtspWatcher		m_RtspWatcher;
	AudioEncoder		audioEncoder;
	TextEncoder		textEncoder;
	BroadcastSession	broadcast;
	// group h264 encoder addon
	GroupVideoStream	m_GroupVideo;
	GroupAudioStream	m_GroupAudio;
	int			m_GroupCastId;
	int			m_ConfId;
	int 			m_CurrentBroadCaster;
private:
	void SendIdrPacket(RTPSession *rtp);
	Use			participantsLock;
};

#endif
