#ifndef SERVER_H_
#define SERVER_H_

#include "taskDataStructure.h"
#include "cmdFlag.h"
#define BUF_SIZE 2048000
#define LOOP_DELAY 1

class transferModule{
private:
	WSADATA wsaData;
	SOCKET sockSrv;	//服务器SOCKET
	SOCKET sockConn;	//连接服务器的客户端的SOCKET

	SOCKADDR_IN addrSrv;
	SOCKADDR_IN  addrClient;
public:
	//默认构造函数。默认服务器监听6000端口
	transferModule(const int & port = 6000 ){
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

	static vector<char> pack( char cmd , char fmt , string mac , char * dataSection , int dataSectionLength){
		int length = ( dataSectionLength > 0 )?(32 + dataSectionLength):32;
		char * temp = (char * )malloc( sizeof(char) * length );
		memset( temp , '0' , sizeof(char) * length );
		temp[0] = cmd;
		temp[1] = fmt;
		sprintf(temp + 2 ,"%08d", length );
		sprintf(temp + 10,"%012s" , mac.c_str() );
		temp[22] = '0';
		memcpy(temp + 32 , dataSection , dataSectionLength );
		vector<char> ret( temp , temp + length );
		free(temp);
		return ret;
	}
};

/**********************************************************************************/
//全局变量taksManager，负责管理任务
TaskManager taskManager;	//warning:one program,one Taskmanager Obeject!!!

//实现运行逻辑的线程
unsigned int __stdcall taskThread( LPVOID lpArg ){
	taskManager.EnterSpecifyCriticalSection( (int)lpArg );
	int arg = (int)lpArg;
	TaskQueue * curQueue = taskManager.getSpecifyQueue( arg ) ;

	if(curQueue == NULL ){
		taskManager.LeaveSpecifyCritialSection( arg );
		return 0;
	}

	Task * pTask = curQueue->getCurTask();
	if( pTask == NULL ){
		taskManager.LeaveSpecifyCritialSection( arg );
		return 0;
	}

	int length = pTask->length ;//!!
	if( length == 0 || pTask->data == NULL ){
		curQueue->pop_front();
		taskManager.LeaveSpecifyCritialSection( arg );
		return 0;
	}


	char * data = (char *)malloc(sizeof(char)*pTask->length);
	memcpy( data , pTask->data , length );
	curQueue->pop_front();
	taskManager.LeaveSpecifyCritialSection( arg );

	//.........................
	char cmdFlag = *data;
	char formatFlag = *(data+1);
	char * dataSection = data;
	int dataSectionLen = 0;
	string taskMac( data + 10 , data + 16 );
	string targetMac;

	if( length > 32 && data != NULL){
		dataSection += 32;
		dataSectionLen = length - 32;
	}

	Log("[server]:task thread.\n");
	switch(cmdFlag){
	case SRV_QUIT://debug
		Log("[server]:quit.\n");
		exit(0);
		break;
	case DUMP:
		Log("[server]:dump.\n");
		/*for( int i = 0 ; i < dataSectionLen ; ++ i ){
			putchar(*(dataSection + i ));
		}
		putchar('\n');*/
		++ log_dump;
		break;
	default:
		Log("[server]:Invalid CMD flag = %x\n",cmdFlag);
		log_invalid ++;
		//return error message
		
		break;
	}

	
	++ log_use;
	Log("[server]:connection=%d,recv=%d,throw=%d,use=%d,dump=%d,invalid=%d\n" , log_connect , log_recv , log_throw , log_use , log_dump,log_invalid);
	
	return 0;
}

//每隔一段时间检查taskManager中有没有需要执行的任务 
unsigned int __stdcall processTasks( LPVOID lpArg ){
	int taskQueueNum = 0 ;
	int d= 0;
	int taskNum = 0;
	Task * pTask = NULL;
	TaskQueue * ptr = NULL;

	while( true ){

		taskQueueNum = taskManager.getTaskQueueNum();	
		for( int i = 0 ; i < taskQueueNum ; ++ i ){
			taskManager.EnterSpecifyCriticalSection( i );
			ptr = taskManager.getSpecifyQueue(i);
			if( ptr != NULL ){
				d = 0;
				taskNum = ptr->getTaskNumber();
				if( taskNum > 64 )
					taskNum >>= 2;
				while( d < taskNum ){
					if( (pTask = ptr->getCurTask())->visit == 0 ){
						HANDLE h = (HANDLE)_beginthreadex( NULL , 0 , taskThread , (void *)i , 0 ,NULL);
						pTask->visit = 1;
						CloseHandle(h);
					}
					++d;
				}
			}
			taskManager.LeaveSpecifyCritialSection( i );
		}

		Sleep( LOOP_DELAY );
	}
	return 0;
}


unsigned int __stdcall listenThread( LPVOID lpArg ){
	WSADATA wsaData;
	SOCKET sockSrv;	//服务器SOCKET
	SOCKET sockClient;
	SOCKADDR_IN addrSrv;
	SOCKADDR_IN addrClient;
	string ip;

	int arg = (int)lpArg;
	int len = sizeof(SOCKADDR);
	int count = 0;
	int num = 0;
	int limit = 2 * BUF_SIZE;

	char * data =  ( char *)malloc( sizeof(char) * limit);
	char * pData = data;
	char * buffer = ( char *)malloc( sizeof(char) * BUF_SIZE );

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
	addrSrv.sin_port = htons(6000);

	int opt=1;
	setsockopt(sockSrv,SOL_SOCKET,SO_REUSEADDR,(char *)&opt,sizeof(opt));
	bind( sockSrv , (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	listen( sockSrv , 256);
	//Log("[listenThread]:start %d.\n" , arg );

	while( true ){
		sockClient = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
		log_connect ++;

		num = 0;
		count = 0;
		Sleep(LOOP_DELAY);

		if( sockClient != INVALID_SOCKET ){
			while( (num = recv( sockClient , buffer , BUF_SIZE , 0 ) ) > 0 ){
				count += num;
				if( count < limit ){
					memcpy( pData , buffer , num );
					pData += num;
				}
				else{
					break;
				}
				num = 0;
			}

			++ log_recv;
			Log("[server]:listen thread=%d,recv data = %d\n" , arg ,count);

			EnterCriticalSection(&taskManagerCriticalSection);
			taskManager.EnterSpecifyCriticalSection( arg ) ;
			TaskQueue * curQueue = taskManager.getSpecifyQueue(arg);
			bool ret = curQueue->push_back(data, count , arg , ip , ip);
			int y = 0;
			taskManager.LeaveSpecifyCritialSection( arg );

			if( ret == false ){
				for( int i = 0 ; i < 4 ; ++ i ){
					taskManager.EnterSpecifyCriticalSection( i );
					curQueue = taskManager.getSpecifyQueue( i );
					if(curQueue&&(!curQueue->isFull())){
						curQueue->push_back( data , count, i, ip , ip );
						y ++;
						break;
					}
					taskManager.LeaveSpecifyCritialSection(i);
				}

				if( y == 0 ){
					Log("[server]:all task queue full\n");
					++ log_throw ;
				}
				//server busy
				//通知客户端重新发送
			}
			
			LeaveCriticalSection(&taskManagerCriticalSection);
			closesocket( sockClient );
		}
	}

	free( data );
	free( buffer );
	return 0;
}

#endif // !SERVER_H_
