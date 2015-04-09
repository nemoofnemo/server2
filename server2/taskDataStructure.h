#ifndef TASK_H
#define TASK_H
#define DEV_DEBUG

/********************************************************************************
**
**Function:���ݽṹ�����ļ�
**Detail  :��������Ĵ洢��ʽ��������������ط�������ʵ�֡�
**
**Author  :Ԭ��
**Date    :2015/3/29
**
********************************************************************************/

#include <list>
#include <utility>
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <process.h>
#include <Winsock2.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib") 

#define SRV_ERROR -1
#define SRV_MAX_TASK_QUEUE_LENGTH 25
#define SRV_MAX_TASK_QUEUE_NUM 10

using std::pair;
using std::vector;
using std::string;
using std::list;
using std::iterator;
using std::cout;
using std::endl;

/********************************************************************************
**
**Function:����洢�ṹ���弰ʵ��
**Detail  :��������Ĵ洢�ṹ��
**         
**         data ָ��������Ҫ������
**         
**         lengthΪ���ݳ���
**
**Author  :Ԭ��
**Date    :2015/3/28
**
********************************************************************************/
class Task{
public:
	char * data;	//��TaskQueue��Push�����з����ڴ棬��pop���ͷ�
	int length;

public:
	Task( char * src , int length ){
		data = src;
		this->length = length;
	}

	Task(){
		data = NULL;
		length = 0;
	}
};

/********************************************************************************
**
**Function:������ж��弰ʵ��
**Detail  :ʵ��������еĲ�����
**         curTaskNumber��ǰ������������
**         maxTaskNumber���������
**         key��ʶ��ǰ������С�ͨ�����ֶ�ȷ����̨�豸�����ӡ�pair��һ���ֶ�Ϊ���ƶ�mac��ַ���ڶ����ֶ�Ϊ���ض�mac��ַ��
**         curWardIP���浱ǰ���ƶ˵�ip
**         curTargetIP���浱ǰ���ض�ip
**
**Author  :Ԭ��
**Date    :2015/3/29
**
********************************************************************************/
class TaskQueue{
private:
	int curTaskNumber;
	int maxTaskNumber;
	pair<string,string> key;
	list<Task> taskList;

	string curWardIP;
	string curTargetIP;
public:
	TaskQueue(){
		curTaskNumber = 0;
		maxTaskNumber = SRV_MAX_TASK_QUEUE_LENGTH;
		key.first = "NULL";
		key.second = "NULL";
		curWardIP = "NULL";
		curTargetIP = "NULL";
	}

	TaskQueue(string wardMac , string targetMac , int maxNum = SRV_MAX_TASK_QUEUE_LENGTH){
		curTaskNumber = 0;
		maxTaskNumber = SRV_MAX_TASK_QUEUE_LENGTH;
		key.first = wardMac;
		key.second = targetMac;
		curWardIP = "NULL";
		curTargetIP = "NULL";
	}

	~TaskQueue(){
		while( !taskList.empty() ){
			free(taskList.front().data);
			taskList.pop_front();
		}
	}

	void clear( void ){
		while( !taskList.empty() ){
			free(taskList.front().data);
			taskList.pop_front();
		}
	}

	bool push_back( Task arg , string ward = "NULL" , string target =  "NULL" ){
		if( curTaskNumber > SRV_MAX_TASK_QUEUE_LENGTH ){
			printf("[taskQueue]:error in taskQueue.pushBack:queue is full\n");
			return false;
		}

		if( ward != curWardIP ){		
			curWardIP = ward;
		}

		if( target != curTargetIP ){
			curTargetIP = target;
		}

		taskList.push_back( arg );
		++ curTaskNumber;
		return true;
	}

	bool push_back( char * src , int length , string ward = "NULL" , string target =  "NULL" ){
		if( length < 0 || src == NULL ){
			printf("error in taskQueue.pushBack\n");
			return false;
		}

		if( curTaskNumber >= SRV_MAX_TASK_QUEUE_LENGTH ){
			printf("error in taskQueue.pushBack:queue is full\n");
			return false;
		}

		if( ward != curWardIP ){		
			curWardIP = ward;
		}

		if( target != curTargetIP ){
			curTargetIP = target;
		}

		//������������pop_front�������ͷ�
		char * st = (char*)malloc(sizeof(char)*length + 32 );	
		memcpy( st , src , length );

		taskList.push_back( Task( st , length ) );
		++ curTaskNumber;

		return true;
	}

	bool pop_front(){
		if( curTaskNumber > 0 ){
			-- curTaskNumber;
			free( taskList.front().data );
			taskList.pop_front();	
			return true;
		}
		else{
			printf("[taskQueue]:error in taskQueue.pop_front:queue is empty\n");
			return false;
		}
	}

	//ip

	string getCurWardIP( void ){
		return curWardIP;
	}

	string getCurTargetIP( void ){
		return curTargetIP;
	}

	void setIP( string wardIP , string targetIP ){
		curWardIP = wardIP;
		curTargetIP = targetIP;
	}

	//mac pair

	pair<string,string> getKey( void ){
		return key;
	}

	void setKey( pair<string,string> k ){
		key.first = k.first;
		key.second = k.second;
	}

	bool macExist( string mac ){
		if( mac == key.first || mac == key.second ){
			return true;
		}
		return false;
	}

	int getTaskNumber( void ){
		return curTaskNumber;
	}

	Task * getCurTask( void ){
		return &(taskList.front());
	}

	bool isFull( void ){
		return (curTaskNumber == SRV_MAX_TASK_QUEUE_LENGTH);
	}

	void printQueueInfo( void ){
		//using std::iterator;
		//list<Task>::iterator it = taskList.begin();
		
		cout << "[taskQueue]"<< "ward=" << key.first << ' ' << ",target=" << key.second << endl
			 << "[taskQueue]"<< "task num=" << curTaskNumber << endl
			 << "[taskQueue]"<< "ward ip=" << curWardIP << ",target ip=" << curTargetIP << endl;
		
		/*for( int i = 0 ; i < curTaskNumber ; ++ i ){
			if( it == taskList.end() )
				break;

			cout << '[' << i << ']' << "length=" << it->length << endl << "data=";
			for( int d = 0 ;d < it->length;d ++){
				putchar( it->data[d]);
			}
			putchar('\n');
			it ++;
		}*/
	}
};

//critical section of taskmanager.initialise in taskmanager constructor.(global)
CRITICAL_SECTION taskManagerCriticalSection;

/********************************************************************************
**
**Function:task manager
**Args    :
**Return  :
**Detail  :����������ࡣ
**         �������������У�processTasks�߳�ÿ��ѭ����ͬʱ�������ж��еĵ�һ������
**         ��taskQueues����������������У������Ԫ�����ζ�ӦtaskQueueCriticalSections�����е�Ԫ�ء�
**         taskQueueCriticalSections��Ԫ��������ʶ��Ӧλ�õĶ����Ƿ����ڱ�����
**         ����taskmanager���е�ÿһ��������Ҫ��ʹ�õ��������ʹ��EnterCriticalSection������LeaveCriticalSection������ȷ��ÿ
**		   ��������ͬһʱ��ֻ��һ���߳��ڷ��ʡ�
**         ȫ�ֱ���taskManagerCriticalSectionΪtaskmanager�Ĺؼ��ֶΡ�ÿ���������ֻ����һ��TaskManager�Ķ���
**Author  :yuanyi
**Date    :2015/3/31
**
********************************************************************************/
class TaskManager{
private:
	int curQueueNumber;
	//TaskQueue taskQueues[SRV_MAX_TASK_QUEUE_NUM + 1];
	//CRITICAL_SECTION taskQueueCriticalSections[SRV_MAX_TASK_QUEUE_NUM + 1];
	vector< pair<TaskQueue,CRITICAL_SECTION> > taskQueues;
public:
	TaskManager(){
		curQueueNumber = 0;
		if( InitializeCriticalSectionAndSpinCount( &taskManagerCriticalSection , 4096) != TRUE){
			puts("[taskManager]:create global cs failed.");
			exit(EXIT_FAILURE);
		}
	}

	~TaskManager(){
		DeleteCriticalSection( &taskManagerCriticalSection );
		for( int i = 0 ; i < curQueueNumber ; ++ i ){
			DeleteCriticalSection( &(taskQueues[i].second ) );
		}

		WSACleanup();
	}

	//need test
	bool registerQueue( string wardMac ,string targetMac){
		int i = 0;
		int count = -1;
		CRITICAL_SECTION cs;
		
		if( curQueueNumber == SRV_MAX_TASK_QUEUE_NUM ){
			return false;
		}
		
		for( i = 0; i < curQueueNumber ; ++ i ){
			EnterCriticalSection( &(taskQueues[i].second) );
		}

		for( i = 0 ; i < curQueueNumber ; ++ i ){
			if( taskQueues[i].first.macExist(wardMac) || taskQueues[i].first.macExist( targetMac ) ){
				++ count;
			}
			LeaveCriticalSection( &(taskQueues[i].second) );
		}

		if( count == -1 ){
			taskQueues.push_back( pair<TaskQueue,CRITICAL_SECTION>(TaskQueue( wardMac , targetMac ) , cs) );
			InitializeCriticalSectionAndSpinCount( &( taskQueues.back().second ) , 4096 );
			++ curQueueNumber ;
			return true;
		}
		return false;
	}

	//need test
	bool removeQueue(string mac){
		if( curQueueNumber == 0 ){
			return false;
		}

		int i = 0;
		int pos = -1;
		CRITICAL_SECTION cs;

		for( i = 0; i < curQueueNumber ; ++ i ){
			EnterCriticalSection( &(taskQueues[i].second) );
		}

		for( i = 0 ; i < curQueueNumber ; ++ i ){
			if( pos == -1 && taskQueues[i].first.macExist(mac) ){ 
				pos = i;
			}
			LeaveCriticalSection( &(taskQueues[i].second) );
		}

		if( pos >= 0 && pos < curQueueNumber ){
			cs = taskQueues[pos].second;
			EnterCriticalSection( &cs );
			taskQueues[pos].first.clear();
			taskQueues.erase( taskQueues.begin() + pos );
			LeaveCriticalSection( &cs );
			DeleteCriticalSection( &cs );
			curQueueNumber --;
			return true;
		}

		return false;
	}

	//need test
	int findByMac( string mac ){
		int ret = -1;
		for( int i = 0 ; i < curQueueNumber ; ++ i ){
			if( taskQueues[i].first.macExist( mac ) ){
				ret = i;
				break;
			}
		}
		return ret;	//cannot find -1.
	}

	int getTaskQueueNum( void ){
		int ret = curQueueNumber;
		return ret;
	}

	TaskQueue * getSpecifyQueue( int index ){
		TaskQueue * ret = NULL;
		if(index >= 0 && index < curQueueNumber ){
			ret = &(taskQueues[index].first);
		}
		return ret;
	}

	void EnterSpecifyCriticalSection( int index ){
		if( index < curQueueNumber && index >= 0 )
			EnterCriticalSection( &(taskQueues[index].second) );
	}

	void LeaveSpecifyCritialSection( int index ){
		if( index < curQueueNumber && index >= 0 )
			LeaveCriticalSection( &(taskQueues[index].second) );
	}

	static int getMaxQueueNum(void ){
		return SRV_MAX_TASK_QUEUE_NUM;
	}

};

#endif // !TASK_H
