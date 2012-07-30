#include "log.h"
#include "rtmpstream.h"

RTMPStream::RTMPStream(DWORD id)
{
	playListener = NULL;
	publishListener = NULL;
	this->id = id;
}

RTMPStream::~RTMPStream()
{
}

void RTMPStream::PublishMediaFrame(RTMPMediaFrame* frame)
{
	//If we got a listener
	if (publishListener)
		//Signal 
		publishListener->OnPublishedFrame(id,frame);
}

void RTMPStream::PublishCommand(const wchar_t *name,AMFData* obj)
{
	//If we got a listener
	if (publishListener)
		//Signal
		publishListener->OnPublishedCommand(id,name,obj);
}

void RTMPStream::PlayMediaFrame(RTMPMediaFrame* frame)
{
	//If we got a listener
	if (playListener)
		//Signal 
		playListener->OnPlayedFrame(id,frame);
}

void RTMPStream::PublishMetaData(RTMPMetaData* meta)
{
	//If we got a listener
	if (publishListener)
		//Signal 
		publishListener->OnPublishedMetaData(id,meta);
}

void RTMPStream::PlayMetaData(RTMPMetaData* meta)
{
	//If we got a listener
	if (playListener)
		//Signal 
		return playListener->OnPlayedMetaData(id,meta);
}

void RTMPStream::SendCommand(const wchar_t *name,AMFData* obj)
{
	//If we got a listener
	if (playListener)
		//Signal
		return playListener->OnCommand(id,name,obj);
}

void RTMPStream::SendStreamEnd()
{
	//If we got a listener
	if (playListener)
		//Signal
		return playListener->OnStreamEnd(id);
}

void RTMPStream::SendStreamBegin()
{
	//If we got a listener
	if (playListener)
		//Signal
		return playListener->OnStreamBegin(id);
}

void RTMPStream::SendStreamIsRecorded()
{
	//If we got a listener
	if (playListener)
		//Signal
		return playListener->OnStreamIsRecorded(id);
}

void RTMPStream::Reset()
{
	//If we got a listener
	if (playListener)
		//Signal
		return playListener->OnStreamReset(id);
}

void RTMPStream::SetPublishListener(PublishListener* listener)
{
	publishListener = listener;
}
void RTMPStream::SetPlayListener(PlayListener* listener)
{
	playListener = listener;
}
