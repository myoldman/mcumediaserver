/**
* @file live555MediaServer.cpp
* @brief Rtsp media server with live555 rtsp library
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#include <BasicUsageEnvironment.hh>
#include "DynamicRTSPServer.hh"
#include "live555MediaServer.h"
#include "log.h"

auto_ptr<Live555MediaServer> Live555MediaServer::_instance;


int Live555MediaServer::Init()
{
	 createPriorityThread(&m_ServerThread,run,this,0);
}


void * Live555MediaServer::run(void *par)
{
        Log("-----Rtsp Server Thread [%d]\n",getpid());

        Live555MediaServer *ses = (Live555MediaServer *)par;

        blocksignals();

        pthread_exit((void *)ses->Run());
}


int Live555MediaServer::Run()
{
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	m_Env = BasicUsageEnvironment::createNew(*scheduler);
	portNumBits rtspServerPortNum = 554;

	m_RtspServer = DynamicRTSPServer::createNew(*m_Env, rtspServerPortNum, NULL);
	if (m_RtspServer == NULL) {
		rtspServerPortNum = 8554;
		m_RtspServer = DynamicRTSPServer::createNew(*m_Env, rtspServerPortNum, NULL);
	}

	if (m_RtspServer == NULL) {
		Error("-----Failed to create RTSP server: %s\n", m_Env->getResultMsg());
		exit(1);
	}

	/*
	if (m_RtspServer->setUpTunnelingOverHTTP(80)) {
		Log("-----We use port : %d for optional RTSP-over-HTTP tunneling, or for HTTP live streaming (for indexed Transport Stream files only).\n", m_RtspServer->httpServerPortNum());
	} else {
		Error("-----RTSP-over-HTTP tunneling is not available.\n");
	}
	*/

	m_ServerPort = rtspServerPortNum;
	m_Env->taskScheduler().doEventLoop();

	Log("-----End of running RTSP Server\n");
	exit(0);
}

Live555MediaServer::Live555MediaServer() {
	m_RtspServer = NULL;
	m_Env = NULL;
	m_ServerPort = 0;
	m_Inited  = 0;
	m_ServerThread = 0;
}

Live555MediaServer::~Live555MediaServer() {
}
