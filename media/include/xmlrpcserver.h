#ifndef _XMLRPCSERVER_H_
#define _XMLRPCSERVER_H_
#include <xmlrpc.h>
#include <xmlrpc-c/abyss.h>
#include <xmlrpc-c/server_abyss.h>
#include <string>
#include <map>
using namespace std;

class Handler
{
public:
	virtual int ProcessRequest(TRequestInfo *req,TSession * const ses)=0;
};

class XmlRpcServer
{
public:
	XmlRpcServer(int port);

	int AddHandler(string base,Handler* hnd);

	int Start();
	int Run();
	int Stop();

	static void RequestHandler(void *par,TSession *ses, abyss_bool *ret);
	static int GetBody(TSession *r,char *body,short bodyLen);
	static int SendResponse(TSession *r, short code, const char *msg, int length);
	static int SendError(TSession * r, short code);
	static int SendError(TSession * r, short code, const char *msg);

protected:
	int DispatchRequest(TSession *ses);

private:
	typedef map<string,Handler *> LstHandlers;
	int running;
	TServer srv;
	LstHandlers lstHandlers;

};

#endif
