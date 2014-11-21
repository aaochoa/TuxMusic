CC=g++ -std=c++11
#CC=clang++ -std=c++11
#ZMQ=/home/utp/cs/zmq
ZMQ=/usr/local
ZMQ_LIBS=$(ZMQ)/lib
ZMQ_HDRS=$(ZMQ)/include


all: Server

Server: Server.o
	$(CC) -L$(ZMQ_LIBS) -o Server Server.o -lzmq -lczmq 

Server.o: Server.cpp 
	$(CC) -I$(ZMQ_HDRS) -c Server.cpp 




clean:
	rm -rf Server Server.o *~
	
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utp/cs/zmq/lib
