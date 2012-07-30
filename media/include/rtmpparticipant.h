/* 
 * File:   rtmpparticipant.h
 * Author: Sergio
 *
 * Created on 19 de enero de 2012, 18:43
 */

#ifndef RTMPPARTICIPANT_H
#define	RTMPPARTICIPANT_H

#include "rtmpstream.h"
#include "codecs.h"
#include "participant.h"
#include "multiconf.h"
#include "waitqueue.h"
#include "groupvideostream.h"

class RTMPParticipant :
	public Participant,
	public RTMPStream::PublishListener
{
public:
	RTMPParticipant(int partId, int confId);
	virtual ~RTMPParticipant();

	virtual int SetVideoCodec(VideoCodec::Type codec,int mode,int fps,int bitrate,int quality=0, int fillLevel=0,int intraPeriod=0);
	virtual int SetAudioCodec(AudioCodec::Type codec);
	virtual int SetTextCodec(TextCodec::Type codec);

	virtual int SendVideoFPU();
	virtual MediaStatistics GetStatistics(MediaFrame::Type type);

	virtual int SetVideoInput(VideoInput* input)	{ videoInput	= input;	}
	virtual int SetVideoOutput(VideoOutput* output) { videoOutput	= output;	}
	virtual int SetAudioInput(AudioInput* input)	{ audioInput	= input;	}
	virtual int SetAudioOutput(AudioOutput *output)	{ audioOutput	= output;	}
	virtual int SetTextInput(TextInput* input)	{ textInput	= input;	}
	virtual int SetTextOutput(TextOutput* output)	{ textOutput	= output;	}
	virtual int SetMute(MediaFrame::Type media, bool isMuted);
	virtual int Init();
	virtual int End();

	//Publish listener
	virtual bool OnPublishedFrame(DWORD streamId,RTMPMediaFrame *frame);
	virtual bool OnPublishedMetaData(DWORD streamId,RTMPMetaData *meta);
	virtual bool OnPublishedCommand(uint32_t, const wchar_t*, AMFData*);

	int StartSending(RTMPStream *receiver);
	int StopSending();
	int StartReceiving(RTMPStream *publisher);
	int StopReceiving();
	// add for rtsp watcher by liuhong
	virtual int SetGroupVideoStream(GroupVideoStream *groupVideoStream){}

protected:
	int RecVideo();
	int RecAudio();
	int SendText();
	int SendVideo();
	int SendAudio();

	
private:
	//Video RTP
	int StartSendingVideo();
	int SetSendingVideoCodec();
	int SendFPU();
	int StopSendingVideo();
	int StartReceivingVideo();
	int StopReceivingVideo();
	//Audio RTP
	int StartSendingAudio();
	int SetSendingAudioCodec();
	int StopSendingAudio();
	int StartReceivingAudio();
	int StopReceivingAudio();
	//T140 Text RTP
	int StartSendingText();
	int StopSendingText();
	int StartReceivingText();
	int StopReceivingText();

	static void* startReceivingVideo(void *par);
	static void* startReceivingAudio(void *par);

	static void* startSendingVideo(void *par);
	static void* startSendingAudio(void *par);
	static void* startSendingText(void *par);
	
public:
	class InputStream : public RTMPStream
	{
	public:
		InputStream(DWORD id,MultiConf *conf);
		virtual ~InputStream();
		virtual bool Play(std::wstring& url);
		virtual bool Seek(DWORD time);
		virtual bool Publish(std::wstring& url);
		virtual bool Close();
	private:
		MultiConf *conf;
		RTMPParticipant *part;
	};

	class OutputStream : public RTMPStream
	{
	public:
		OutputStream(DWORD id,MultiConf *conf);
		virtual ~OutputStream();
		virtual bool Play(std::wstring& url);
		virtual bool Seek(DWORD time);
		virtual void PlayMediaFrame(RTMPMediaFrame *frame);
		virtual bool Publish(std::wstring& url);
		virtual bool Close();
	private:
		MultiConf *conf;
		RTMPParticipant *part;
		DWORD sesId;
		QWORD first;
	};

private:
	//Bridged sessions
	RTMPStream		*receiver;
	RTMPStream		*publisher;
	RTMPMetaData		*meta;

	MediaStatistics		audioStats;
	MediaStatistics		videoStats;
	MediaStatistics		textStats;
	WaitQueue<RTMPVideoFrame*> videoFrames;
	WaitQueue<RTMPAudioFrame*> audioFrames;

	AudioCodec::Type audioCodec;
	VideoCodec::Type videoCodec;
	int 		videoWidth;
	int 		videoHeight;
	int 		videoFPS;
	int 		videoBitrate;
	int		videoIntraPeriod;

	//Las threads
	pthread_t 	recVideoThread;
	pthread_t 	recAudioThread;
	pthread_t 	sendTextThread;
	pthread_t 	sendVideoThread;
	pthread_t 	sendAudioThread;
	pthread_mutex_t	mutex;

	//Controlamos si estamos mandando o no
	bool	sendingVideo;
	bool 	receivingVideo;
	bool	sendingAudio;
	bool	receivingAudio;
	bool	sendingText;
	bool	inited;
	bool	waitVideo;
	bool	sendFPU;
	timeval	first;

	VideoInput*	videoInput;
	VideoOutput*	videoOutput;
	AudioInput*	audioInput;
	AudioOutput*	audioOutput;
	TextInput*	textInput;
	TextOutput*	textOutput;

	bool	audioMuted;
	bool	videoMuted;
	bool	textMuted;
};

#endif	/* RTMPPARTICIPANT_H */

