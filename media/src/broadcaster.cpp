#include "log.h"
#include "broadcaster.h"
#include "rtmpflvstream.h"
#include "rtmpmp4stream.h"


/**************************************
* Broadcaster
*	Constructor
**************************************/
Broadcaster::Broadcaster() 
{
	//Inicializamos los mutex
	pthread_mutex_init(&mutex,NULL);
}

/**************************************
* ~Broadcaster
*	Destructur
**************************************/
Broadcaster::~Broadcaster()
{
	//Destruimos el mutex
	pthread_mutex_destroy(&mutex);
}


/**************************************
* Init
*	Inicializa el servidor de FLV
**************************************/
bool Broadcaster::Init()
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
*	Termina el servidor de FLV
**************************************/
bool Broadcaster::End()
{
	Log(">End Broadcaster\n");

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Dejamos de estar iniciados
	inited = false;

	//Paramos las sesions
	for (BroadcastEntries::iterator it=broadcasts.begin(); it!=broadcasts.end(); it++)
	{
		//Obtenemos la sesion
		BroadcastSession *session = it->second.session;

		//La paramos
		session->End();

		//Y la borramos
		delete session;
	}

	//Clear the broadcaster list
	broadcasts.clear();

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	Log("<End Broadcaster\n");

	//Salimos
	return !inited;
}

/**************************************
* CreateBroadcast
*	Inicia una sesion
**************************************/
DWORD Broadcaster::CreateBroadcast(const std::wstring &name,const std::wstring &tag)
{
	Log("-CreateBroadcast [name:\"%ls\",tag:\"%ls\"]\n",name.c_str(),tag.c_str());


	//Creamos la session
	BroadcastSession * session = new BroadcastSession(tag);

	//Obtenemos el id
	DWORD sessionId = maxId++;

	//Creamos la entrada
	BroadcastEntry entry; 

	//Guardamos los datos
	entry.id 	= sessionId;
	entry.name 	= name;
        entry.tag       = tag;
	entry.session 	= session;
	entry.enabled 	= 1;
	entry.numRef	= 0;
	
	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//a�adimos a la lista
	broadcasts[sessionId] = entry;

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	return sessionId;
}

/**************************************
 * PublishBroadcast
 *  
 **************************************/
bool Broadcaster::PublishBroadcast(DWORD id,const std::wstring &pin)
{
	Log(">PublishBroadcast [id:%d,pin:\"%ls\"]\n",id,pin.c_str());

	bool res = false;

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Get the broadcaster session entry
	BroadcastEntries::iterator it = broadcasts.find(id);

	//Check it
	if (it==broadcasts.end())
	{
		//Broadcast not found
		Error("Broadcast session not found\n");
		//Exit
		goto end;
	}

	//Check if it was already been published
	if (!it->second.pin.empty())
	{
		//Broadcast not found
		Error("Broadcast session already published\n");
		//Exit
		goto end;
	}

	//Check if the pin is already in use
	if (published.find(pin)!=published.end())
	{
		//Broadcast not found
		Error("Pin already in use by other broadcast session\n");
		//Exit
		goto end;
	}

	//Add to the pin list
	published[pin] = id;

	//Set the entry broadcast session publication pin
	it->second.pin = pin;

	//Everything was ok
	res = true;

end:
	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	Log("<PublishBroadcast\n");

	return res;
}

/********************************************************
 * AddBroadcastToken
 *   Add a token for playing
 ********************************************************/
bool Broadcaster::AddBroadcastToken(DWORD id, const std::wstring &token)
{
	Log("-AddBroadcastToken [id:%d,token:\"%ls\"]\n",id,token.c_str());

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Check if the pin is already in use
	if (tokens.find(token)!=tokens.end())
	{
		//Desbloqueamos
		pthread_mutex_unlock(&mutex);
		//Broadcast not found
		return Error("Token already in use\n");
	}

	//Add to the pin list
	tokens[token] = id;

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	return true;
}

/**************************************
 * UnPublishBroadcast
 *  
 **************************************/
bool Broadcaster::UnPublishBroadcast(DWORD id)
{
	Log(">UnPublishBroadcast [id:%d]\n",id);

	bool res = false;

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Get the broadcaster session entry
	BroadcastEntries::iterator it = broadcasts.find(id);

	//Check it
	if (it==broadcasts.end())
	{
		//Broadcast not found
		Error("Broadcast session not found\n");
		//Exit
		goto end;
	}

	//Check if it was already been published
	if (it->second.pin.empty())
	{
		//Broadcast not found
		Error("Broadcast session was not published\n");
		//Exit
		goto end;
	}

	//Remove pin
	published.erase(it->second.pin);

	//Unset transmitter
	it->second.session->UnsetTransmitter();

	//Set the entry broadcast session publication pin empty
	it->second.pin = L"";

	//Everything was ok
	res = true;

end:
	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	Log("<PublishBroadcast\n");

	return res;
}

/**************************************
 * GetPublishedBroadcastId
 *   Get the id of the session published with the pin
 * ************************************/
DWORD Broadcaster::GetPublishedBroadcastId(const std::wstring &pin)
{
	DWORD sessionId = 0;

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Check if there is already a pin like that
	PublishedBroadcasts::iterator it = published.find(pin);

	//Check we found one
	if (it!=published.end())
		//Get id
		sessionId = it->second;

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	//Return session id
	return sessionId;
}

/**************************************
 * GetTokenBroadcastId
 *   Get the id of the session published with the token and remove it
 *************************************/
DWORD Broadcaster::GetTokenBroadcastId(const std::wstring &token)
{
	Log(">GetTokenBroadcastId [token:\"%ls\"]\n",token.c_str());

	DWORD sessionId = 0;

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Check if there is already a pin like that
	BroadcastTokens::iterator it = tokens.find(token);

	//Check we found one
	if (it!=tokens.end())
	{
		//Get id
		sessionId = it->second;
		//Remove token
		tokens.erase(it);
	}

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	Log("<GetTokenBroadcastId [sessionId:\"%d\"]\n",sessionId);

	//Return session id
	return sessionId;
}


/**************************************
* GetBroadcastRef
*	Obtiene una referencia a una sesion
**************************************/
bool Broadcaster::GetBroadcastRef(DWORD id,BroadcastSession **session)
{
	Log(">GetBroadcastRef [%d]\n",id);

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Find session reference
	BroadcastEntries::iterator it = broadcasts.find(id);

	//SI no esta
	if (it==broadcasts.end())
	{
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);
		//Y salimos
		return Error("Session no encontrada [%d]\n",id);
	}

	//Get entry
	BroadcastEntry &entry = it->second;

	//Aumentamos el contador
	entry.numRef++;

	//Y obtenemos el puntero a la la sesion
	*session = entry.session;

	//Desbloquamos el mutex
	pthread_mutex_unlock(&mutex);

	Log("<GetBroadcastRef \n");

	return true;
}

/**************************************
* ReleaseBroadcastRef
*	Libera una referencia a una sesion
**************************************/
bool Broadcaster::ReleaseBroadcastRef(DWORD id)
{
	Log(">ReleaseBroadcastRef [%d]\n",id);

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Find session reference
	BroadcastEntries::iterator it = broadcasts.find(id);

	//SI no esta
	if (it==broadcasts.end())
	{
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);
		//Y salimos
		return Error("Session no encontrada [%d]\n",id);
	}

	//Get entry
	BroadcastEntry &entry = it->second;

	//Aumentamos el contador
	entry.numRef--;

	//Desbloquamos el mutex
	pthread_mutex_unlock(&mutex);

	Log("<ReleaseBroadcastRef\n");

	return true;
}

/**************************************
* DeleteSession
*	Inicializa el servidor de FLV
**************************************/
bool Broadcaster::DeleteBroadcast(DWORD id)
{
	Log(">DeleteSession [%d]\n",id);

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Find sessionerence
	BroadcastEntries::iterator it = broadcasts.find(id);

	//Check if we found it or not
	if (it==broadcasts.end())
	{
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);

		//Y salimos
		return Error("Session no encontrada [%d]\n",id);
	}

	//Get entry
	BroadcastEntry &entry = it->second;

	//Whait to get free
	while(entry.numRef>0)
	{
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);
		//FIXME: poner una condicion
		sleep(20);
		//Bloqueamos
		pthread_mutex_lock(&mutex);
		//Find broadcast
		it =  broadcasts.find(id);
		//Check if we found it or not
		if (it==broadcasts.end())
		{
			//Desbloquamos el mutex
			pthread_mutex_unlock(&mutex);
			//Y salimos
			return Error("Broadcast no encontrada [%d]\n",id);
		}
		//Get entry again
		entry = it->second;
	}

	//Get sessionerence
	BroadcastSession *session = entry.session;

	//If it was published
	if (!entry.pin.empty())
		//Remove from published list
		published.erase(entry.pin);

	//Remove from list
	broadcasts.erase(it);

	//Desbloquamos el mutex
	pthread_mutex_unlock(&mutex);

	//Unset transmitter
	session->UnsetTransmitter();

	//End
	session->End();

	//Delete sessionerence
	delete session;

	Log("<DeleteSession [%d]\n",id);

	//Exit
	return true;
}

RTMPStream* Broadcaster::CreateStream(DWORD streamId,std::wstring &appName,DWORD audioCaps,DWORD videoCaps)
{
	//Check app name
	if (appName.compare(L"broadcaster/publish")==0)
	{
		Log("-Creating transmitter stream\n");
		//A publishing stream
		return new BroadcastInputStream(streamId,this);
	} else if (appName.compare(L"broadcaster")==0) {
		Log("-Creating receiver stream\n");
		//A receiver stream
		return new BroadcastOutputStream(streamId,this);
	} else if (appName.compare(L"streamer/flv")==0) {
		Log("-Creating FLV streamer stream\n");
		//A receiver stream
		return new RTMPFLVStream(streamId);
	} else if (appName.compare(L"streamer/mp4")==0) {
		Log("-Creating MP4 streamer stream\n");
		//A receiver stream
		return new RTMPMP4Stream(streamId);
	} else {
		Log("-Application name incorrect [%ls]\n",appName.c_str());
		//No stream for that url
		return NULL;
	}
	
}

/************************************
 * BroadcastInputStream
 *
 ************************************/
Broadcaster::BroadcastInputStream::BroadcastInputStream(DWORD streamId,Broadcaster *broadcaster) : RTMPStream(streamId)
{
	//Store the broarcaster
	this->broadcaster = broadcaster;
	//Not publishing
	sesId = 0;
}

Broadcaster::BroadcastInputStream::~BroadcastInputStream()
{
	//If we are still publishing
	if (sesId)
		//Close it
		Close();
}

bool Broadcaster::BroadcastInputStream::Play(std::wstring& url)
{
	//Not supported
	return false;
}

bool Broadcaster::BroadcastInputStream::Seek(DWORD time)
{
	//Send error
	SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Seek.Failed",L"error",L"Seek not supported"));
	return false;
}

bool Broadcaster::BroadcastInputStream::Publish(std::wstring& url)
{
	BroadcastSession *session;

	//Parse url
	std::wstring pin(url);

	//Find query string
	int pos = url.find(L"?");

	//Remove query string
	if (pos>0)
		//Remove it
		pin.erase(pos,pin.length());

	Log("Publish sream [pin:\"%ls\"]\n",pin.c_str());

	//Check if we already had a broadcast session
	if (sesId)
		//Unplubish
		Close();

	//Get published session id
	sesId = broadcaster->GetPublishedBroadcastId(pin);

	//Check if we have one 
	if (!sesId)
		//Pin not found
		return Error("No broadcast session published for [pin:\"%ls\"]\n",pin.c_str());	

	//Get the broadcaster
	if (!broadcaster->GetBroadcastRef(sesId,&session))
		//Broadcast not found
		return Error("Session not found\n");

	//Add us as the transmitter
	bool ret = session->SetTransmitter(this);

	//Release conference
	broadcaster->ReleaseBroadcastRef(sesId);
	
	//Everything ok
	return ret;
}

bool Broadcaster::BroadcastInputStream::Close()
{
	//The broadcast sessioon
	BroadcastSession *session;

	//Get Broadcast
	if (!broadcaster->GetBroadcastRef(sesId,&session))
		//Broadcast session not found
		return false;

	//Remove us as a receiver
	session->UnsetTransmitter();

	//Unlink us
	sesId = 0;

	//Release conference
	broadcaster->ReleaseBroadcastRef(sesId);

	//Everything ok
	return true;
}

/************************************
 * BroadcastOutputStream
 *
 ************************************/
Broadcaster::BroadcastOutputStream::BroadcastOutputStream(DWORD streamId, Broadcaster *broadcaster) : RTMPStream(streamId)
{
	//Store the broadcaser
	this->broadcaster = broadcaster;
	//Not attached
	sesId = 0;
	//No packet yet
	first = (QWORD)-1;
}

Broadcaster::BroadcastOutputStream::~BroadcastOutputStream()
{
	//If we have not been closed
	if (!sesId)
		//Close
		Close();
}
bool Broadcaster::BroadcastOutputStream::Seek(DWORD time)
{
	//Send error
	SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Seek.Failed",L"error",L"Seek not supported"));
	return false;
}

bool Broadcaster::BroadcastOutputStream::Play(std::wstring& url)
{

	Log("-Playing broadcast session [url:\"%ls\"]\n",url.c_str());

	//The broadcast sessioon
	BroadcastSession *session;

	//Get published session id
	sesId = broadcaster->GetTokenBroadcastId(url.c_str());

	//In the future parse queryString also to get token
	if(!sesId)
	{
		//Send error
		SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Play.StreamNotFound",L"error",L"Token not found"));
		//Broadcast not found
		return Error("No session id found for token [%ls]\n",url.c_str());
	}

	//Get Broadcast
	if (!broadcaster->GetBroadcastRef(sesId,&session))
	{
		//No session
		sesId = 0;
		//Send error
		 SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Play.StreamNotFound",L"error",L"Stream not found"));
		//Broadcast not found
		return Error("Broadcast session not found\n");
	}

	//Start stream
	SendStreamBegin();

	//Add us as a receiver
	session->AddReceiver(this);

	//Release conference
	broadcaster->ReleaseBroadcastRef(sesId);

	return true;
}

void Broadcaster::BroadcastOutputStream::PlayMediaFrame(RTMPMediaFrame* frame)
{
	//Get timestamp
	QWORD ts = frame->GetTimestamp();
	//Check if it is first
	if (first==(QWORD)-1)
	{
		Log("-Got first frame [%llu]\n",ts);
		//Store timestamp
		first = ts;
		//Everything ok
		SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Play.Reset",L"status",L"Playback started") );
	}

	//Modify timestamp
	frame->SetTimestamp(ts-first);

	//Send frame
	RTMPStream::PlayMediaFrame(frame);

	//Restore timestamp
	frame->SetTimestamp(ts);
}


bool Broadcaster::BroadcastOutputStream::Publish(std::wstring& url)
{
	//Only output
	return false;
}

bool Broadcaster::BroadcastOutputStream::Close()
{
	//The broadcast sessioon
	BroadcastSession *session;

	//Get Broadcast
	if (!broadcaster->GetBroadcastRef(sesId,&session))
		//Broadcast session not found
		return false;

	//Remove us as a receiver
	session->RemoveReceiver(this);

	//Unlink us
	sesId = 0;

	//Release conference
	broadcaster->ReleaseBroadcastRef(sesId);

	//Everything ok
	return true;
}


