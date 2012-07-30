/* 
 * File:   mediagateway.cpp
 * Author: Sergio
 * 
 * Created on 22 de diciembre de 2010, 18:10
 */
#include "log.h"
#include "mediagateway.h"

MediaGateway::MediaGateway()
{
	//Inicializamos los mutex
	pthread_mutex_init(&mutex,NULL);
}

MediaGateway::~MediaGateway()
{
	//Destroy mutex
	pthread_mutex_destroy(&mutex);
}


/**************************************
* Init
*	Inititalize the media gateway server
**************************************/
bool MediaGateway::Init()
{
	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Estamos iniciados
	inited = true;

	//El id inicial
	maxId=100;

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	//Salimos
	return inited;
}

/**************************************
* End
*	Ends media gateway server
**************************************/
bool MediaGateway::End()
{
	Log(">End MediaGateway\n");

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Dejamos de estar iniciados
	inited = false;

	//Paramos las sesions
	for (MediaBridgeEntries::iterator it=bridges.begin(); it!=bridges.end(); it++)
	{
		//Obtenemos la sesion
		MediaBridgeSession *session = it->second.session;

		//La paramos
		session->End();

		//Y la borramos
		delete session;
	}

	//Clear the MediaGateway list
	bridges.clear();

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	Log("<End MediaGateway\n");

	//Salimos
	return !inited;
}

/**************************************
* CreateMediaBridge
*	Create a media bridge session
**************************************/
DWORD MediaGateway::CreateMediaBridge(const std::wstring &name)
{
	Log("-CreateBroadcast [name:\"%ls\"]\n",name.c_str());


	//Creamos la session
	MediaBridgeSession * session = new MediaBridgeSession();

	//Obtenemos el id
	DWORD sessionId = maxId++;

	//Creamos la entrada
	MediaBridgeEntry entry;

	//Guardamos los datos
	entry.id 	= sessionId;
	entry.name 	= name;
        entry.session 	= session;
	entry.enabled 	= 1;
	entry.numRef	= 0;

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//a�adimos a la lista
	bridges[sessionId] = entry;

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	return sessionId;
}

/**************************************
 * SetMediaBridgeInputToken
 *	Associates a token with a media bridge for input
 *	In case there is already a pin associated with that session it fails.
 **************************************/
bool MediaGateway::SetMediaBridgeInputToken(DWORD id,const std::wstring &token)
{
	Log(">SetMediaBridgeInputToken [id:%d,token:\"%ls\"]\n",id,token.c_str());

	bool res = false;

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Get the MediaGateway session entry
	MediaBridgeEntries::iterator it = bridges.find(id);

	//Check it
	if (it==bridges.end())
	{
		//Broadcast not found
		Error("Media bridge session not found\n");
		//Exit
		goto end;
	}

	//Check the input is already in use
	if (!it->second.inputToken.empty())
	{
		//Broadcast not found
		Error("Media bridge session already published\n");
		//Exit
		goto end;
	}

	//Check if the pin is already in use
	if (inputTokens.find(token)!=inputTokens.end())
	{
		//Broadcast not found
		Error("Input token already in use by other media gateway session\n");
		//Exit
		goto end;
	}

	//Add to the input token list
	inputTokens[token] = id;

	//Set the entry broadcast session publication pin
	it->second.inputToken = token;

	//Everything was ok
	res = true;

end:
	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	Log("<SetMediaBridgeInputToken\n");

	return res;
}

/**************************************
 * SetMediaBridgeOutputToken
 *	Associates a token with a media bridge for input
 *	In case there is already a pin associated with that session it fails.
 **************************************/
bool MediaGateway::SetMediaBridgeOutputToken(DWORD id,const std::wstring &token)
{
	Log(">SetMediaBridgeOutputToken [id:%d,token:\"%ls\"]\n",id,token.c_str());

	bool res = false;

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Get the MediaGateway session entry
	MediaBridgeEntries::iterator it = bridges.find(id);

	//Check it
	if (it==bridges.end())
	{
		//Broadcast not found
		Error("Media bridge session not found\n");
		//Exit
		goto end;
	}

	//Check the input is already in use
	if (!it->second.outputToken.empty())
	{
		//Broadcast not found
		Error("Media bridge session already published\n");
		//Exit
		goto end;
	}

	//Check if the pin is already in use
	if (outputTokens.find(token)!=outputTokens.end())
	{
		//Broadcast not found
		Error("Output token already in use by other media gateway session\n");
		//Exit
		goto end;
	}

	//Add to the input token list
	outputTokens[token] = id;

	//Set the entry media bridge session output token
	it->second.outputToken = token;

	//Everything was ok
	res = true;

end:
	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	Log("<SetMediaBridgeOutputToken\n");

	return res;
}

/**************************************
 * GetMediaBridgeIdFromInputToken
 *   Get the id of the session for the bridge session from the input token
 * ************************************/
DWORD MediaGateway::GetMediaBridgeIdFromInputToken(const std::wstring &token)
{
	DWORD sessionId = 0;

	Log(">GetMediaBridgeIdFromInputToken [token:\"%ls\"]\n",token.c_str());

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Check if there is already a pin like that
	InputMediaBridgesTokens::iterator it = inputTokens.find(token);

	//Check we found one
	if (it!=inputTokens.end())
	{
		//Get id
		sessionId = it->second;
		//Remove token
		inputTokens.erase(it);
	}

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	Log("<GetMediaBridgeIdFromInputToken [sessionId:%d]\n",sessionId);

	//Return session id
	return sessionId;
}


/**************************************
 * GetMediaBridgeIdFromOutputToken
 *   Get the id of the session for the bridge session from the input token
 * ************************************/
DWORD MediaGateway::GetMediaBridgeIdFromOutputToken(const std::wstring &token)
{
	DWORD sessionId = 0;

	Log(">GetMediaBridgeIdFromOutputToken [token:\"%ls\"]\n",token.c_str());

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Check if there is already a pin like that
	InputMediaBridgesTokens::iterator it = outputTokens.find(token);

	//Check we found one
	if (it!=outputTokens.end())
	{
		//Get id
		sessionId = it->second;
		//Remove token
		outputTokens.erase(it);
	}

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	Log("<GetMediaBridgeIdFromOutputToken [sessionId:%d]\n",sessionId);

	//Return session id
	return sessionId;
}

/**************************************
* GetMediaBridgeRef
*	Obtiene una referencia a una sesion
**************************************/
bool MediaGateway::GetMediaBridgeRef(DWORD id,MediaBridgeSession **session)
{
	Log(">GetMediaBridgeRef [%d]\n",id);

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Get it
	MediaBridgeEntries::iterator it = bridges.find(id);

	//SI no esta
	if (it==bridges.end())
	{
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);
		//Y salimos
		return Error("Session no encontrada [%d]\n",id);
	}

	//Get bridge entry
	MediaBridgeEntry &entry = it->second;

	//Increase counter reference
	entry.numRef++;

	//Y obtenemos el puntero a la la sesion
	*session = entry.session;

	//Desbloquamos el mutex
	pthread_mutex_unlock(&mutex);

	Log("<GetMediaBridgeRef \n");

	return true;
}

/**************************************
* ReleaseMediaBridgeRef
*	Libera una referencia a una sesion
**************************************/
bool MediaGateway::ReleaseMediaBridgeRef(DWORD id)
{
	Log(">ReleaseMediaBridgeRef [%d]\n",id);

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Get it
	MediaBridgeEntries::iterator it = bridges.find(id);

	//SI no esta
	if (it==bridges.end())
	{
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);
		//Y salimos
		return Error("Session no encontrada [%d]\n",id);
	}

	//Get bridge entry
	MediaBridgeEntry &entry = it->second;

	//Decrease counter
	entry.numRef--;

	//Desbloquamos el mutex
	pthread_mutex_unlock(&mutex);

	Log("<ReleaseMediaBridgeRef\n");

	return true;
}

/**************************************
* DeleteSession
*	Inicializa el servidor de FLV
**************************************/
bool MediaGateway::DeleteMediaBridge(DWORD id)
{
	Log(">DeleteMediaBridge [%d]\n",id);

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Find sessionerence
	MediaBridgeEntries::iterator it = bridges.find(id);

	//Check if we found it or not
	if (it==bridges.end())
	{
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);

		//Y salimos
		return Error("Session no encontrada [%d]\n",id);
	}

	//TODO: Wait for numref == 0

	//Get entry
	MediaBridgeEntry &entry = it->second;

	//Get sessionerence
	MediaBridgeSession *session = entry.session;

	//If it has tokens
	if (!entry.inputToken.empty())
		//Remove from input list
		inputTokens.erase(entry.inputToken);
	//If it has tokens
	if (!entry.outputToken.empty())
		//Remove from output list
		outputTokens.erase(entry.outputToken);

	//Remove entry from list
	bridges.erase(it);

	//Unset transmitter
	session->UnsetTransmitter();
	//Unset receiver
	session->UnsetReceiver();

	Log("-Ending session [%d]\n",id);

	//End session
	session->End();

	//Delete sessionerence
	delete session;

	//Desbloquamos el mutex
	pthread_mutex_unlock(&mutex);

	Log("<DeleteMediaBridge [%d]\n",id);

	//Exit
	return true;
}

RTMPStream* MediaGateway::CreateStream(DWORD streamId,std::wstring &appName,DWORD audioCaps,DWORD videoCaps)
{
	//Check app name
	if (appName.compare(L"bridge/input")==0)
	{
		Log("-Creating transmitter stream\n");
		//A publishing stream
		return new MediaBridgeInputStream(streamId,this);
	} else if (appName.compare(L"bridge/output")==0) {
		Log("-Creating receiver stream\n");
		//A receiver stream
		return new MediaBridgeOutputStream(streamId,this);
	} else {
		Log("-Application name incorrect [%ls]\n",appName.c_str());
		//No stream for that url
		return NULL;
	}

}

/************************************
 * MediaBridgeInputStream
 *
 ************************************/
MediaGateway::MediaBridgeInputStream::MediaBridgeInputStream(DWORD streamId,MediaGateway *MediaGateway) : RTMPStream(streamId)
{
	//Store the broarcaster
	this->mediaGateway = MediaGateway;
	//Not publishing
	sesId = 0;
}

MediaGateway::MediaBridgeInputStream::~MediaBridgeInputStream()
{
	//If we are still publishing
	if (sesId)
		//Close it
		Close();
}

bool MediaGateway::MediaBridgeInputStream::Play(std::wstring& url)
{
	//Not supported
	return false;
}

bool MediaGateway::MediaBridgeInputStream::Seek(DWORD time)
{
	//Send error
	SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Seek.Failed",L"error",L"Seek not supported"));
	return false;
}

bool MediaGateway::MediaBridgeInputStream::Publish(std::wstring& url)
{
	MediaBridgeSession *session;

	//Parse url
	std::wstring token(url);

	//Find query string
	int pos = url.find(L"?");

	//Remove query string
	if (pos>0)
		//Remove it
		token.erase(pos,token.length());

	//Check if we already had a media bridge session
	if (sesId)
		//Unplubish
		Close();

	//Get published session id
	sesId = mediaGateway->GetMediaBridgeIdFromInputToken(token);

	//Check if we have one
	if (!sesId)
		//Pin not found
		return Error("No media bridge session published for [pin:\"%ls\"]\n",token.c_str());

	//Get the media bridge session
	if (!mediaGateway->GetMediaBridgeRef(sesId,&session))
		//Broadcast not found
		return Error("Session not found\n");

	//Add us as the transmitter
	bool ret = session->SetTransmitter(this);

	//Release conference
	mediaGateway->ReleaseMediaBridgeRef(sesId);

	//Everything ok
	return ret;
}

bool MediaGateway::MediaBridgeInputStream::Close()
{
	//The broadcast sessioon
	MediaBridgeSession *session;

	//If no session
	if (!sesId)
		//Exit
		return false;

	//Get Broadcast
	if (!mediaGateway->GetMediaBridgeRef(sesId,&session))
		//Exit
		return false;

	//Remove us as a receiver
	session->UnsetTransmitter();


	//Release conference
	mediaGateway->ReleaseMediaBridgeRef(sesId);

	//Unlink us from session
	sesId = 0;

	return true;
}

/************************************
 * MediaBridgeOutputStream
 *
 ************************************/
MediaGateway::MediaBridgeOutputStream::MediaBridgeOutputStream(DWORD streamId, MediaGateway *MediaGateway) : RTMPStream(streamId)
{
	//Store the broadcaser
	this->mediaGateway = MediaGateway;
	//Not attached
	sesId = 0;
	//No packet yet
	first = -1;
}

MediaGateway::MediaBridgeOutputStream::~MediaBridgeOutputStream()
{
	//If we have not been closed
	if (!sesId)
		//Close
		Close();
}
bool MediaGateway::MediaBridgeOutputStream::Seek(DWORD time)
{
	//Send error
	SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Seek.Failed",L"error",L"Seek not supported"));
	return false;
}

bool MediaGateway::MediaBridgeOutputStream::Play(std::wstring& url)
{

	Log("-Playing broadcast session [url:\"%ls\"]\n",url.c_str());

	//The broadcast sessioon
	MediaBridgeSession *session;

	//Remove extra data from FMS
	if (url.find(L"*flv:")==0)
		//Erase it
		url.erase(0,5);
	else if (url.find(L"flv:")==0)
		//Erase it
		url.erase(0,4);

	//Get published session id
	sesId = mediaGateway->GetMediaBridgeIdFromOutputToken(url.c_str());

	//In the future parse queryString also to get token
	if(!sesId)
	{
		//Send error
		playListener->OnCommand(id,L"onStatus", new RTMPNetStatusEvent(L"NetStream.Play.StreamNotFound",L"error",L"Token not found"));
		//Broadcast not found
		return Error("No session id found for token [%ls]\n",url.c_str());
	}

	//Get Broadcast
	if (!mediaGateway->GetMediaBridgeRef(sesId,&session))
	{
		//No session
		sesId = 0;
		//Send error
		 playListener->OnCommand(id,L"onStatus", new RTMPNetStatusEvent(L"NetStream.Play.StreamNotFound",L"error",L"Stream not found"));
		//Broadcast not found
		return Error("Broadcast session not found\n");
	}

	//Start stream
	playListener->OnStreamBegin(id);

	//Add us as a receiver
	session->SetReceiver(this);

	//Release conference
	mediaGateway->ReleaseMediaBridgeRef(sesId);

	return true;
}

void MediaGateway::MediaBridgeOutputStream::PlayMediaFrame(RTMPMediaFrame* frame)
{
	//Get timestamp
	QWORD ts = frame->GetTimestamp();
	
	//Check if it is not negative
	if (ts!=-1)
	{
		//Check if it is first
		if (first==-1)
		{
			Log("-Got first frame [%llu]\n",ts);
			//Store timestamp
			first = ts;
			//Everything ok
			playListener->OnCommand(id,L"onStatus", new RTMPNetStatusEvent(L"NetStream.Play.Reset",L"status",L"Playback started") );
			//Send play comand
			SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Play.Start",L"status",L"Playback started") );
		}

		//Modify timestamp
		frame->SetTimestamp(ts-first);
	}

	//Send frame
	playListener->OnPlayedFrame(id,frame);
}


bool MediaGateway::MediaBridgeOutputStream::Publish(std::wstring& url)
{
	//Only output
	return false;
}

bool MediaGateway::MediaBridgeOutputStream::Close()
{
	//The broadcast sessioon
	MediaBridgeSession *session;

	//If no session
	if (!sesId)
		//Exit
		return false;

	//Get Broadcast
	if (!mediaGateway->GetMediaBridgeRef(sesId,&session))
		//Broadcast session not found
		return false;

	//Remove us as a receiver
	session->UnsetReceiver();

	//Release conference
	mediaGateway->ReleaseMediaBridgeRef(sesId);

	//Unlink us
	sesId = 0;
	
	//Everything ok
	return true;
}


