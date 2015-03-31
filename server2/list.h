#ifndef LISTTEMPLATE
#define LISTTEMPLATE

//#define DEBUG

#ifdef DEBUG
#include <iostream>
using std::endl;
using std::cout;
#endif

#include <cstdlib>

template <typename type>
class list{
private:
	struct node {
		type data;
		node * pNext;
	};

	int num;
	node root;

	node * rear;
public:
	list(){
		num = 0;
		root.pNext = NULL;
		rear = &root;
	}

	list( const type & data ){
		num = 0;
		root.data = data;
		root.pNext = NULL;
		rear = &root;
	}

	~list( void ){
		node * ptr = root.pNext;
		node * st = NULL;

		while( ptr ){
			st = ptr;
			ptr = ptr->pNext;
			delete st;
		}

		rear = NULL;
		root.pNext = NULL;
		num = 0;
	}

	int Size( void ){
		return num;
	}

	node * travel_list( const int & count ){
		if( count < 0 || count >= num )
			return NULL;
		node * ptr = root.pNext;
		int i = 0;

		while( i < count ){
			ptr = ptr->pNext;
			i ++;
		}

		return ptr;
	}

	void push_back( const type & data ){
		node * newNode = new node;
		newNode->data = data;
		newNode->pNext = NULL;

		rear->pNext = newNode;
		rear = newNode;

		num ++;
	}

	void push_front( const type & data ){
		node * newNode = new node ;
		newNode->data = data;

		if( num ){
			newNode->pNext = root.pNext;
			root.pNext = newNode;
		}
		else{
			newNode->pNext = NULL;
			root.pNext = newNode;
			rear = newNode;
		}

		num ++;
	}

	bool pop_front( type & data ){
		node * ptr = NULL;
		if( num == 1 ){
			data = rear->data;
			delete rear;
			rear = &root;
			root.pNext = NULL;
		}
		else if( num > 1 ){
			data = root.pNext->data;
			ptr = root.pNext;
			root.pNext = ptr->pNext;
			delete ptr;
		}
		else{
			return false;
		}
		num --;
		return true;
	}

	bool pop_front ( void ){
		node * ptr = NULL;
		if( num == 1 ){
			delete rear;
			rear = &root;
			root.pNext = NULL;
		}
		else if( num > 1 ){
			ptr = root.pNext;
			root.pNext = ptr->pNext;
			delete ptr;
		}
		else{
			return false;
		}
		num --;
		return true;
	}

	bool pop_back( type & data ){
		if( num == 1 ){
			data = rear->data;
			delete rear;
			rear = &root;
			root.pNext = NULL;
		}
		else if( num > 1 ){
			data = rear->data;
			delete rear;
			rear = travel_list( num - 2 );
			rear->pNext = NULL;
		}
		else{
			return false;
		}

		num --;
		return true;
	}

	bool pop_back( void ){
		if( num == 1 ){
			delete rear;
			rear = &root;
			root.pNext = NULL;
		}
		else if( num > 1 ){
			delete rear;
			rear = travel_list( num - 2 );
			rear->pNext = NULL;
		}
		else{
			return false;
		}

		num --;
		return true;
	}

	bool pop_position( const int & pos , type & data ){
		if( pos < 0 || pos >= num )
			return false;
		node * st = NULL;
		node * temp = NULL;
		
		if( num == 1 ){
			data = rear->data;
			delete rear;
			root.pNext = NULL;
			rear = &root;
		}
		else if( pos == num - 1 ){
			st = travel_list( pos - 1 );
			data = rear->data;
			delete rear;
			st ->pNext= NULL;
			rear = st;
		}
		else if( pos == 0 ){
			st = root.pNext;
			data = st->data;
			root.pNext = st->pNext;
			delete st;
		}
		else{
			st = travel_list( pos - 1 );
			temp = st->pNext;
			st-pNext = tenp->pNext;
			data = temp->data;
			delete temp;
		}

		num --;
		return false;
	}

	bool pop_position ( const int & pos ){
		if( pos < 0 || pos >= num )
			return false;
		node * st = NULL;
		node * temp = NULL;
		
		if( num == 1 ){
			delete rear;
			root.pNext = NULL;
			rear = &root;
		}
		else if( pos == num - 1 ){
			st = travel_list( pos - 1 );
			delete rear;
			st->pNext = NULL;
			rear = st;
		}
		else if( pos == 0 ){
			st = root.pNext;
			root.pNext = st->pNext;
			delete st;
		}
		else{
			st = travel_list( pos - 1 );
			temp = st->pNext;
			st->pNext = temp->pNext;
			delete temp;
		}

		num --;
		return false;
	}

	int findData( const type & data ){//tode
		return 1;
	}

	type & operator[] ( const int & num ){
		return travel_list( num )->data;
	}

	type front(void){
		return root.pNext->data;
	}

	bool empty( void ){
		return ( num == 0 );
	}

#ifdef DEBUG
	void show( void ){
		node * ptr = root.pNext;
		while( ptr ){
			cout << ptr->data << ' ';
			ptr = ptr->pNext;
		}
		putchar('\n');
	}
#endif // DEBUG
};


#endif // !LISTTEMPLATE
