#include "broadcastsession.h"
#include "tools.h"
#include "log.h"

BroadcastSession::BroadcastSession(const std::wstring &tag)
{
        //Save tag
 	this->tag = tag;
	//Not inited
	inited = true;
	transmitter = NULL;
	meta = NULL;
	sent = 0;
	maxTransfer = 0;
	maxConcurrent = 0;
	//Create mutex and condition
	pthread_mutex_init(&mutex,0);
}

BroadcastSession::~BroadcastSession()
{
	//If still running
	if (inited)
		//End
		End();
	//Destroy mutex
	pthread_mutex_destroy(&mutex);
}

bool BroadcastSession::Init(DWORD maxTransfer,DWORD maxConcurrent)
{
	//Set default limits
	this->maxTransfer = maxTransfer;
	this->maxConcurrent = maxConcurrent;

	//Reset sent bytes
	sent = 0;
	//We are started
	inited = true;

	return true;
}

bool BroadcastSession::End()
{
	//Check if we are running
	if (inited)
	{
		//Stop
		inited=0;
	
		//Unset transmitter
		UnsetTransmitter();
	}
		
	//Remove meta
	if (meta)
		//delete objet
		delete(meta);
	//No meta
	meta = NULL;

	return true;
}

bool BroadcastSession::SetTransmitter(RTMPStreamPublisher *transmitter)
{
	Log("-Set transmitter\n");

	//Lock
	pthread_mutex_lock(&mutex);

	//Check if we already have a transmitter for it
	if (this->transmitter || !inited || !transmitter)
	{
		//Unlock
		pthread_mutex_unlock(&mutex);
		//error
		return false;
	}

	//Create name
	char filename[1024];
	//Get time
	struct timeval now;
	gettimeofday(&now,0);
	//Set filenamee
	snprintf(filename,sizeof(filename),"/var/recordings/%.11ld-%ls.flv",(long)now.tv_sec,tag.c_str());

	//Set transmitter
	this->transmitter = transmitter;	

	//Log filename
	Log("-Recording broadcast [file:\"%s\"]\n",filename);

	//Open file for recording
	recorder.Create(filename);

	//And start recording
	recorder.Record();

	//Set callback
	transmitter->SetPublishListener(this);

	//Unlock
	pthread_mutex_unlock(&mutex);

	//No error
	return true;
}

void BroadcastSession::UnsetTransmitter()
{
	//Lock
	pthread_mutex_lock(&mutex);

	//Check transmitter
	if (transmitter)
		//Set data signal
		transmitter->SetPublishListener(NULL);

	//Set transmitter
	transmitter = NULL;	

	//Close recorder
	recorder.Close();

	//Unlock
	pthread_mutex_unlock(&mutex);
}

bool BroadcastSession::OnPublishedFrame(DWORD streamId,RTMPMediaFrame* frame)
{
	//Lock
	pthread_mutex_lock(&mutex);

	//Check
	if (maxTransfer && sent>maxTransfer)
	{
		//Unlock
		pthread_mutex_unlock(&mutex);
		//Exit
		return false;
	}
	
	//For all the receivers
	for (Receivers::iterator it=receivers.begin();it!=receivers.end();++it)
		//Send frame
		(*it)->PlayMediaFrame(frame);

	//Increase sent size
	sent += frame->GetSize()*receivers.size();

	//Record frame
	recorder.Write(frame);

	//unlock
	pthread_mutex_unlock(&mutex);

	return true;
}

bool BroadcastSession::AddReceiver(RTMPStream *receiver)
{
	//Lock
	pthread_mutex_lock(&mutex);

	//Check if we have reach the transmitters concurrent limit
	if (maxConcurrent && receivers.size()>maxConcurrent)
	{
		//unlock
		pthread_mutex_unlock(&mutex);
		//Error
		return Error("Concurrent receivers limit reached\n");
	}

	//Add to set
	receivers.insert(receiver);

	//If we already have metadata
	if (meta)
	{
		//Send it
		receiver->PlayMetaData(meta);

		//Add sent data
		sent += meta->GetSize();
	}

	//unlock
	pthread_mutex_unlock(&mutex);

	return true;
}

bool BroadcastSession::RemoveReceiver(RTMPStream *receiver)
{
	//Lock
	pthread_mutex_lock(&mutex);

	//Search
	Receivers::iterator it = receivers.find(receiver);

	//If found
	if (it!=receivers.end())
		//End
		receivers.erase(it);

	//Unlock
	pthread_mutex_unlock(&mutex);

	return true;
}

bool BroadcastSession::OnPublishedCommand(DWORD id,const wchar_t *name,AMFData* obj)
{
	//Lock
	pthread_mutex_lock(&mutex);

	//For all the receivers
	/*for (Receivers::iterator it=receivers.begin();it!=receivers.end();++it)
		//Send frame
		(*it)->PlayCommand(name,obj);
	*/
	//Unlock
	pthread_mutex_unlock(&mutex);

	return true;
}

bool BroadcastSession::OnPublishedMetaData(DWORD streamId,RTMPMetaData *publishedMetaData)
{
	//Lock
	pthread_mutex_lock(&mutex);

	//Check method
	AMFString* name = (AMFString*)publishedMetaData->GetParams(0);
	
	//Check it
	if (name->GetWString().compare(L"@setDataFrame")==0)
	{
		//If we already had one
		if (meta)
			//Delete if already have one
			delete(meta);

		//Create new msg
		meta = new RTMPMetaData(publishedMetaData->GetTimestamp());

		//Copy the rest of params
		for (DWORD i=1;i<publishedMetaData->GetParamsLength();i++)
			//Clone and append
			meta->AddParam(publishedMetaData->GetParams(i)->Clone());

		//For all the receivers
		for (Receivers::iterator it=receivers.begin();it!=receivers.end();++it)
			//Send frame
			(*it)->PlayMetaData(meta);

		//Record metadata
		recorder.Set(meta);

	}

	//Unlock
	pthread_mutex_unlock(&mutex);

	return true;
}

