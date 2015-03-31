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

#include "list.h"
#include <utility>
#include <string>
#include <iostream>
#include <cstdlib>
#include <process.h>
#include <Winsock2.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib") 

#define SRV_ERROR -1
#define SRV_MAX_TASK_QUEUE_LENGTH 25
#define SRV_MAX_TASK_QUEUE_NUM 6

#define S_TASK_INI 2
#define S_TASK_DONE 1
#define S_TASK_STANDBY 0

using std::pair;
using std::string;
//using std::list;
using std::iterator;
using std::cout;
using std::endl;

#ifdef DEV_DEBUG

#endif

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

	bool push_back( char * src , int length , string ward = "NULL" , string target =  "NULL" ){
		if( length < 0 || src == NULL ){
			printf("error in taskQueue.pushBack\n");
			return false;
		}

		if( curTaskNumber > SRV_MAX_TASK_QUEUE_LENGTH ){
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
**
**         ��taskQueues����������������У������Ԫ�����ζ�ӦtaskQueueCriticalSections�����е�Ԫ�ء�
**         taskQueueCriticalSections��Ԫ��������ʶ��Ӧλ�õĶ����Ƿ����ڱ�����
**         taskmanager���е�ÿһ��������Ҫʹ��EnterCriticalSection������LeaveCriticalSection������ȷ��ÿ
**	��������ͬһʱ��ֻ��һ���߳��ڷ��ʡ�
**
**         ����taskManager�ĳ����������͹��캯��֮������ⷽ��ʱ,��Ҫ�ڵ���֮ǰEnterCriticalSection,��
**	��֮��LeaveCriticalSection����ȷ��taskManager��ͬһʱ��ֻ��Ψһ���̲߳�����ȫ�ֱ���
**	taskManagerCriticalSectionΪtaskmanager�Ĺؼ��ֶΡ�ÿ���������ֻ����һ��TaskManager�Ķ���
**
**Author  :yuanyi
**Date    :2015/3/31
**
********************************************************************************/
class TaskManager{
private:
	int curQueueNumber;
	TaskQueue taskQueues[SRV_MAX_TASK_QUEUE_NUM + 1];
	CRITICAL_SECTION taskQueueCriticalSections[SRV_MAX_TASK_QUEUE_NUM + 1];
public:
	TaskManager(){
		curQueueNumber = 0;
		for( int i = 0 ; i < SRV_MAX_TASK_QUEUE_NUM ; ++ i ){
			if( InitializeCriticalSectionAndSpinCount( taskQueueCriticalSections + i , 4096) != TRUE){
				puts("[taskManager]:create cs failed.");
				exit(EXIT_FAILURE);
			}
		}

		if( InitializeCriticalSectionAndSpinCount( &taskManagerCriticalSection , 4096) != TRUE){
			puts("[taskManager]:create global cs failed.");
			exit(EXIT_FAILURE);
		}

	}

	~TaskManager(){
		for( int i = 0 ; i < SRV_MAX_TASK_QUEUE_NUM ; ++ i ){
			DeleteCriticalSection( taskQueueCriticalSections + i );
		}
		WSACleanup();
	}

	bool registerQueue( string wardMac ,string targetMac){
		if( curQueueNumber == SRV_MAX_TASK_QUEUE_NUM )
			return false;

		for( int i = 0 ; i < curQueueNumber ; ++ i ){
			EnterCriticalSection( taskQueueCriticalSections + i );
			if( taskQueues[i].macExist(wardMac) || taskQueues[i].macExist( targetMac) ){
				EnterCriticalSection( taskQueueCriticalSections + i );
				return true;
			}
			LeaveCriticalSection( taskQueueCriticalSections + i );
		}

		EnterCriticalSection( taskQueueCriticalSections + curQueueNumber);
		taskQueues[curQueueNumber].setKey( pair<string,string>(wardMac,targetMac) );
		LeaveCriticalSection( taskQueueCriticalSections + curQueueNumber );
		curQueueNumber ++;

		return true;
	}

	bool addTask(int index , char * src , int length , string ward = "NULL" , string target = "NULL"){
		bool ret = true;
		if( index < 0 ){
			ret= false;
		}

		if( src == NULL || length < 0 ){
			ret=false;
		}

		if( ret == true ){
			EnterCriticalSection( taskQueueCriticalSections + index );
			ret = taskQueues[index].push_back( src , length , ward , target);
			LeaveCriticalSection( taskQueueCriticalSections + index );
		}
		return ret;
	}

	bool removeFirst( int index ){
		if( index < 0 ){
			return false;
		}

		if( taskQueues[index].getTaskNumber() == 0 ){
			return false;
		}

		EnterCriticalSection( taskQueueCriticalSections + index );
		taskQueues[index].pop_front();
		LeaveCriticalSection( taskQueueCriticalSections + index );

		return true;
	}

	int findByMac( string mac ){
		int ret = -1;
		for( int i = 0 ; i < curQueueNumber ; ++ i ){
			EnterCriticalSection( taskQueueCriticalSections + i );
			if( taskQueues[i].macExist( mac ) ){
				ret = i;
				LeaveCriticalSection( taskQueueCriticalSections + i );
				break;
			}
			LeaveCriticalSection( taskQueueCriticalSections + i );
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
			ret = taskQueues + index;
		}
		return ret;
	}

	void EnterSpecifyCriticalSection( int index ){
		EnterCriticalSection( taskQueueCriticalSections + index );
	}

	void LeaveSpecifyCritialSection( int index ){
		LeaveCriticalSection( taskQueueCriticalSections + index );
	}

	static int getMaxQueueNum(void ){
		return SRV_MAX_TASK_QUEUE_NUM;
	}

	void printAll( void ){
		cout << "[taskManager]task queue number:" << curQueueNumber << endl;
		for( int i = 0 ; i < curQueueNumber ; ++ i ){
			EnterCriticalSection(taskQueueCriticalSections  + i );
			cout  << "[taskQueue " << i << ']' <<endl;
			taskQueues[i].printQueueInfo();
			LeaveCriticalSection(taskQueueCriticalSections  + i );
		}
	}
};

#endif // !TASK_H
