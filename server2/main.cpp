#include "server.h"

int main( void ){
	Server srv;
	puts("[main]:program start");
	
	taskManager.registerQueue( "123456" , "qwerty" );
	puts("[main]:test register success");

	puts("[main]:processTask thread start");
	HANDLE h = (HANDLE)_beginthreadex( NULL , 0 , processTasks , NULL , 0 , NULL );
	//WaitForSingleObject( h , INFINITE );
	
	puts("[main]:start server");
	srv.startListening();

	puts("[main]:program end");
	return 0;
}