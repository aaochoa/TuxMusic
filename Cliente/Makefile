CC=g++ -std=c++11
#CC=clang++ -std=c++11
#ZMQ=/home/utp/cs/zmq
ZMQ=/usr/local
ZMQ_LIBS=$(ZMQ)/lib
ZMQ_HDRS=$(ZMQ)/include


all: Client


Client: Client.o
	$(CC) -L$(ZMQ_LIBS) -o Client Client.o -lpthread -lzmq -lczmq  -lsfml-audio -lsfml-system 

Client.o: Client.cpp 
	$(CC) -I$(ZMQ_HDRS) -c Client.cpp 


clean:
	rm -rf Client Client.o  *~
	
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utp/cs/zmq/lib
