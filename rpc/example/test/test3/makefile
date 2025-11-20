CFLAG= -std=c++11 -I../../../build/release-install-cpp11/include/
LFLAG=-L../../../build/release-install-cpp11/lib -lmuduo_net -lmuduo_base -ljsoncpp -pthread

.PHONY:all
all:publish_client server subscribe_client
publish_client:publish_client.cc
	g++ -g $(CFLAG) -o $@ $^ $(LFLAG)

server:server.cc
	g++ -g $(CFLAG) -o $@ $^ $(LFLAG)

subscribe_client:subscribe_client.cc
	g++ -g $(CFLAG) -o $@ $^ $(LFLAG)

.PHONY:clean
clean:
	rm -rf server publish_client subscribe_client

