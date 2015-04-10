#include "server.h"

int main( void ){
	Server srv;
	Log("[main]:program start\n");
	
	taskManager.registerQueue( "123456" , "qwerty" );
	Log("[main]:test register success\n");

	Log("[main]:processTask thread start\n");
	HANDLE h = (HANDLE)_beginthreadex( NULL , 0 , processTasks , NULL , 0 , NULL );
	//WaitForSingleObject( h , INFINITE );
	
	Log("[main]:start server\n");
	HANDLE hserver = (HANDLE)_beginthreadex( NULL , 0 , startListening ,NULL ,0,NULL);
	WaitForSingleObject( hserver , INFINITE );
	Log("[main]:program end\n");
	return 0;
}