CFLAG= -std=c++11 -I ../../bitrpc/build/release-install-cpp11/include/
LFLAG=-L../../bitrpc/build/release-install-cpp11/lib -lmuduo_net -lmuduo_base -pthread

all:server client
server: server.cpp
	g++ $(CFLAG) $^ -o $@  $(LFLAG)
client: client.cpp
	g++ $(CFLAG) $^ -o $@  $(LFLAG)
.PHONY:clean
clean:
	rm -rf server client