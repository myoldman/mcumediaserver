#include <stdlib.h>
#include <map>
#include "log.h"
#include "mcu.h"
#include "rtmpparticipant.h"


/**************************************
* MCU
*	Constructor
**************************************/
MCU::MCU()
{
	//Inicializamos los mutex
	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&cond,NULL);
	//No event mngr
	eventMngr = NULL;
}

/**************************************
* ~MCU
*	Destructur
**************************************/
MCU::~MCU()
{
	//End just in case
	End();
	//Destruimos el mutex
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}


/**************************************
* Init
*	Inicializa la mcu
**************************************/
int MCU::Init(XmlStreamingHandler *eventMngr)
{
	timeval tv;
	
	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Estamos iniciados
	inited = true;

	//Get secs
	gettimeofday(&tv,NULL);
	
	//El id inicial
	maxId = (tv.tv_sec & 0x7FFF) << 16;

	//Store event mngr
	this->eventMngr = eventMngr;

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	//Salimos
	return 1;
}

/**************************************
* End
*	Termina la mcu
**************************************/
int MCU::End()
{
	Log(">End MCU\n");

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Dejamos de estar iniciados
	inited = false;

	//Paramos las conferencias
	for (Conferences::iterator it=conferences.begin(); it!=conferences.end(); ++it)
	{
		//Get the entry
		ConferenceEntry *entry = it->second;

		//Get the conference
		MultiConf *multi = entry->conf;

		//End it
		multi->End();

		//Delete conference
		delete multi;

		//Delete entry
		delete entry;
	}

	//LImpiamos las listas
	conferences.clear();

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	Log("<End MCU\n");

	//Salimos
	return false;
}

int MCU::CreateEventQueue()
{
	//Check mngr
	if (!eventMngr)
		//Error
		return Error("Event manager not set!\n");

	//Create it
	return eventMngr->CreateEventQueue();
}

int MCU::DeleteEventQueue(int id)
{
	//Check mngr
	if (!eventMngr)
		//Error
		return Error("Event manager not set!\n");

	//Create it
	return eventMngr->DestroyEventQueue(id);
}

/**************************************
* CreateConference
*	Inicia una conferencia
**************************************/
int MCU::CreateConference(std::wstring tag,int queueId)
{
	//Log
	Log(">CreateConference [tag:%ls,queueId:%d]\n",tag.c_str(),queueId);


	//Create the entry
	ConferenceEntry *entry = new ConferenceEntry();

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Get the id
	int confId = maxId++;

	//Creamos la multi
	MultiConf * conf = new MultiConf(tag, confId);



	//Guardamos los datos
	entry->id 	= confId;
	entry->conf 	= conf;
	entry->enabled 	= 1;
	entry->numRef	= 0;
	entry->queueId	= queueId;

	//a�adimos a la lista
	conferences[confId] = entry;

	//Desbloqueamos
	pthread_mutex_unlock(&mutex);

	//Set us as listeners
	conf->SetListener(this,(void*)entry);
		
	Log("<CreateConference [%d]\n",confId);

	return confId;
}

/**************************************
* GetConferenceRef
*	Obtiene una referencia a una conferencia
**************************************/
int MCU::GetConferenceRef(int id,MultiConf **conf)
{
	Log("-GetConferenceRef [%d]\n",id);

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Find confernce
	Conferences::iterator it = conferences.find(id);

	//SI no esta
	if (it==conferences.end())
	{
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);
		//Y salimos
		return Error("Conference not found [%d]\n",id);
	}

	//Get entry
	ConferenceEntry *entry = it->second;

	//Check it is enabled
	if (!entry->enabled)
	{
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);
		//Y salimos
		return Error("Conference not enabled [%d]\n",id);
	}

	//Aumentamos el contador
	entry->numRef++;

	//Y obtenemos el puntero a la la conferencia
	*conf = entry->conf;

	//Desbloquamos el mutex
	pthread_mutex_unlock(&mutex);

	//Signal condition
	pthread_cond_signal(&cond);

	return true;
}

/**************************************
* ReleaseConferenceRef
*	Libera una referencia a una conferencia
**************************************/
int MCU::ReleaseConferenceRef(int id)
{
	Log(">ReleaseConferenceRef [%d]\n",id);

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Find confernce
	Conferences::iterator it = conferences.find(id);

	//SI no esta
	if (it==conferences.end())
	{
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);
		//Y salimos
		return Error("Conference not found [%d]\n",id);
	}

	//Get entry
	ConferenceEntry *entry = it->second;

	//Aumentamos el contador
	entry->numRef--;

	//Desbloquamos el mutex
	pthread_mutex_unlock(&mutex);

	//Signal condition
	pthread_cond_signal(&cond);

	Log("<ReleaseConferenceRef\n");

	return true;
}

/**************************************
* DeleteConference
*	Inicializa la mcu
**************************************/
int MCU::DeleteConference(int id)
{
	Log(">DeleteConference [%d]\n",id);
	timespec ts;

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//Find conference
	Conferences::iterator it = conferences.find(id);

	//Check if we found it or not
	if (it==conferences.end())
	{
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);

		//Y salimos
		return Error("Conference not found [%d]\n",id);
	}

	//Get entry
	ConferenceEntry* entry = it->second;

	Log("-Disabling conference [%d]\n",entry->enabled);

	//Disable it
	entry->enabled = 0;

	//Whait to get free
	while(entry->numRef>0)
	{
		int ret = 0;
		//Calculate timeout
		calcTimout(&ts,5);

			//Wait with time out
		ret = pthread_cond_timedwait(&cond,&mutex,&ts);
			//Check if there is an errot different than timeout
		if (ret && ret!=ETIMEDOUT) {
			continue;
		} else if(ret) {
			continue;
		}
		/*	
		//Desbloquamos el mutex
		pthread_mutex_unlock(&mutex);
		//FIXME: poner una condicion
		sleep(5);
		//Bloqueamos
		pthread_mutex_lock(&mutex);
		*/
		//Find conference
		it = conferences.find(id);
		//Check if we found it or not
		if (it==conferences.end())
		{
			//Desbloquamos el mutex
			pthread_mutex_unlock(&mutex);
			//Y salimos
			return Error("Conference not found [%d]\n",id);
		}
		//Get entry again
		entry = it->second;
	}

	//Get conference from ref entry
	MultiConf *conf = entry->conf;

	//Remove from list
	conferences.erase(it);

	//Desbloquamos el mutex
	pthread_mutex_unlock(&mutex);

	//End conference
	conf->End();

	//Delete conference
	delete conf;

	//Delete the entry
	delete entry;

	Log("<DeleteConference [%d]\n",id);

	//Exit
	return true;
}

/********************************
 * Create
 **********************************/
RTMPStream* MCU::CreateStream(DWORD streamId,std::wstring &appName,DWORD audioCaps,DWORD videoCaps)
{
	RTMPStream* stream = NULL;
	MultiConf *conf = NULL;
	wchar_t *stopwcs;
	
	//Skip the mcu part and find the conf Id
	int i = 4;
	int j = appName.find(L"/",4);

	//Check if
	if (j<0)
	{
		//Noting found
		Error("Wrong format for app name\n");
		//Exit
		return NULL;
	}

	//Get conf id
	int confId = wcstol(appName.substr(i,j-i).c_str(),&stopwcs,10);

	//Get conference
	if(!GetConferenceRef(confId,&conf))
	{
		//No conference found
		Error("Confernce not found [confId:%d]\n",confId);
		//Exit
		return NULL;
	}

	//Get now the type
	std::wstring type = appName.substr(j+1);

	//Check app name
	if (type.compare(L"input")==0)
	{
		//Log
		Log("-Creating rtmp participant input stream\n");
		//A publishing stream
		stream = new RTMPParticipant::InputStream(streamId,conf);
	} else if (type.compare(L"output")==0) {
		//Log
		Log("-Creating rtmp participant output stream\n");
		//A receiver stream
		stream = new  RTMPParticipant::OutputStream(streamId,conf);
	} else if (type.compare(L"watcher")==0) {
		//Log
		Log("-Creating mcu watcher stream\n");
		//A receiver stream
		stream = new MultiConf::BroadcastStream(streamId,conf);
	} else {
		//Log
		Log("-Application type name incorrect [%ls]\n",type.c_str());
	}

	//release it
	ReleaseConferenceRef(confId);

	//No stream for that url
	return stream;
}


int MCU::GetConferenceList(ConferencesInfo& lst)
{
	Log(">GetConferenceList\n");

	//Bloqueamos
	pthread_mutex_lock(&mutex);

	//For each conference
	for (Conferences::iterator it = conferences.begin(); it!=conferences.end(); ++it)
	{
		//Get entry
		ConferenceEntry *entry = it->second;

		//Check it is enabled
		if (entry->enabled)
		{
			ConferenceInfo info;
			//Set data
			info.id = entry->id;
			info.name = entry->conf->GetTag();
			info.numPart = entry->conf->GetNumParticipants();
			//Append new info
			lst[entry->id] = info;
		}
	}
	
	//Desbloquamos el mutex
	pthread_mutex_unlock(&mutex);

	Log("<GetConferenceList\n");

	return true;
}

void MCU::onParticipantRequestFPU(MultiConf *conf,int partId,void *param)
{
	ConferenceEntry *entry = (ConferenceEntry *)param;
	//Check Event and event queue
	if (eventMngr && entry->queueId>0)
		//Send new event
		eventMngr->AddEvent(entry->queueId, new ::PlayerRequestFPUEvent(entry->id,conf->GetTag(),partId));
}
