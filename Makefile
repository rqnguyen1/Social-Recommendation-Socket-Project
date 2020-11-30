all:
	g++ -std=c++11 -o client client.cpp client.h
	g++ -std=c++11 -o mainserver servermain.cpp servermain.h
	g++ -std=c++11 -o serverA serverA.cpp serverA.h
	g++ -std=c++11 -o serverB serverB.cpp serverB.h

.PHONY: mainserver
mainserver:
	./mainserver

.PHONY: serverA
serverA:
	./serverA

.PHONY: serverB
serverB:
	./serverB
