CC=gcc
FLAGS=-I.

all: tcpclient tcpserver udpclient udpserver

tcpclient:
	$(CC) tcpclient.c -o tcpclient $(FLAGS)

tcpserver:
	$(CC) tcpserver.c -o tcpserver $(FLAGS)

udpclient:
	$(CC) udpclient.c -o udpclient $(FLAGS)

udpserver:
	$(CC) udpserver.c -o udpserver $(FLAGS)

clean:
	rm tcpclient tcpserver udpclient udpserver