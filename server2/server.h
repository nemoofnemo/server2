#ifndef SERVER_H_
#define SERVER_H_

#include "taskDataStructure.h"
#include "cmdFlag.h"
#define BUF_SIZE 2048000
#define LOOP_DELAY 1

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
//dump
int log_dump;
//invalid
int log_invalid;
//���������̵߳��������
const int srv_max_thread = 150;
//��ǰ����������ݵ��̵߳�����
int srv_cur_thread = 0;

//ʵ�������߼����߳�
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

	char * data = (char *)malloc(sizeof(char)*length);
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
		for( int i = 0 ; i < dataSectionLen ; ++ i ){
			putchar(*(dataSection + i ));
		}
		putchar('\n');
		++ log_dump;
		break;
	default:
		Log("[server]:Invalid CMD flag = %x\n",cmdFlag);
		log_invalid ++;
		//return error message
		
		break;
	}

	free( data );
	++ log_use;
	Log("[server]:connection=%d,recv=%d,throw=%d,use=%d,dump=%d,invalid=%d\n" , log_connect , log_recv , log_throw , log_use , log_dump,log_invalid);
	return 0;
}

//ÿ��һ��ʱ����taskManager����û����Ҫִ�е����� 
unsigned int __stdcall processTasks( LPVOID lpArg ){
	int taskQueueNum = 0 ;
	int d= 0;
	int taskNum = 0;
	Task * pTask = NULL;
	const int handleArrLen = taskManager.getMaxQueueNum();

	while( true ){
		taskQueueNum = taskManager.getTaskQueueNum();	
		for( int i = 0 ; i < taskQueueNum ; ++ i ){
			taskManager.EnterSpecifyCriticalSection( i );
			TaskQueue * ptr = taskManager.getSpecifyQueue(i);
			if( ptr != NULL ){
				d = 0;
				taskNum = ptr->getTaskNumber();
				while( d < taskNum ){
					if( (pTask = ptr->getCurTask())->visit == false ){
						HANDLE h = (HANDLE)_beginthreadex( NULL , 0 , taskThread , (void *)i  , 0 ,NULL);
						pTask->visit = true;
						CloseHandle(h);
					}
					++d;
				}
			}
			taskManager.LeaveSpecifyCritialSection( i );
		}
		//wait 
		//Sleep( LOOP_DELAY );
	}
	return 0;
}

/**********************************************************************************/
CRITICAL_SECTION recvThreadCS;
struct recvDataArg{
	SOCKET sock;	//socket that need to call function recv
	char ip[16];	//ip of socket
};

//�������ݵ��߳�
unsigned int __stdcall recvData( LPVOID lpArg ){
	Sleep(1);
	SOCKET sock = ((recvDataArg*)lpArg)->sock;
	string ip(((recvDataArg*)lpArg)->ip);
	LeaveCriticalSection( &recvThreadCS );

	int count = 0;
	int num = 0;
	int limit = 2 * BUF_SIZE;

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
	Log("[server]:recv data = %d\n" , count);

	EnterCriticalSection( & taskManagerCriticalSection );
	taskManager.EnterSpecifyCriticalSection( 0 ) ;
	TaskQueue * curQueue = taskManager.getSpecifyQueue(0);
	if( !curQueue->push_back(data, count , ip , ip)){
		Log("[server]:specify task queue full\n");
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
		EnterCriticalSection( &taskManagerCriticalSection );
		bool ret = taskManager.registerQueue(ward,target );
		LeaveCriticalSection( &taskManagerCriticalSection );
		return ret;
	}

	void startListening( void ){
		recvDataArg arg;
		listener.startBind();
		listener.prepareListen(50);

		Log("[server]:server crital section created.\n");
		if(InitializeCriticalSectionAndSpinCount(&recvThreadCS,4000) != TRUE){
			exit(EXIT_FAILURE);
		}
		
		Log("[server]:start.\n");
		//��������ѭ��
		while( true ){
			Log("[server]:listening.\n");
			Log("[server]:connection=%d,recv=%d,throw=%d,use=%d,dump=%d,invalid=%d\n" , log_connect , log_recv , log_throw , log_use , log_dump,log_invalid);
			
			//��֤�����Ļ������.��thread recvData��leave
			EnterCriticalSection( &recvThreadCS );
			if((arg.sock = listener.acceptClient()) == INVALID_SOCKET ){
				Log("[server]:accept falied.\n");
				LeaveCriticalSection( &recvThreadCS );
				continue;
			}
			++ log_connect;
			strcpy( arg.ip , listener.getClientIp() );
			
			//�������߳̽�������
			HANDLE h = INVALID_HANDLE_VALUE;
			if( arg.sock != INVALID_SOCKET ){
				if( srv_cur_thread < srv_max_thread ){
					h = (HANDLE)_beginthreadex( NULL , 0 , recvData , &arg , 0 , NULL );
					++ srv_cur_thread;
					Log("[server]:A new thread has been created.\n");
				}
				else{
					Log("[server]:thread pool full.throw.\n");
					closesocket(arg.sock);
					++ log_throw;
					Sleep(1);
					LeaveCriticalSection( &recvThreadCS );
				}
				CloseHandle( h );
			}
		}

		DeleteCriticalSection(&recvThreadCS );
		Log("[server]:stop.\n");
	}

};

#endif // !SERVER_H_
