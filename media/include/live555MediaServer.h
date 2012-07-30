/**
* @file live555MediaServer.h
* @brief Rtsp media server with live555 rtsp library
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/
#ifndef _LIVE555MEDIASERVER_H_
#define _LIVE555MEDIASERVER_H_
#include <memory>
#include <iostream>
#include "RTSPServerSupportingHTTPStreaming.hh"
using namespace std;


class Live555MediaServer
{
public:
	/**
	* @brief Get singleton instance of live555MediaServer
	* @return singleton instance of live555MediaServer
	*/
	static Live555MediaServer * Instance()
	{
		if( NULL == _instance.get())
		{
			_instance.reset( new Live555MediaServer);
		}
		return _instance.get();
	}

	/**
	* @brief Start live555MediaServer process thread
	* @return init result
	*/
	int Init();
	
	
	/**
	* @brief Get instance of rtsp server from live555 library
	* @return m_RtspServer
	*/
	RTSPServer* GetRTSPServer() { return m_RtspServer;}

	/**
	* @brief Get instance of UsageEnvironment from live555 library
	* @return m_Env
	*/
	UsageEnvironment* GetUsageEnvironment() { return m_Env;}

protected:
	/** Constructor */
	Live555MediaServer();
	/** Destructor */
	~Live555MediaServer();
	
	/** auto point of singleton instance for auto memory release */
	friend class auto_ptr<Live555MediaServer>;
	static auto_ptr<Live555MediaServer> _instance;
	
	/**
	* @brief live555MediaServer process thread running method
	* @return running result
	*/
	int Run();

private:
	/**
	* @brief static method for starting process thread
	* @return none
	*/
	static void * run(void *par);
private:
	// Inited status of rtsp server
	int m_Inited;
	// local port of rtsp server
	int m_ServerPort;
	// thread id for processing thread
	pthread_t m_ServerThread;
	
	// instance of rtsp server from live555 library
	RTSPServer* m_RtspServer;
	// instance of usage environment from live55 library
	UsageEnvironment* m_Env;
	
};

#endif
