#include "log.h"
#include "xmlhandler.h"
#include "broadcaster.h"

xmlrpc_value* BroadcasterCreateBroadcast(xmlrpc_env *env, xmlrpc_value *param_array, void *user_data)
{
	UTF8Parser nameParser;
        UTF8Parser tagParser;
	Broadcaster *broadcaster = (Broadcaster *)user_data;
	BroadcastSession *session = NULL;

	 //Parseamos
	char *str;
        char *tag;
	int maxTransfer;
	int maxConcurrent;
	xmlrpc_parse_value(env, param_array, "(ssii)", &str,&tag,&maxTransfer,&maxConcurrent);

	//Parse string
	nameParser.Parse((BYTE*)str,strlen(str));
        tagParser.Parse((BYTE*)tag,strlen(tag));
	 
	//Creamos la sessionerencia
	int sessionId = broadcaster->CreateBroadcast(nameParser.GetWString(),tagParser.GetWString());
	
	//Obtenemos la referencia
	if(!broadcaster->GetBroadcastRef(sessionId,&session))
		return xmlerror(env,"Conferencia borrada antes de poder iniciarse\n");

	//Si error
	if (!sessionId>0)
		return xmlerror(env,"Could not create session");

	//La iniciamos
	bool res = session->Init(maxTransfer,maxConcurrent);

	//Liberamos la referencia
	broadcaster->ReleaseBroadcastRef(sessionId);

	//Salimos
	if(!res)
		return xmlerror(env,"Could not start conference\n");

	//Devolvemos el resultado
	return xmlok(env,xmlrpc_build_value(env,"(i)",sessionId));
}

xmlrpc_value* BroadcasterDeleteBroadcast(xmlrpc_env *env, xmlrpc_value *param_array, void *user_data)
{
	Broadcaster *broadcaster = (Broadcaster *)user_data;

	 //Parseamos
	int sessionId;
	xmlrpc_parse_value(env, param_array, "(i)", &sessionId);

	//Comprobamos si ha habido error
	if(env->fault_occurred)
		return 0;

	//Delete sessionerence 
	if (!broadcaster->DeleteBroadcast(sessionId))
		return xmlerror(env,"Session does not exist");

	//Devolvemos el resultado
	return xmlok(env);
}

xmlrpc_value* BroadcasterPublishBroadcast(xmlrpc_env *env, xmlrpc_value *param_array, void *user_data)
{
	UTF8Parser parser;
	Broadcaster *broadcaster = (Broadcaster *)user_data;

	//Parseamos
	int sessionId;
	char *str;
	xmlrpc_parse_value(env, param_array, "(is)", &sessionId, &str);

	//Comprobamos si ha habido error
	if(env->fault_occurred)
		return 0;

	//Parse string
	parser.Parse((BYTE*)str,strlen(str));

	//La borramos
	bool res = broadcaster->PublishBroadcast(sessionId,parser.GetWString());

	//Salimos
	if(!res)
		return xmlerror(env,"Could not add pin to broadcast session\n");

	//Devolvemos el resultado
	return xmlok(env);
}

xmlrpc_value* BroadcasterUnPublishBroadcast(xmlrpc_env *env, xmlrpc_value *param_array, void *user_data)
{
	Broadcaster *broadcaster = (Broadcaster *)user_data;

	 //Parseamos
	int sessionId;
	xmlrpc_parse_value(env, param_array, "(i)", &sessionId);

	//Comprobamos si ha habido error
	if(env->fault_occurred)
		return 0;

	//Delete sessionerence 
	if (!broadcaster->UnPublishBroadcast(sessionId))
		return xmlerror(env,"Session does not exist");

	//Devolvemos el resultado
	return xmlok(env);
}

xmlrpc_value* BroadcasterAddBroadcastToken(xmlrpc_env *env, xmlrpc_value *param_array, void *user_data)
{
	UTF8Parser parser;
	Broadcaster *broadcaster = (Broadcaster *)user_data;

	 //Parseamos
	int sessionId;
	char *str;
	xmlrpc_parse_value(env, param_array, "(is)", &sessionId, &str);

	//Comprobamos si ha habido error
	if(env->fault_occurred)
		return 0;

	//Parse string
	parser.Parse((BYTE*)str,strlen(str));
	 
	//Add token
	bool res = broadcaster->AddBroadcastToken(sessionId,parser.GetWString());

	//Salimos
	if(!res)
		return xmlerror(env,"Could not add pin to broadcast session\n");

	//Devolvemos el resultado
	return xmlok(env);
}


XmlHandlerCmd broadcasterCmdList[] =
{
	{"CreateBroadcast",BroadcasterCreateBroadcast},
	{"DeleteBroadcast",BroadcasterDeleteBroadcast},
	{"PublishBroadcast",BroadcasterPublishBroadcast},
	{"UnPublishBroadcast",BroadcasterUnPublishBroadcast},
	{"AddBroadcastToken",BroadcasterAddBroadcastToken},
	{NULL,NULL}
};
