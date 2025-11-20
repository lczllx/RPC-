CFLAG= -std=c++11 -I../../../build/release-install-cpp11/include/ -I../../../
LFLAG=-L../../../build/release-install-cpp11/lib -lmuduo_net -lmuduo_base -ljsoncpp -pthread

.PHONY: all clean

all: registry_server provider_server consumer_client

registry_server: registry_server.cc
	g++ -g $(CFLAG) -o $@ $^ $(LFLAG)

provider_server: provider_server.cc
	g++ -g $(CFLAG) -o $@ $^ $(LFLAG)

consumer_client: consumer_client.cc
	g++ -g $(CFLAG) -o $@ $^ $(LFLAG)

clean:
	rm -rf registry_server provider_server consumer_client

