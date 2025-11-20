CFLAG= -std=c++11 -I../../../build/release-install-cpp11/include/
LFLAG=-L../../../build/release-install-cpp11/lib -lmuduo_net -lmuduo_base -ljsoncpp -pthread

.PHONY:all
all:regi_server server client
regi_server:registry_server.cc
	g++ -g $(CFLAG) -o $@ $^ $(LFLAG)

server:rpc_server.cc
	g++ -g $(CFLAG) -o $@ $^ $(LFLAG)

client:rpc_client.cc
	g++ -g $(CFLAG) -o $@ $^ $(LFLAG)

.PHONY:clean
clean:
	rm -rf server client regi_server

