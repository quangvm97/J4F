all:
	make server
	make peer

server: server.cpp
	g++ server.cpp -o server -std=c++11

peer: peer.cpp
	g++ peer.cpp -o peer

clean:
	rm server peer
