#ifndef SERVER_H_
#define SERVER_H_
/********************************************************************************
**
**Function:server运行逻辑
**Args    :
**Return  :
**Detail  :实现server的功能
**
**Author  :袁羿
**Date    :2015/3/29
**
********************************************************************************/
#include "taskDataStructure.h"
#include "cmdFlag.h"
#define BUF_SIZE 2048000

/********************************************************************************
**
**Function:transferModule
**Args    :
**Return  :
**Detail  :socket数据传输模块。
**         构造函数初始化WSA库。
**         在数据传输之前，需要依次调用startBind，prepareListen。
**         静态方法checkLength为检测包长度。
**
**Author  :袁羿
**Date    :2015/3/29
**
********************************************************************************/
class transferModule{
private:
	WSADATA wsaData;
	SOCKET sockSrv;	//服务器SOCKET
	SOCKET sockConn;	//连接服务器的客户端的SOCKET

	SOCKADDR_IN addrSrv;
	SOCKADDR_IN  addrClient;

public:
	//默认构造函数。默认服务器监听6000端口
	transferModule(){
		if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 ) {
			exit(EXIT_FAILURE);
		}

		if ( LOBYTE( wsaData.wVersion ) != 2 ||	HIBYTE( wsaData.wVersion ) != 2 ){
			WSACleanup();
			exit(EXIT_FAILURE);
		}

		sockSrv = socket(AF_INET, SOCK_STREAM, 0);
		// 将INADDR_ANY转换为网络字节序，调用 htonl(long型)或htons(整型)
		addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY); 
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(6000);

	}

	transferModule(const int & port ){
		if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 ) {
			exit(EXIT_FAILURE);
		}

		if ( LOBYTE( wsaData.wVersion ) != 2 ||	HIBYTE( wsaData.wVersion ) != 2 ){
			WSACleanup( );
			exit(EXIT_FAILURE);
		}

		sockSrv = socket(AF_INET, SOCK_STREAM, 0);
		// 将INADDR_ANY转换为网络字节序，调用 htonl(long型)或htons(整型)
		addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY); 
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(port);

	}

	//析构函数。关闭socket
	~transferModule(){
		closesocket( sockSrv );
		closesocket( sockConn );
	}

	//检测包长度
	static bool checkLength( const char * data , const int length ){
		char temp[16] = {0};
		for( int i = 2 ; i < 10 ; ++ i ){
			temp[i-2] = data[i];
		}
		int val = atoi( temp );
		return val == length;
	}

	//waring:this function will release all socket connections
	void releaseAllSocket(){
		WSACleanup();
	}

	//服务器绑定。需要在prepareListen之前调用。
	int startBind( void ){
		return bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	}

	//使服务器准备接受连接
	int prepareListen(const int num = 15){
		return listen( sockSrv , num );
	}

	//获取server的socket
	SOCKET getServerSocket( void ){
		return this->sockSrv;
	}

	//获取客户端连接socket。如果建立成功返回socket。否则返回INVALID_SOCKET
	SOCKET acceptClient( void ){
		int len = sizeof(SOCKADDR);
		sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
		return sockConn ;
	}

	SOCKET getClientSocket( void ){
		return sockConn;
	}

	//获取客户端信息
	char * getClientIp( void ){
		return inet_ntoa(addrClient.sin_addr);
	}

	//关闭客户端socket
	int closeClient( void ){
		return closesocket( sockConn );
	}

	//send的第一个参数是客户端的socket
	//向客户端发送
	int sendData( char * data , int length){
		return send( sockConn , data , length , 0 );
	}

	//向客户端的监听线程发送数据//need test
	static int sendDataToListener( char * data , int length , string ip , int port){

		SOCKADDR_IN addr;
		addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());		
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
		connect(sock, (SOCKADDR*)&addr, sizeof(SOCKADDR));
		int status = send( sock , data , length , 0 );

		closesocket(sock);
		return status;
	}

	//从客户端接收发给服务器的数据
	//recv函数的第一个参数是发送数据的socket。
	int recvData( char * dest , int maxLength ){
		int count = 0;
		int num = 0;
		char * pData = dest;

		char * buffer = ( char *)malloc( sizeof(char) * BUF_SIZE );
		while( (num = recv( sockConn , buffer , BUF_SIZE , 0 ) ) > 0 ){
			count += num;
			if( count < maxLength ){
				memcpy( pData , buffer , num );
				pData += num;
			}
			else{
				return SRV_ERROR;
			}
			num = 0;
		}

		free( buffer );
		return count;
	}
};

//全局变量taksManager，负责管理任务
TaskManager taskManager;	//warning:Enter critical section before use TaskManager's methods!!!
//全局变量listener，负责监听
transferModule listener;	//listener

//数据统计
int log_connect;
int log_recv;
int log_use;
int log_throw;

const int srv_max_thread = 100;
int srv_cur_thread = 0;

//实现运行逻辑的线程
unsigned int __stdcall taskThread( LPVOID lpArg ){//should check length!!!!
	taskManager.EnterSpecifyCriticalSection( *((int*)lpArg) );
	int arg = *((int*)lpArg);

	TaskQueue * curQueue = taskManager.getSpecifyQueue( arg ) ;
	if( curQueue->getTaskNumber() == 0 ){
		taskManager.LeaveSpecifyCritialSection( arg );
		return 0;
	}

	Task curTask = *(curQueue->getCurTask());
	int length = curTask.length - 32;//!!
	char * data = curTask.data;

	char cmdFlag = *data;
	char formatFlag = *(data+1);
	char * dataSection = data;
	string taskMac( data + 10 , data + 16 );
	string targetMac;

	if( length > 0 && data != NULL){
		dataSection += 32;
	}

	puts("[server]:task thread running");
	//TODO
	switch(cmdFlag){//need to change status filed in Task
	case SENDTEXT:
		puts("[server]:msg");
		listener.sendDataToListener( "a" ,2,curQueue->getCurTargetIP() , 6002);
		curQueue->pop_front();
		break;
	case SRV_QUIT:
		curQueue->pop_front();
		puts("[server]:quit.");
		exit(0);
		break;
	default:
		puts("[server]:ok");
		curQueue->pop_front();
		break;
	}

	++ log_use;
	//taskManager.LeaveSpecifyCritialSection( arg );
	printf("[server]:connection=%d,recv=%d,throw=%d,use=%d\n" , log_connect , log_recv , log_throw , log_use);
	curQueue->printQueueInfo();
	taskManager.LeaveSpecifyCritialSection( arg );
	return 0;
}

//每隔一段时间检查taskManager中有没有需要执行的任务 
unsigned int __stdcall processTasks( LPVOID lpArg ){
	int taskQueueNum =0 ;
	const int handleArrLen = taskManager.getMaxQueueNum();
	int * arr = ( int  * )malloc( sizeof(int) * handleArrLen );
	HANDLE * handleArr = (HANDLE *)malloc( sizeof(HANDLE) * handleArrLen);

	for( int i = 0 ; i < handleArrLen ; ++ i ){
		arr[i] = i;
	}

	while( true ){
		//EnterCriticalSection( &taskManagerCriticalSection );
		taskQueueNum = taskManager.getTaskQueueNum();	
		for( int i = 0 ; i < taskQueueNum ; ++ i ){
			taskManager.EnterSpecifyCriticalSection( i );
			TaskQueue * ptr = taskManager.getSpecifyQueue(i);
			if( ptr && ptr->getTaskNumber() > 0){
				handleArr[i] = (HANDLE)_beginthreadex( NULL , 0 , taskThread , arr + i  , 0 ,NULL);		
			}
			taskManager.LeaveSpecifyCritialSection( i );
			//CloseHandle(handleArr[i]);//need test
		}
		
		//WaitForMultipleObjects( taskQueueNum , handleArr , true , INFINITE );
		//LeaveCriticalSection( &taskManagerCriticalSection );
		//wait 1 ms
		Sleep( 1 );
	}
	return 0;
}

//===============================================

//warning.this struct is the argument of thread recvData
CRITICAL_SECTION recvThreadCS;
struct recvDataArg{
	SOCKET sock;	//socket that need to call function recv
	char ip[16];	//ip of socket
};

//接收数据的线程
unsigned int __stdcall recvData( LPVOID lpArg ){
	EnterCriticalSection( & taskManagerCriticalSection );
	SOCKET sock = ((recvDataArg*)lpArg)->sock;
	string ip(((recvDataArg*)lpArg)->ip);
	LeaveCriticalSection( &recvThreadCS );
	
	int count = 0;
	int num = 0;
	int limit = 5 * BUF_SIZE;

	char * data =  ( char *)malloc( sizeof(char) * limit);
	char * pData = data;
	char * buffer = ( char *)malloc( sizeof(char) * BUF_SIZE );

	while( (num = recv( sock , buffer , BUF_SIZE , 0 ) ) > 0 ){
		count += num;
		if( count < limit ){
			memcpy( pData , buffer , num );
			pData += num;
		}
		else{
			-- srv_cur_thread ;
			LeaveCriticalSection( & taskManagerCriticalSection );
			return SRV_ERROR;
		}
		num = 0;
	}

	closesocket(sock);

	//TODO : need find by mac.check length.ip
	//judge
	++ log_recv;
	printf("[server]:recv data = %d\n" , count);
	/*for( int d = 0 ;d < count;d ++){
		putchar( data[d]);
	}*/
	
	taskManager.EnterSpecifyCriticalSection( 0 ) ;
	TaskQueue * curQueue = taskManager.getSpecifyQueue(0);
	if( !curQueue->push_back(data, count , ip , ip)){
		puts("[server]:specify task queue full");
		++ log_throw ;
		//通知客户端重新发送
	}else{
		
	}
	taskManager.LeaveSpecifyCritialSection( 0 );
	LeaveCriticalSection( & taskManagerCriticalSection );
	//release
	free( data );
	free( buffer );
	-- srv_cur_thread ;
	return 0;
}

//============================================

class Server{
public:
	Server(){
		
	}

	~Server(){
		WSACleanup();
	}

	bool registerMac( string ward , string target ){
		return taskManager.registerQueue(ward,target );
	}

	void startListening( void ){
		recvDataArg arg;
		listener.startBind();
		listener.prepareListen(25);

		puts("[server]:server crital section created.");
		if(InitializeCriticalSectionAndSpinCount(&recvThreadCS,4000) != TRUE){
			exit(EXIT_FAILURE);
		}
		
		puts("[server]:start.");
		//监听的主循环
		while( true ){
			puts("[server]:listening.");

			//保证参数的互斥访问.在thread recvData中leave
			EnterCriticalSection( &recvThreadCS );
			if((arg.sock = listener.acceptClient()) == INVALID_SOCKET ){
				puts("[server]:accept falied.");
				LeaveCriticalSection( &recvThreadCS );
				continue;
			}
			++ log_connect;
			puts("[server]:connection accepted.");
			memcpy( arg.ip , listener.getClientIp() , 16 );
			
			//启动新线程接收数据
			if( arg.sock != INVALID_SOCKET ){
				if( srv_cur_thread < srv_max_thread ){
					HANDLE h = (HANDLE)_beginthreadex( NULL , 0 , recvData , &arg , 0 , NULL );
					++ srv_cur_thread;
					puts("[server]:A new thread has been created.");
				}
				else{
					puts("[server]:thread pool full.throw");
					closesocket(arg.sock);
					++ log_throw;
					LeaveCriticalSection( &recvThreadCS );
				}
				//CloseHandle( h );//need test
			}
		}

		DeleteCriticalSection(&recvThreadCS );
		puts("[server]:stop.");
	}

};

#endif // !SERVER_H_
