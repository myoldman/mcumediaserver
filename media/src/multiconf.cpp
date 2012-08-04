#include <stdlib.h>
#include "log.h"
#include "multiconf.h"
#include "rtpparticipant.h"
#include "rtmpparticipant.h"

/************************
* MultiConf
* 	Constructor
*************************/
MultiConf::MultiConf(const std::wstring &tag, int confId) : broadcast(tag)
{
	//Guardamos el nombre
	this->tag = tag;

	//No watcher
	watcherId = 0;

	//Inicializamos el contador
	maxId = 500;
	spyId = 1000;

	//Y no tamos iniciados
	inited = 0;
	
	// add conf id 
	m_ConfId = confId;

	m_CurrentBroadCaster = 0;
}


/************************
* ~ MultiConf
* 	Destructor
*************************/
MultiConf::~MultiConf()
{
	//Pa porsi
	if (inited)
		End();
}

void MultiConf::SetListener(Listener *listener,void* param)
{
	//Store values
	this->listener = listener;
	this->param = param;
}

/************************
* Init
* 	Constructo
*************************/
int MultiConf::Init()
{
	Log("-Init multiconf\n");

	//We are inited
	inited = true;

	//Init audio mixers
	int res = audioMixer.Init();
	//Init video mixer with dedault parameters
	res &= videoMixer.Init(Mosaic::mosaic2x2,CIF);
	//Init text mixer
	res &= textMixer.Init();

	//Check if we are inited
	if (!res)
		//End us
		End();

	//Get the id
	watcherId = maxId++;

	//Create audio and text watcher
	audioMixer.CreateMixer(watcherId);
	std::wstring name = std::wstring(L"watcher");
	textMixer.CreateMixer(watcherId,name);

	//Init the audio encoder
	audioEncoder.Init(audioMixer.GetInput(watcherId));
	//Init the text encoder
	textEncoder.Init(textMixer.GetInput(watcherId));

	//Set codec
	audioEncoder.SetAudioCodec(AudioCodec::PCMA);

	//Start mixers
	audioMixer.InitMixer(watcherId);
	textMixer.InitMixer(watcherId);

	//Start encoding
	audioEncoder.StartEncoding();
	textEncoder.StartEncoding();

	return res;
}

/************************
* SetCompositionType
* 	Set mosaic type and size
*************************/
int MultiConf::SetCompositionType(int mosaicId,Mosaic::Type comp,int size)
{
	Log("-SetCompositionType [mosaic:%d,comp:%d,size:%d]\n",mosaicId,comp,size);

	//POnemos el tipo de video mixer
	return videoMixer.SetCompositionType(mosaicId,comp,size);
}

/*****************************
* StartBroadcaster
* 	Create FLV Watcher port
*******************************/
int MultiConf::StartBroadcaster()
{
	std::wstring name = std::wstring(L"broadcaster");

	Log(">StartBroadcaster\n");

	//Cehck if running
	if (!inited)
		//Exit
		return Error("-Not inited\n");

	//Get the id
	broadcastId = maxId++;

	//Create mixers
	videoMixer.CreateMixer(broadcastId);
	audioMixer.CreateMixer(broadcastId);
	textMixer.CreateMixer(broadcastId,name);

	// rtsp addon by liuhong
	//Init it flv encoder
	//flvEncoder.Init(audioMixer.GetInput(broadcastId),videoMixer.GetInput(broadcastId));

	//Create the broadcast session without limits
	broadcast.Init(0,0);

	// rtsp addon by liuhong
	//Set the encoder as the transmitter
	//broadcast.SetTransmitter(&flvEncoder);

	//Init mixers
	videoMixer.InitMixer(broadcastId,0);
	audioMixer.InitMixer(broadcastId);
	textMixer.InitMixer(broadcastId);

	// rtsp addon by liuhong
	//Start encoding
	//flvEncoder.StartEncoding();
	
	// rtsp addon by liuhong
//	char buffer[20];
//	sprintf(buffer, "%d", m_ConfId);
//	m_RtspWatcher.Init(audioMixer.GetInput(broadcastId),videoMixer.GetInput(broadcastId), std::string(buffer), this);
//	m_RtspWatcher.SetVideoCodec(VideoCodec::H264, CIF, 15, 512, 1, 6, 300);
//	m_RtspWatcher.SetAudioCodec(AudioCodec::PCMU);
//	m_RtspWatcher.StartStreaming();
	
	// group h264 encoder addon
	m_GroupCastId = maxId++;
	std::wstring groupCastName = std::wstring(L"GroupCast");
	//Create mixers
	videoMixer.CreateMixer(m_GroupCastId);
	audioMixer.CreateMixer(m_GroupCastId);
	textMixer.CreateMixer(m_GroupCastId,groupCastName);
	//Init mixers
	videoMixer.InitMixer(m_GroupCastId,0);
	audioMixer.InitMixer(m_GroupCastId);
	textMixer.InitMixer(m_GroupCastId);

	m_GroupVideo.Init(videoMixer.GetInput(m_GroupCastId), NULL);
	m_GroupAudio.Init(audioMixer.GetInput(m_GroupCastId), NULL);
	m_GroupVideo.SetPublishListener(&broadcast);

	Log("<StartBroadcaster\n");

	return 1;
}

/*****************************
* StopBroadcaster
*       End FLV Watcher port
******************************/
int MultiConf::StopBroadcaster()
{
	//If no watcher
	if (!broadcastId)
		//exit
		return 0;

	Log(">StopBroadcaster\n");

	//Stop endoding
	//flvEncoder.StopEncoding();

	//End mixers
	videoMixer.EndMixer(broadcastId);
	audioMixer.EndMixer(broadcastId);
	textMixer.EndMixer(broadcastId);

	// rtsp addon by liuhong
	//End Transmiter
	//flvEncoder.End();
	
	// rtsp addon by liuhong
//	m_RtspWatcher.StopStreaming();
	
	// rtsp addon by liuhong
//	m_RtspWatcher.End();

	//Stop broacast
	broadcast.End();
	m_GroupVideo.SetPublishListener(NULL);

	//End mixers
	videoMixer.DeleteMixer(broadcastId);
	audioMixer.DeleteMixer(broadcastId);
	textMixer.DeleteMixer(broadcastId);

	//Unset watcher id
	broadcastId = 0;
	
	// group stream add on by liuhong
	videoMixer.EndMixer(m_GroupCastId);
	audioMixer.EndMixer(m_GroupCastId);
	textMixer.EndMixer(m_GroupCastId);

	m_GroupAudio.End();
	m_GroupVideo.End();
	
	videoMixer.DeleteMixer(m_GroupCastId);
	audioMixer.DeleteMixer(m_GroupCastId);
	textMixer.DeleteMixer(m_GroupCastId);

	m_GroupCastId = 0;
	
	Log("<StopBroadcaster\n");
	
	return 1;
}

bool MultiConf::AddBroadcastReceiver(RTMPStream *receiver)
{
	broadcast.AddReceiver(receiver);
	Participants::iterator itBroadcaster = participants.find(m_CurrentBroadCaster);
	if(itBroadcaster != participants.end())
	{
		RTPParticipant *broadCaster = (RTPParticipant*)itBroadcaster->second;
		Log("Send idr packet to newly broadcast reciever\n");
		IDRPacketSize idrPacketSize = broadCaster->GetIdrPacketSize();
		IDRPacket idrPacket = broadCaster->GetIdrPacket();
		DWORD currentTimeStamp = broadCaster->GetCurrentTimestamp();
		size_t packetSize = idrPacket.size();

		//Crete desc frame
		RTMPVideoFrame frameDesc(0,2048);
		//Send
		frameDesc.SetTimestamp(currentTimeStamp);
		//Set type
		frameDesc.SetVideoCodec(RTMPVideoFrame::AVC);
		//Set type
		frameDesc.SetFrameType(RTMPVideoFrame::INTRA);
		//Set NALU type
		frameDesc.SetAVCType(0);
		//Set no delay
		frameDesc.SetAVCTS(0);
		//Create description
		AVCDescriptor desc;
		//Set values
		desc.SetConfigurationVersion(1);
		//desc.SetAVCProfileIndication(0x42);
		//desc.SetProfileCompatibility(0x80);
		//desc.SetAVCLevelIndication(0x14);
		//desc.SetAVCProfileIndication(idrPacket[0][1]);
		//desc.SetProfileCompatibility(idrPacket[0][2]);
		//desc.SetAVCLevelIndication(idrPacket[0][3]);
		desc.SetAVCProfileIndication(0x64);
		desc.SetProfileCompatibility(0x00);
		desc.SetAVCLevelIndication(0x28);
		desc.SetNALUnitLength(3);
		desc.AddSequenceParameterSet(idrPacket[0],idrPacketSize[0]);
		desc.AddPictureParameterSet(idrPacket[1],idrPacketSize[1]);
		//Serialize
		DWORD len = desc.Serialize(frameDesc.GetMediaData(),frameDesc.GetMaxMediaSize());
		//Set size
		frameDesc.SetMediaSize(len);
		//broadcast.OnPublishedFrame(0, &frameDesc);
		receiver->PlayMediaFrame(&frameDesc);
		frameDesc.Dump();

		RTMPVideoFrame frame(0,65535);
		//Set codec
		frame.SetVideoCodec(RTMPVideoFrame::AVC);
		//Set NALU type
		frame.SetAVCType(1);
		//Set no delay
		frame.SetAVCTS(0);
		frame.SetTimestamp(currentTimeStamp);
		frame.SetFrameType(RTMPVideoFrame::INTRA);
		VideoFrame *videoFrame;
		RTPDepacketizer *depacketizer = RTPDepacketizer::Create( MediaFrame::Video, VideoCodec::H264);
		for(int i = 0; i < packetSize; i++) {
			BYTE *packet = idrPacket[i];
			int packet_size = idrPacketSize[i];
			videoFrame = (VideoFrame *)depacketizer->AddPayload(packet,packet_size);
		}
		frame.SetVideoFrame(videoFrame->GetData(), videoFrame->GetLength());
		receiver->PlayMediaFrame(&frame);
		frame.Dump();
		delete depacketizer;
		
	}
	
	return true;
}

bool MultiConf::RemoveBroadcastReceiver(RTMPStream *receiver)
{
	return broadcast.RemoveReceiver(receiver);
}
/************************
* SetMosaicSlot
* 	Set slot position on mosaic
*************************/
int MultiConf::SetMosaicSlot(int mosaicId,int slot,int id)
{
	Log("-SetMosaicSlot [mosaic:%d,slot:%d,id:%d]\n",mosaicId,slot,id);
	participantsLock.IncUse();
	if(m_CurrentBroadCaster == id && participants.size())
	{
		Log("BroadCaster Not Change just do nothing\n");
	}
	else
	{
		Participants::iterator itold = participants.find(m_CurrentBroadCaster);
		if (itold == participants.end())
		{
			participantsLock.DecUse();
			return Error("old broadcast not found\n");
		}
		itold->second->SetGroupVideoStream(NULL);
		
		Participants::iterator itnew = participants.find(id);

		if (itnew == participants.end())
		{
			participantsLock.DecUse();
			return Error("New broadcast not found\n");
		}

		itnew->second->SetGroupVideoStream(&m_GroupVideo);
		m_CurrentBroadCaster = id;
		itnew->second->SendVideoFPU();
	}
	participantsLock.DecUse();
	//Set it
	return videoMixer.SetSlot(mosaicId,slot,id);
}

/************************
* AddMosaicParticipant
* 	Show participant in a mosaic
*************************/
int MultiConf::AddMosaicParticipant(int mosaicId,int partId)
{
	//Set it
	return videoMixer.AddMosaicParticipant(mosaicId,partId);
}

/************************
* RemoveMosaicParticipant
* 	Unshow a participant in a mosaic
*************************/
int MultiConf::RemoveMosaicParticipant(int mosaicId,int partId)
{
	Log("-RemoveMosaicParticipant [mosaic:%d,partId:]\n",mosaicId,partId);

	//Set it
	return videoMixer.RemoveMosaicParticipant(mosaicId,partId);
}

/************************
* CreateMosaic
* 	Add a mosaic to the conference
*************************/
int MultiConf::CreateMosaic(Mosaic::Type comp,int size)
{
	return videoMixer.CreateMosaic(comp,size);
}
/************************
* SetMosaicOverlayImage
* 	Set mosaic overlay image
*************************/
int MultiConf::SetMosaicOverlayImage(int mosaicId,const char* filename)
{
	return videoMixer.SetMosaicOverlayImage(mosaicId,filename);
}
/************************
* ResetMosaicOverlayImage
* 	Reset mosaic overlay image
*************************/
int MultiConf::ResetMosaicOverlay(int mosaicId)
{
	return videoMixer.ResetMosaicOverlay(mosaicId);
}
/************************
* DeleteMosaic
* 	delete mosaic
*************************/
int MultiConf::DeleteMosaic(int mosaicId)
{
	return videoMixer.DeleteMosaic(mosaicId);
}


int MultiConf::CreateSpy(int spyeeId)
{
	Participant *part = NULL;
	Log(">CreateSpy [spyee:%d]\n",spyeeId);	
	if (!inited)
		return Error("Not inited\n");
	
	//Get lock
	spysLock.WaitUnusedAndLock();

	//Obtenemos el id
	int spyId = this->spyId++;
	spys[spyId] = spyeeId;

	//Unlock
	spysLock.Unlock();

	Log("<CreateSpy [%d]\n",spyeeId);	
}

/************************
* CreateParticipant
* 	A�ade un participante
*************************/
int MultiConf::CreateParticipant(int mosaicId,std::wstring name,Participant::Type type)
{
	Participant *part = NULL;

	Log(">CreateParticipant [mosaic:%d]\n",mosaicId);

	//SI no tamos iniciados pasamos
	if (!inited)
		return Error("Not inited\n");

	//Get lock
	participantsLock.WaitUnusedAndLock();

	//Obtenemos el id
	int partId = maxId++;

	//Unlock
	participantsLock.Unlock();

	//Le creamos un mixer
	if (!videoMixer.CreateMixer(partId))
		return Error("Couldn't set video mixer\n");

	//Y el de audio
	if (!audioMixer.CreateMixer(partId))
	{
		//Borramos el de video
		videoMixer.DeleteMixer(partId);
		//Y salimos
		return Error("Couldn't set audio mixer\n");
	}

	//And text
	if (!textMixer.CreateMixer(partId,name))
	{
		//Borramos el de video y audio
		videoMixer.DeleteMixer(partId);
		audioMixer.DeleteMixer(partId);
		//Y salimos
		return Error("Couldn't set text mixer\n");
	}

	//Depending on the type
	switch(type)
	{
		case Participant::RTP:
			//Create RTP Participant
			part = new RTPParticipant(partId, m_ConfId);
			break;
		case Participant::RTMP:
			part = new RTMPParticipant(partId, m_ConfId);
			//Create RTP Participant
			break;
	}

	//Set inputs and outputs
	part->SetVideoInput(videoMixer.GetInput(partId));
	part->SetVideoOutput(videoMixer.GetOutput(partId));
	part->SetAudioInput(audioMixer.GetInput(partId));
	part->SetAudioOutput(audioMixer.GetOutput(partId));
	part->SetTextInput(textMixer.GetInput(partId));
	part->SetTextOutput(textMixer.GetOutput(partId));

	//Init participant
	part->Init();

	//E iniciamos el mixer
	videoMixer.InitMixer(partId,mosaicId);
	audioMixer.InitMixer(partId);
	textMixer.InitMixer(partId);

	//Get lock
	participantsLock.WaitUnusedAndLock();

	//Lo insertamos en el map
	participants[partId] = part;

	//add for rtsp watcher by liuhong start
	if (m_CurrentBroadCaster == 0)
	{
		part->SetGroupVideoStream(&m_GroupVideo);
		m_CurrentBroadCaster = partId;
		videoMixer.SetSlot(mosaicId,0,partId);
	}
	else
	{
		Log("BroadCaster already exist send fpu everybody\n");
		//Participants::iterator it = participants.find(m_CurrentBroadCaster);
		//if (it == participants.end())
		//{
		//	return Error("Participant not found\n");
		//}
		//it->second->SendVideoFPU();
	}
	//add for rtsp watcher by end

	//Unlock
	participantsLock.Unlock();

	//Set us as listener
	part->SetListener(this);

	Log("<CreateParticipant [%d]\n",partId);

	return partId;
}

int MultiConf::DeleteSpy(int spyId)
{
	int ret = 0;
	if(spys.find(spyId) == spys.end())
		return 0;
	
	int spyeeId = spys[spyId];	

	Log(">DeleteSpy [%d]\n",spyId);

	//Use list
	participantsLock.IncUse();
	RTPParticipant *part = (RTPParticipant*)GetParticipant(spyeeId,Participant::RTP);
	
	//Check particpant
	if (part)
		//Set video codec
		ret = part->DeleteVideoSpy(spyId);


	//Unlock
	participantsLock.DecUse();
	

	Log("<DeleteSpy [%d]\n",spyId);

	return 1;
}

/************************
* DeleteParticipant
* 	Borra un participante
*************************/
int MultiConf::DeleteParticipant(int id)
{
	Log(">DeleteParticipant [%d]\n",id);

	//Stop recording participant just in case
	StopRecordingParticipant(id);

	//Block
	participantsLock.WaitUnusedAndLock();
	
	//El iterator
	Participants::iterator it = participants.find(id);

	//Si no esta
	if (it == participants.end())
	{
		//Unlock
		participantsLock.Unlock();
		//Exit
		return Error("Participant not found\n");
	}

	//LO obtenemos
	Participant *part = it->second;
	part->SetGroupVideoStream(NULL);


	Log("-DeleteParticipant ending media [%d]\n",id);

	//Y lo quitamos del mapa
	participants.erase(it);

	//Unlock
	participantsLock.Unlock();

	//Destroy participatn
	DestroyParticipant(id,part);
	
//Terminamos el audio y el video
	if(m_CurrentBroadCaster == id && participants.size())
	{
		Log("BroadCaster exist use the first coming user\n");
		participants.begin()->second->SetGroupVideoStream(&m_GroupVideo);
		m_CurrentBroadCaster = participants.begin()->second->GetPartId();
		participants.begin()->second->SendVideoFPU();
	} else if(m_CurrentBroadCaster == id && participants.size() == 0) {
		Log("BroadCaster and no user left\n");
		m_CurrentBroadCaster = 0;
	}

	Log("<DeleteParticipant [%d]\n",id);

	return 1;
}

void MultiConf:: DestroyParticipant(int partId,Participant* part)
{
	Log(">DestroyParticipant [%d]\n",partId);

	//End participant audio and video streams
	part->End();

	Log("-DestroyParticipant ending mixers [%d]\n",partId);

	//End participant mixers
	videoMixer.EndMixer(partId);
	audioMixer.EndMixer(partId);
	textMixer.EndMixer(partId);

	Log("-DestroyParticipant deleting mixers [%d]\n",partId);

	//QUitamos los mixers
	videoMixer.DeleteMixer(partId);
	audioMixer.DeleteMixer(partId);
	textMixer.DeleteMixer(partId);
	m_GroupVideo.StopSending(partId);
	m_GroupAudio.StopSending(partId);
	//Delete participant
	delete part;

	Log("<DestroyParticipant [%d]\n",partId);

}

Participant *MultiConf::GetParticipant(int partId)
{
	//Find participant
	Participants::iterator it = participants.find(partId);

	//If not found
	if (it == participants.end())
		//Error
		return (Participant *)Error("Participant not found\n");

	//Get the participant
	return it->second;
}

Participant *MultiConf::GetParticipant(int partId,Participant::Type type)
{
	//Find participant
	Participant *part = GetParticipant(partId);

	//If no participant
	if (!part)
		//Exit
		return NULL;

	//Ensure it is from the correct type
	if (part->GetType()!=type)
		//Error
		return (Participant *)Error("Participant is not of desired type");
	
	//Return it
	return part;
}

/************************
* End
* 	Termina una multiconferencia
*************************/
int MultiConf::End()
{
	Log(">End multiconf\n");

	//End watchers
	StopBroadcaster();

	//End mixers
	audioMixer.EndMixer(watcherId);
	textMixer.EndMixer(watcherId);

	//Stop encoding
	audioEncoder.StopEncoding();
	textEncoder.StopEncoding();

	//End encoders
	audioEncoder.End();
	textEncoder.End();

	//End mixers
	audioMixer.DeleteMixer(watcherId);
	textMixer.DeleteMixer(watcherId);

	//Get lock
	participantsLock.WaitUnusedAndLock();
	
	//Destroy all participants
	for(Participants::iterator it=participants.begin(); it!=participants.end(); it++)
		//Destroy it
		DestroyParticipant(it->first,it->second);

	//Clear list
	participants.clear();
	
	//Unlock
	participantsLock.Unlock();

	//Remove all players
	while(players.size()>0)
		//Delete the first one
		DeletePlayer(players.begin()->first);

	Log("-End conference mixers\n");

	//Terminamos los mixers
	videoMixer.End();
	audioMixer.End();
	textMixer.End();

	//No inicado
	inited = 0;

	Log("<End multiconf\n");
}

/************************
* SetVideoCodec
* 	SetVideoCodec
*************************/
int MultiConf::SetVideoCodec(int id,int codec,int mode,int fps,int bitrate,int quality, int fillLevel,int intraPeriod)
{
	int ret = 0;

	Log("-SetVideoCodec [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	Participant *part = GetParticipant(id);

	if (!m_GroupVideo.IsInited())
	{
		m_GroupVideo.SetVideoCodec((VideoCodec::Type)codec,mode,fps,bitrate,quality,fillLevel,intraPeriod);
	}

	//Check particpant
	if (part)
		//Set video codec
		ret = part->SetVideoCodec((VideoCodec::Type)codec,mode,fps,bitrate,quality,fillLevel,intraPeriod);

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}


int MultiConf::StartSendingVideoSpy(int spyId,char *sendVideoIp,int sendVideoPort,VideoCodec::RTPMap& rtpMap)
{
	int ret = 0;
	RTPSession *rtp = NULL;
	if(spys.find(spyId) == spys.end())
		return 0;
	
	int spyeeId = spys[spyId];	

	Log("-StartSendingVideoSpy [%d]\n",spyeeId);
	//Use list
	participantsLock.IncUse();
	RTPParticipant *part = (RTPParticipant*)GetParticipant(spyeeId,Participant::RTP);
	
	//Check particpant
	if (part)
		//Set video codec
		rtp = part->StartSendingVideoSpy(spyId, sendVideoIp, sendVideoPort, rtpMap);


	//Unlock
	participantsLock.DecUse();
	if(rtp != NULL)
		ret = 1;
	//Start sending the video
	//return ((RTPParticipant*)part)->StartSendingVideo(sendVideoIp,sendVideoPort,rtpMap);
	return ret;

}

/************************
* StartSendingVideo
* 	StartSendingVideo
*************************/
RTPSession *MultiConf::StartSendingVideo(int id,char *sendVideoIp,int sendVideoPort,VideoCodec::RTPMap& rtpMap)
{
	int ret = 0;

	Log("-StartSendingVideo [%d]\n",id);
	//Use list
	participantsLock.IncUse();
	Participants::iterator it = participants.find(id);
	RTPSession *rtp = NULL;
	//Si no esta
	if (it == participants.end()) {
		Log("Participant is not of RTP type, may be from rtsp.\n");
		rtp = m_GroupVideo.StartSending(id,sendVideoIp,sendVideoPort,rtpMap, NULL);
		SendIdrPacket(rtp);
		participantsLock.DecUse();
		return rtp;
	}
	
	 //Get the participant
        Participant *part = (*it).second;

	//Ensure it is from the correct type
	if (part->GetType()!=Participant::RTP)
		//Not possible
		Log("Participant is not of RTP type, may be from rtsp.\n");

	rtp = ((RTPParticipant*)part)->GetVideoRTPSession();
	m_GroupVideo.StartSending(id,sendVideoIp,sendVideoPort,rtpMap, rtp);
	if(m_CurrentBroadCaster != id)
		SendIdrPacket(rtp);

	//Unlock
	participantsLock.DecUse();
	//Start sending the video
	//return ((RTPParticipant*)part)->StartSendingVideo(sendVideoIp,sendVideoPort,rtpMap);
	return rtp;
}

void MultiConf::SendIdrPacket(RTPSession *rtp)
{
	Participants::iterator itBroadcaster = participants.find(m_CurrentBroadCaster);
	if(rtp != NULL && itBroadcaster != participants.end())
	{
		RTPParticipant *broadCaster = (RTPParticipant*)itBroadcaster->second;
		Log("Send idr packet to newly added video\n");
		IDRPacketSize idrPacketSize = broadCaster->GetIdrPacketSize();
		IDRPacket idrPacket = broadCaster->GetIdrPacket();
		DWORD currentTimeStamp = broadCaster->GetCurrentTimestamp();
		size_t packetSize = idrPacket.size();
		for (size_t i = 0; i< packetSize ; ++i) 
		{
			BYTE *packet = idrPacket[i];
			int length = idrPacketSize[i];
			if(i == packetSize - 1)
			{
				rtp->ForwardPacket(packet, length, 1, currentTimeStamp);
			}
			else
			{
				rtp->ForwardPacket(packet, length, 0, currentTimeStamp);
			}
		}
	}
}

/************************
* StopSendingVideo
* 	StopSendingVideo
*************************/
int MultiConf::StopSendingVideo(int id)
{
	int ret = 0;

	Log("-StopSendingVideo [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	 //El iterator
        Participants::iterator it = participants.find(id);

	//Si no esta
	if (it == participants.end()) {
		Log("Participant is not of RTP type, may be from rtsp.\n");
		m_GroupVideo.StopSending(id);
		participantsLock.DecUse();
		return 1;
	}
	
	//Get the participant
        Participant *part = (*it).second;

	//Ensure it is from the correct type
	if (part->GetType()!=Participant::RTP)
		//Not possible
		Log("Participant is not of RTP type, may be from rtsp.\n");

	
	m_GroupVideo.StopSending(id);
	//Unlock
	participantsLock.DecUse();

	return 1;
	//Stop sending the video
	//return ((RTPParticipant*)part)->StopSendingVideo();
}


int MultiConf::StartReceivingVideoSpy(int spyId, VideoCodec::RTPMap& rtpMap)
{
	int ret = 0;
	
	Log("-StartReceivingVideoSpy [%d]\n",spyId);

	if(spys.find(spyId) == spys.end())
		return 0;
	
	int spyeeId = spys[spyId];
	//Use list
	participantsLock.IncUse();

	//Get the participant
	RTPParticipant *part = (RTPParticipant*)GetParticipant(spyeeId,Participant::RTP);

	//Check particpant
	if (part)
		//Set video codec
		ret = part->StartReceivingVideoSpy(spyId, rtpMap);

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;

}

/************************
* StartReceivingVideo
* 	StartReceivingVideo
*************************/
int MultiConf::StartReceivingVideo(int id,VideoCodec::RTPMap& rtpMap)
{
	int ret = 0;

	Log("-StartReceivingVideo [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	RTPParticipant *part = (RTPParticipant*)GetParticipant(id,Participant::RTP);

	//Check particpant
	if (part)
		//Set video codec
		ret = part->StartReceivingVideo(rtpMap);

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

/************************
* StopReceivingVideo
* 	StopReceivingVideo
*************************/
int MultiConf::StopReceivingVideo(int id)
{
	int ret = 0;

	Log("-StopReceivingVideo [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	RTPParticipant *part = (RTPParticipant*)GetParticipant(id,Participant::RTP);

	//Check particpant
	if (part)
		//Set video codec
		ret = part->StopReceivingVideo();

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

/************************
* SetAudioCodec
* 	SetAudioCodec
*************************/
int MultiConf::SetAudioCodec(int id,int codec)
{
	int ret = 0;

	Log("-SetAudioCodec [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	Participant *part = GetParticipant(id);

	//Check particpant
	if (part)
		//Set video codec
		ret = part->SetAudioCodec((AudioCodec::Type)codec);
	if (!m_GroupAudio.IsInited())
	{
		m_GroupAudio.SetAudioCodec((AudioCodec::Type)codec);
	}

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

/************************
* StartSendingAudio
* 	StartSendingAudio
*************************/
RTPSession *MultiConf::StartSendingAudio(int id,char *sendAudioIp,int sendAudioPort,AudioCodec::RTPMap& rtpMap)
{
	int ret = 0;

	Log("-StartSendingAudio [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//El iterator
        Participants::iterator it = participants.find(id);

	//Si no esta
	if (it == participants.end()) {
		Log("Participant is not of RTP type, may be from rtsp.\n");
		RTPSession *rtp = m_GroupAudio.StartSending(id,sendAudioIp,sendAudioPort,rtpMap, NULL);
		participantsLock.DecUse();
		return rtp;
	}

 	//Get the participant
        Participant *part = (*it).second;

	//Ensure it is from the correct type
	if (part->GetType()!=Participant::RTP)
		//Not possible
		Log("Participant is not of RTP type, may be from rtsp.\n");
	
	//Unlock
	participantsLock.DecUse();
	((RTPParticipant*)part)->StartSendingAudio(sendAudioIp,sendAudioPort,rtpMap);
	//m_GroupAudio.StartSending(id,sendAudioIp,sendAudioPort,rtpMap, ((RTPParticipant*)part)->GetAudioRTPSession());
	
	//return 1;
	//Start sending the video
	return ((RTPParticipant*)part)->GetAudioRTPSession();
}

/************************
* StopSendingAudio
* 	StopSendingAudio
*************************/
int MultiConf::StopSendingAudio(int id)
{
	int ret = 0;

	Log("-StopSendingAudio [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//El iterator
        Participants::iterator it = participants.find(id);

	//Si no esta
	if (it == participants.end()) {
		Log("Participant is not of RTP type, may be from rtsp.\n");
		m_GroupAudio.StopSending(id);
		participantsLock.DecUse();
		return 1;
	}

	//Get the participant
        Participant *part = (*it).second;

	//Ensure it is from the correct type
	if (part->GetType()!=Participant::RTP)
		//Not possible
		Log("Participant is not of RTP type, may be from rtsp.\n");

	//Unlock
	participantsLock.DecUse();

	//m_GroupAudio.StopSending(id);
	//return 1;
	//Start sending the audio
	return ((RTPParticipant*)part)->StopSendingAudio();
}

/************************
* StartReceivingAudio
* 	StartReceivingAudio
*************************/
int MultiConf::StartReceivingAudio(int id,AudioCodec::RTPMap& rtpMap)
{
	int ret = 0;

	Log("-StartReceivingAudio [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	RTPParticipant *part = (RTPParticipant*)GetParticipant(id,Participant::RTP);

	//Check particpant
	if (part)
		//Set video codec
		ret = part->StartReceivingAudio(rtpMap);

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

/************************
* End
* 	Termina una multiconferencia
*************************/
int MultiConf::StopReceivingAudio(int id)
{
	int ret = 0;

	Log("-StopReceivingAudio [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	RTPParticipant *part = (RTPParticipant*)GetParticipant(id,Participant::RTP);

	//Check particpant
	if (part)
		//Set video codec
		ret = part->StopReceivingAudio();

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

/************************
* SetTextCodec
* 	SetTextCodec
*************************/
int MultiConf::SetTextCodec(int id,int codec)
{
	int ret = 0;

	Log("-SetTextCodec [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	Participant *part = GetParticipant(id);

	//Check particpant
	if (part)
		//Set video codec
		ret = part->SetTextCodec((TextCodec::Type)codec);

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

/************************
* StartSendingText
* 	StartSendingText
*************************/
int MultiConf::StartSendingText(int id,char *sendTextIp,int sendTextPort,TextCodec::RTPMap& rtpMap)
{
	int ret = 0;

	Log("-StartSendingText [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	RTPParticipant *part = (RTPParticipant*)GetParticipant(id,Participant::RTP);

	//Check particpant
	if (part)
		//Set video codec
		ret = part->StartSendingText(sendTextIp,sendTextPort,rtpMap);

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

/************************
* StopSendingText
* 	StopSendingText
*************************/
int MultiConf::StopSendingText(int id)
{
	int ret = 0;

	Log("-StopSendingText [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	RTPParticipant *part = (RTPParticipant*)GetParticipant(id,Participant::RTP);

	//Check particpant
	if (part)
		//Set video codec
		ret = part->StopSendingText();

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

/************************
* StartReceivingText
* 	StartReceivingText
*************************/
int MultiConf::StartReceivingText(int id,TextCodec::RTPMap& rtpMap)
{
	int ret = 0;

	Log("-StartReceivingText [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	RTPParticipant *part = (RTPParticipant*)GetParticipant(id,Participant::RTP);

	//Check particpant
	if (part)
		//Set video codec
		ret = part->StartReceivingText(rtpMap);

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

/************************
* End
* 	Termina una multiconferencia
*************************/
int MultiConf::StopReceivingText(int id)
{
	int ret = 0;

	Log("-StopReceivingText [%d]\n",id);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	RTPParticipant *part = (RTPParticipant*)GetParticipant(id,Participant::RTP);

	//Check particpant
	if (part)
		//Set video codec
		ret = part->StopReceivingText();

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}
/************************
* SetParticipantMosaic
* 	Change participant mosaic
*************************/
int MultiConf::SetParticipantMosaic(int partId,int mosaicId)
{
	int ret = 0;

	Log("-SetParticipantMosaic [partId:%d,mosaic:%d]\n",partId,mosaicId);

	//Use list
	participantsLock.IncUse();

	//Get the participant
	Participant *part = GetParticipant(partId);

	//Check particpant
	if (part)
		//Set it in the video mixer
		ret =  videoMixer.SetMixerMosaic(partId,mosaicId);

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

/************************
* CreatePlayer
* 	Create a media player
*************************/
int MultiConf::CreatePlayer(int privateId,std::wstring name)
{
	Log(">CreatePlayer [%d]\n",privateId);


	//SI no tamos iniciados pasamos
	if (!inited)
		return Error("Not inited\n");

	//Obtenemos el id
	int playerId = maxId++;

	//Le creamos un mixer
	if (!videoMixer.CreateMixer(playerId))
		return Error("Couldn't set video mixer\n");

	//Y el de audio
	if (!audioMixer.CreateMixer(playerId))
	{
		//Borramos el de video
		videoMixer.DeleteMixer(playerId);
		//Y salimos
		return Error("Couldn't set audio mixer\n");
	}

	//Add a pivate text
	if (!textMixer.CreatePrivate(playerId,privateId,name))
	{
		//Borramos el de video y audio
		videoMixer.DeleteMixer(playerId);
		audioMixer.DeleteMixer(playerId);
		//Y salimos
		return Error("Couldn't set text mixer\n");
	}

	//Create player
	MP4Player *player = new MP4Player();

	//Init
	player->Init(audioMixer.GetOutput(playerId),videoMixer.GetOutput(playerId),textMixer.GetOutput(playerId));

	//E iniciamos el mixer
	videoMixer.InitMixer(playerId,-1);
	audioMixer.InitMixer(playerId);
	textMixer.InitPrivate(playerId);

	//Lo insertamos en el map
	players[playerId] = player;

	Log("<CreatePlayer [%d]\n",playerId);

	return playerId;
}
/************************
* StartPlaying
* 	Start playing the media in the player
*************************/
int MultiConf::StartPlaying(int playerId,const char* filename,bool loop)
{
	Log("-Start playing [id:%d,file:\"%s\",loop:%d]\n",playerId,filename,loop);

	//Find it
	Players::iterator it = players.find(playerId);

	//Si no esta
	if (it == players.end())
		//Not found
		return Error("-Player not found\n");

	//Play
	return it->second->Play(filename,loop);
}
/************************
* StopPlaying
* 	Stop the media playback
*************************/
int MultiConf::StopPlaying(int playerId)
{
	Log("-Stop playing [id:%d]\n",playerId);

	//Find it
	Players::iterator it = players.find(playerId);

	//Si no esta
	if (it == players.end())
		//Not found
		return Error("-Player not found\n");

	//Play
	return it->second->Stop();
}

/************************
* DeletePlayer
* 	Delete a media player
*************************/
int MultiConf::DeletePlayer(int id)
{
	Log(">DeletePlayer [%d]\n",id);


	//El iterator
	Players::iterator it = players.find(id);

	//Si no esta
	if (it == players.end())
		//Not found
		return Error("-Player not found\n");

	//LO obtenemos
	MP4Player *player = (*it).second;

	//Y lo quitamos del mapa
	players.erase(it);

	//Terminamos el audio y el video
	player->Stop();

	Log("-DeletePlayer ending mixers [%d]\n",id);

	//Paramos el mixer
	videoMixer.EndMixer(id);
	audioMixer.EndMixer(id);
	textMixer.EndPrivate(id);

	//End it
	player->End();

	//QUitamos los mixers
	videoMixer.DeleteMixer(id);
	audioMixer.DeleteMixer(id);
	textMixer.DeletePrivate(id);

	//Lo borramos
	delete player;

	Log("<DeletePlayer [%d]\n",id);

	return 1;
}

int MultiConf::StartRecordingParticipant(int partId,const char* filename)
{
	int ret = 0;
	
	Log("-StartRecordingParticipant [id:%d,name:\"%s\"]\n",partId,filename);

	//Lock
	participantsLock.IncUse();

	//Get participant
	RTPParticipant *rtp = (RTPParticipant*)GetParticipant(partId,Participant::RTP);

	//Check if
	if (!rtp)
		//End
		goto end;
	
	//Start recording
	rtp->recorder.Init();

	//Start recording
	if (!rtp->recorder.Record(filename))
		//End
		goto end;
	
	//Set the listener for the rtp video packets
	rtp->SetMediaListener(&rtp->recorder);

	//Add the listener for audio and text frames of the watcher
	audioEncoder.AddListener(&rtp->recorder);
	textEncoder.AddListener(&rtp->recorder);

	//OK
	ret = 1;

end:
	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

int MultiConf::StopRecordingParticipant(int partId)
{
	int ret = 0;
	
	Log("-StopRecordingParticipant [id:%d]\n",partId);

	//Lock
	participantsLock.IncUse();

	//Get rtp participant
	RTPParticipant* rtp = (RTPParticipant*)GetParticipant(partId,Participant::RTP);

	//Check participant
	if (rtp)
	{
		//Set the listener for the rtp video packets
		rtp->SetMediaListener(NULL);

		//Add the listener for audio and text frames of the watcher
		audioEncoder.RemoveListener(&rtp->recorder);
		textEncoder.RemoveListener(&rtp->recorder);

		//Stop recording
		rtp->recorder.Stop();

		//End recording
		ret = rtp->recorder.End();
	}

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

int MultiConf::SendFPU(int partId)
{
	int ret = 0;
	
	Log("-SendFPU [id:%d]\n",partId);
	
	//Lock
	participantsLock.IncUse();

	//Get participant
	Participant *part = GetParticipant(partId);

	//Check participant
	if (part)
		//Send FPU
		ret = part->SendVideoFPU();
	if(m_CurrentBroadCaster != 0)
	{
		Log("-RequestFPU [id:%d]\n",m_CurrentBroadCaster);
		listener->onParticipantRequestFPU(this,m_CurrentBroadCaster,this->param);
	}
	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

MultiConf::ParticipantStatistics* MultiConf::GetParticipantStatistic(int partId)
{
	//Create statistics map
	ParticipantStatistics *stats = new ParticipantStatistics();

	//Lock
	participantsLock.IncUse();

	//Find participant
	Participant* part = GetParticipant(partId);

	//Check participant
	if (part)
	{
		//Append
		(*stats)["audio"] = part->GetStatistics(MediaFrame::Audio);
		(*stats)["video"] = part->GetStatistics(MediaFrame::Video);
		(*stats)["text"]  = part->GetStatistics(MediaFrame::Text);
	}

	//Unlock
	participantsLock.DecUse();

	//Return stats
	return stats;
}

/********************************************************
 * SetMute
 *   Set participant mute
 ********************************************************/
int MultiConf::SetMute(int partId,MediaFrame::Type media,bool isMuted)
{
	int ret = 0;

	Log("-SetMute [id:%d,media:%d,muted:%d]\n",partId,media,isMuted);

	//Lock
	participantsLock.IncUse();

	//Get participant
	Participant *part = GetParticipant(partId);

	//Check participant
	if (part)
		//Send FPU
		ret = part->SetMute(media,isMuted);

	//Unlock
	participantsLock.DecUse();

	//Exit
	return ret;
}

/********************************************************
 * AddConferenceToken
 *   Add a token for conference watcher
 ********************************************************/
bool MultiConf::AddBroadcastToken(const std::wstring &token)
{
	Log("-AddBroadcastToken [token:\"%ls\"]\n",token.c_str());

	//Check if the pin is already in use
	if (tokens.find(token)!=tokens.end())
		//Broadcast not found
		return Error("Token already in use\n");

	//Add to the pin list
	tokens.insert(token);

	return true;
}
/********************************************************
 * AddParticipantInputToken
 *   Add a token for participant input
 ********************************************************/
bool  MultiConf::AddParticipantInputToken(int partId,const std::wstring &token)
{
	Log("-AddParticipantInputToken [id:%d,token:\"%ls\"]\n",partId,token.c_str());

	//Check if the pin is already in use
	if (tokens.find(token)!=tokens.end())
		//Broadcast not found
		return Error("Token already in use\n");

	//Add to the pin list
	inputTokens[token] = partId;

	return true;
}
/********************************************************
 * AddParticipantOutputToken
 *   Add a token for participant output
 ********************************************************/
bool  MultiConf::AddParticipantOutputToken(int partId,const std::wstring &token)
{
	Log("-AddParticipantOutputToken [id:%d,token:\"%ls\"]\n",partId,token.c_str());

	//Check if the pin is already in use
	if (tokens.find(token)!=tokens.end())
		//Broadcast not found
		return Error("Token already in use\n");

	//Add to the pin list
	outputTokens[token] = partId;

	return true;
}

/********************************************************
 * ConsumeBroadcastToken
 *   Check and consume a token for conference watcher
 ********************************************************/
bool MultiConf::ConsumeBroadcastToken(const std::wstring &token)
{
	//Check token
	BroadcastTokens::iterator it = tokens.find(token);

	//Check we found one
	if (it==tokens.end())
		//Broadcast not found
		return false;

	//Remove token
	tokens.erase(it);

	//It is valid
	return true;
}

/********************************************************
 * ConsumeParticipantInputToken
 *   Check and consume a token for conference watcher
 ********************************************************/
Participant* MultiConf::ConsumeParticipantInputToken(const std::wstring &token)
{
	//Check token
	ParticipantTokens::iterator it = inputTokens.find(token);

	//Check we found one
	if (it==inputTokens.end())
		//Broadcast not found
		return NULL;

	//Get participant id
	int partId = it->second;

	//Remove token
	inputTokens.erase(it);

	//Get it
	Participants::iterator itPart = participants.find(partId);

	//Check if not found
	if (itPart==participants.end())
		//Not found
		return NULL;
	
	//Get it
	Participant* part = itPart->second;

	//Asert correct tipe
	if (part->GetType()!=Participant::RTMP)
		//Esit
		return NULL;

	//return it
	return part;
}

/********************************************************
 * ConsumeBroadcastToken
 *   Check and consume a token for conference watcher
 ********************************************************/
Participant* MultiConf::ConsumeParticipantOutputToken(const std::wstring &token)
{
	//Check token
	ParticipantTokens::iterator it = outputTokens.find(token);

	//Check we found one
	if (it==outputTokens.end())
		//Broadcast not found
		return NULL;

	//Get participant id
	int partId = it->second;

	//Remove token
	outputTokens.erase(it);

	//Get it
	Participants::iterator itPart = participants.find(partId);

	//Check if not found
	if (itPart==participants.end())
		//Not found
		return NULL;

	//Get it
	Participant* part = itPart->second;

	//Asert correct tipe
	if (part->GetType()!=Participant::RTMP)
		//Esit
		return NULL;

	//return it
	return part;
}

// rtsp add on by liuhong

RTPSession *MultiConf::StartRecvingVideoRtcp(int id)
{
	Log("-StartRecvingVideoRtcp [%d]\n",id);
	RTPSession *rtp = NULL;
	Log("Get rtp and rtcp port for rtsp video streaming.\n");
	rtp = m_GroupVideo.StartReceivingRtcp(id);
	return rtp;
}


RTPSession *MultiConf::StartRecvingAudioRtcp(int id)
{
	Log("-StartRecvingAudioRtcp [%d]\n",id);
	RTPSession *rtp = NULL;
	Log("Get rtp and rtcp port for rtsp audio streaming.\n");
	rtp = m_GroupAudio.StartReceivingRtcp(id);
	return rtp;
}



/*****************************************************
 * RTMP Broadcast session
 *
 ******************************************************/
MultiConf::BroadcastStream::BroadcastStream(DWORD streamId,MultiConf *conf) : RTMPStream(streamId)
{
	//Store mcu
	this->conf = conf;
}

MultiConf::BroadcastStream::~BroadcastStream()
{
	//Remove conference watcher
}

/***************************************
 * Play
 *	RTMP event listener
 **************************************/
bool MultiConf::BroadcastStream::Play(std::wstring& url)
{
	Log("-Playing mcu watcher session [url:\"%ls\"]\n",url.c_str());

	//Check we found one
	if (!conf->ConsumeBroadcastToken(url))
	{
		//Send error
		SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Play.StreamNotFound",L"error",L"Token not found"));
		//Broadcast not found
		return Error("Broadcaster token not found in conference [%ls]\n",url.c_str());
	} 

	//Start stream
	SendStreamBegin();

	//Add us to the receiver for the multicong
	conf->AddBroadcastReceiver(this);

	//Everything ok
	return true;
}

bool MultiConf::BroadcastStream::Seek(DWORD time)
{
	//Send error
	SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Seek.Failed",L"error",L"Seek not supported"));
	return false;
}
/***************************************
 * Publish
 *	RTMP event listener
 **************************************/
bool MultiConf::BroadcastStream::Publish(std::wstring& url)
{
	//Do not support it yet
	return false;
}

/***************************************
 * Close
 *	RTMP event listener
 **************************************/
bool MultiConf::BroadcastStream::Close()
{
	Log("-Stop mcu watcher session\n");

	//Start stream
	SendStreamEnd();

	//Add the broadcast receiver for the multicong
	conf->RemoveBroadcastReceiver(this);

	//Everything ok
	return true;
}

void MultiConf::onRequestFPU(Participant *part)
{
	//Check listener
	if (listener)
		//Send event
		listener->onParticipantRequestFPU(this,part->GetPartId(),this->param);
}
