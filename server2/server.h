#ifndef SERVER_H_
#define SERVER_H_
/********************************************************************************
**
**Function:server�����߼�
**Args    :
**Return  :
**Detail  :ʵ��server�Ĺ���
**
**Author  :Ԭ��
**Date    :2015/3/29
**
********************************************************************************/
#include "taskDataStructure.h"
#include "cmdFlag.h"
#define BUF_SIZE 1024000

/********************************************************************************
**
**Function:transferModule
**Args    :
**Return  :
**Detail  :socket���ݴ���ģ�顣
**         ���캯����ʼ��WSA�⡣
**         �����ݴ���֮ǰ����Ҫ���ε���startBind��prepareListen��
**         ��̬����checkLengthΪ�������ȡ�
**
**Author  :Ԭ��
**Date    :2015/3/29
**
********************************************************************************/
class transferModule{
private:
	WSADATA wsaData;
	SOCKET sockSrv;	//������SOCKET
	SOCKET sockConn;	//���ӷ������Ŀͻ��˵�SOCKET

	SOCKADDR_IN addrSrv;
	SOCKADDR_IN  addrClient;

public:
	//Ĭ�Ϲ��캯����Ĭ�Ϸ���������6000�˿�
	transferModule(){
		if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 ) {
			exit(EXIT_FAILURE);
		}

		if ( LOBYTE( wsaData.wVersion ) != 2 ||	HIBYTE( wsaData.wVersion ) != 2 ){
			WSACleanup();
			exit(EXIT_FAILURE);
		}

		sockSrv = socket(AF_INET, SOCK_STREAM, 0);
		// ��INADDR_ANYת��Ϊ�����ֽ��򣬵��� htonl(long��)��htons(����)
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
		// ��INADDR_ANYת��Ϊ�����ֽ��򣬵��� htonl(long��)��htons(����)
		addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY); 
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(port);

	}

	//�����������ر�socket
	~transferModule(){
		closesocket( sockSrv );
		closesocket( sockConn );
	}

	//��������
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

	//�������󶨡���Ҫ��prepareListen֮ǰ���á�
	int startBind( void ){
		return bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	}

	//ʹ������׼����������
	int prepareListen(const int num = 15){
		return listen( sockSrv , num );
	}

	//��ȡserver��socket
	SOCKET getServerSocket( void ){
		return this->sockSrv;
	}

	//��ȡ�ͻ�������socket����������ɹ�����socket�����򷵻�INVALID_SOCKET
	SOCKET acceptClient( void ){
		int len = sizeof(SOCKADDR);
		sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
		return sockConn ;
	}

	SOCKET getClientSocket( void ){
		return sockConn;
	}

	//��ȡ�ͻ�����Ϣ
	char * getClientIp( void ){
		return inet_ntoa(addrClient.sin_addr);
	}

	//�رտͻ���socket
	int closeClient( void ){
		return closesocket( sockConn );
	}

	//send�ĵ�һ�������ǿͻ��˵�socket
	//��ͻ��˷���
	int sendData( char * data , int length){
		return send( sockConn , data , length , 0 );
	}

	//��ͻ��˵ļ����̷߳�������//need test
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

	//�ӿͻ��˽��շ���������������
	//recv�����ĵ�һ�������Ƿ������ݵ�socket��
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

//========================================================

//ȫ�ֱ���taksManager�������������
//check taskmanager critical section
TaskManager taskManager;
//ȫ�ֱ���listener���������
transferModule listener;

//����ͳ��
int log_connect;
int log_recv;
int log_use;
int log_throw;

//ʵ�������߼����߳�
//���߳�����ʱ��ǰ���е�isVistingΪtrue�������߳̽���ʱisVisiting��Ϊfalse
unsigned int __stdcall taskThread( LPVOID lpArg ){//should check length!!!!
	EnterCriticalSection( & taskManagerCriticalSection );
	int arg = *((int*)lpArg);
	taskManager.EnterSpecifyCriticalSection( arg );
	
	//get task
	TaskQueue * curQueue = taskManager.getSpecifyQueue(arg);
	Task curTask =*( curQueue->getCurTask());
	
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
	case REGISTER://need test
		puts("[server]:register");
		targetMac = string( dataSection , dataSection + 7 );
		if( targetMac.length() == 6 ){
			if(taskManager.registerQueue( taskMac , targetMac)){
				//register success
			}
			else{
				//failed .task manager full
			}
		}
		else{
			//failed.invalid data
		}
		
		taskManager.removeFirst( 0 );
		break;
	case SENDTEXT:
		puts("[server]:msg");
		listener.sendDataToListener( "a" ,2,taskManager.getSpecifyQueue(0)->getCurTargetIP() , 6002);
		taskManager.removeFirst( 0 );
		break;
	case SRV_QUIT:
		taskManager.removeFirst( 0 );
		puts("[server]:quit.");
		exit(0);
		break;
	default:
		puts("[server]:ok");
		taskManager.removeFirst( 0 );
		break;
	}

	++ log_use;
	taskManager.LeaveSpecifyCritialSection( arg );
	printf("[server]:connection=%d,recv=%d,throw+%d,use=%d\n" , log_connect , log_recv , log_throw , log_use);
	LeaveCriticalSection( & taskManagerCriticalSection );
	return 0;
}

//ÿ��һ��ʱ����taskManager����û����Ҫִ�е����� 10ms
unsigned int __stdcall processTasks( LPVOID lpArg ){
	int taskNum = 0;
	int taskQueueNum =0 ;
	const int handleArrLen = taskManager.getMaxQueueNum();
	int * arr = (int *)malloc(sizeof(int)*handleArrLen);
	HANDLE * handleArr = (HANDLE *)malloc( sizeof(HANDLE) * handleArrLen);

	for( int k = 0 ; k < handleArrLen ; ++ k ){
		arr[k] = k;
	}

	while( true ){
		EnterCriticalSection( &taskManagerCriticalSection );
		taskQueueNum = taskManager.getTaskQueueNum();	
		for( int i = 0 ; i < taskQueueNum ; ++ i ){		
			TaskQueue * ptr = taskManager.getSpecifyQueue(i);
			taskNum = ptr->getTaskNumber();
			if( taskNum > 0){
				handleArr[i] = (HANDLE)_beginthreadex( NULL , 0 , taskThread , arr + i , 0 ,NULL);		//problem here
			}
			CloseHandle(handleArr[i]);//need test
		}
		//WaitForMultipleObjects( taskQueueNum , handleArr , true , INFINITE );
		LeaveCriticalSection( &taskManagerCriticalSection );
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

//��������������
unsigned int __stdcall recvData( LPVOID lpArg ){
	SOCKET sock = ((recvDataArg*)lpArg)->sock;
	string ip(((recvDataArg*)lpArg)->ip);
	LeaveCriticalSection( &recvThreadCS );
	
	int count = 0;
	int num = 0;
	int limit = 10 * BUF_SIZE;

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
			return SRV_ERROR;
		}
		num = 0;
	}

	closesocket(sock);

	//TODO : need find by mac.check length.ip
	//judge
	++ log_recv;
	printf("[server]:recv data = %d" , count);
	/*for( int d = 0 ;d < count;d ++){
		putchar( data[d]);
	}*/
	
	EnterCriticalSection( &taskManagerCriticalSection );
	if( !taskManager.addTask( 0 , data , count , ip.c_str() , ip.c_str() ) ){
		puts("[server]:specify task queue full");
		++ log_throw ;
		//֪ͨ�ͻ������·���
	}
	LeaveCriticalSection( &taskManagerCriticalSection );

	//release
	free( data );
	free( buffer );

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
		listener.prepareListen();

		puts("[server]:server crital section created.");
		if(InitializeCriticalSectionAndSpinCount(&recvThreadCS,4000) != TRUE){
			exit(EXIT_FAILURE);
		}
		
		puts("[server]:start.");
		//��������ѭ��
		while( true ){
			puts("[server]:listening.");

			//��֤�����Ļ������.��thread recvData��leave
			EnterCriticalSection( &recvThreadCS );
			if((arg.sock = listener.acceptClient()) == INVALID_SOCKET ){
				puts("[server]:accept falied.");
				continue;
			}
			++ log_connect;
			puts("[server]:connection accepted.");
			memcpy( arg.ip , listener.getClientIp() , 16 );
			
			//start new thread
			if( arg.sock != INVALID_SOCKET ){
				HANDLE h = (HANDLE)_beginthreadex( NULL , 0 , recvData , &arg , 0 , NULL );
				puts("[server]:A new thread has been created.");
				CloseHandle( h );//need test
			}
		}

		DeleteCriticalSection(&recvThreadCS );
		puts("[server]:stop.");
	}

};

#endif // !SERVER_H_
