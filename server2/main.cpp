#include "server.h"

int main( void ){
	Log("[main]:program start\n");
	Log("[main]:test register success\n");
	InitializeCriticalSectionAndSpinCount( &mics , 4096 );
	mac_ip.push_back(v_type( (pair<string,string>("0123456789AB" , "NULL" )) , (pair<string,string>("AB0123456789","NULL"))) );
	
	Sleep(500);
	Log("[main]:start server\n");
	HANDLE hserver;
	for( int i = 0 ; i < 1 ; ++ i){
		hserver= (HANDLE)_beginthreadex( NULL , 0 , listenThread ,NULL ,0,NULL);
	}
	WaitForSingleObject( hserver , INFINITE );

	Log("[main]:program end\n");
	return 0;
}