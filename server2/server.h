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
#define BUF_SIZE 2048000
#define LOOP_DELAY 10

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
	transferModule(const int & port = 6000 ){
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

/**********************************************************************************/
//ȫ�ֱ���taksManager�������������
TaskManager taskManager;	//warning:one program,one Taskmanager Obeject!!!
//ȫ�ֱ���listener���������
transferModule listener;	//listener

/**********************************����ͳ��****************************************/
//������������
int log_connect;
//�������ݰ��ĸ���
int log_recv;
//��������������ݰ�����
int log_use;
//�������ӵ������ݰ�����
int log_throw;
/**********************************************************************************/

//���������̵߳��������
const int srv_max_thread = 100;
//��ǰ����������ݵ��̵߳�����
int srv_cur_thread = 0;

//ʵ�������߼����߳�
unsigned int __stdcall taskThread( LPVOID lpArg ){
	taskManager.EnterSpecifyCriticalSection( *((int*)lpArg) );
	int arg = *((int*)lpArg);
	TaskQueue * curQueue = taskManager.getSpecifyQueue( arg ) ;

	if(curQueue == NULL || curQueue->getTaskNumber() == 0 ){
		taskManager.LeaveSpecifyCritialSection( arg );
		return 0;
	}

	Task curTask = *(curQueue->getCurTask());
	int length = curTask.length ;//!!
	char * data = curTask.data;

	if( length == 0 ){
		taskManager.LeaveSpecifyCritialSection( arg );
		return 0;
	}

	//.........................
	char cmdFlag = *data;
	char formatFlag = *(data+1);
	char * dataSection = data;
	string taskMac( data + 10 , data + 16 );
	string targetMac;

	if( length > 0 && data != NULL){
		dataSection += 32;
	}

	puts("[server]:task thread running");
	switch(cmdFlag){//need to change status filed in Task
	case TAKEPIC:
		//send cmd TAKEPIC to target

		curQueue->pop_front();
		break;
	case SENDPIC:
		//send picture data to ward

		curQueue->pop_front();
		break;
	case SENDTEXT://debug
		puts("[server]:msg");
		listener.sendDataToListener( "a" ,2,curQueue->getCurTargetIP() , 6002);
		curQueue->pop_front();
		break;
	case SRV_QUIT://debug
		curQueue->pop_front();
		puts("[server]:quit.");
		exit(0);
		break;
	default:
		puts("[server]:Invalid CMD flag.");
		//return error message
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

/**********************************************************************************/
//ÿ��һ��ʱ����taskManager����û����Ҫִ�е����� 
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
		//wait 
		Sleep( LOOP_DELAY );
	}
	return 0;
}

/**********************************************************************************/
//warning.this struct is the argument of thread recvData
CRITICAL_SECTION recvThreadCS;
struct recvDataArg{
	SOCKET sock;	//socket that need to call function recv
	char ip[16];	//ip of socket
};

//�������ݵ��߳�
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
	//if mac frame is not exist in any taskQueue's key pair , throw;else add task to specify TaskQueue.
	//if length is invalid . throw
	//if ip is different from curTargetIp or curWardIp , update ip.
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
		//server busy
		//֪ͨ�ͻ������·���
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
/**********************************************************************************/

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
		//��������ѭ��
		while( true ){
			puts("[server]:listening.");

			//��֤�����Ļ������.��thread recvData��leave
			EnterCriticalSection( &recvThreadCS );
			if((arg.sock = listener.acceptClient()) == INVALID_SOCKET ){
				puts("[server]:accept falied.");
				LeaveCriticalSection( &recvThreadCS );
				continue;
			}
			++ log_connect;
			puts("[server]:connection accepted.");
			memcpy( arg.ip , listener.getClientIp() , 16 );
			
			//�������߳̽�������
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
/**********************************************************************************/

#endif // !SERVER_H_
