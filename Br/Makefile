CC=g++ -std=c++11
#CC=clang++ -std=c++11
#ZMQ=/home/utp/cs/zmq
ZMQ=/usr/local
ZMQ_LIBS=$(ZMQ)/lib
ZMQ_HDRS=$(ZMQ)/include


all:  Broker

Broker: Broker.cpp
	$(CC) -I$(ZMQ_HDRS) -c Broker.cpp
	$(CC) -L$(ZMQ_LIBS) -o Broker Broker.o -lzmq -lczmq


	

clean:
	rm -rf  Broker Broker.o *~
	
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utp/cs/zmq/lib
