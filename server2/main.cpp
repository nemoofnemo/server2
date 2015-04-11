#include "server.h"

int main( void ){
	Log("[main]:program start\n");
	
	taskManager.registerQueue( "123456" , "qwerty" );
	taskManager.registerQueue( "123456" , "qwerty" );
	taskManager.registerQueue( "123456" , "qwerty" );
	taskManager.registerQueue( "123456" , "qwerty" );
	Sleep(50);

	Log("[main]:test register success\n");
	Log("[main]:processTask thread start\n");
	HANDLE h ;
	for( int i = 0 ; i < 10 ; ++ i ){
		h = (HANDLE)_beginthreadex( NULL , 0 , processTasks , (LPVOID)0 , 0 , NULL );
	}
	
	Sleep(500);
	Log("[main]:start server\n");
	HANDLE hserver;
	for( int i = 0 ; i < 4 ; ++ i){
		hserver= (HANDLE)_beginthreadex( NULL , 0 , listenThread ,(LPVOID)(i%4) ,0,NULL);
	}
	WaitForSingleObject( hserver , INFINITE );

	Log("[main]:program end\n");
	return 0;
}