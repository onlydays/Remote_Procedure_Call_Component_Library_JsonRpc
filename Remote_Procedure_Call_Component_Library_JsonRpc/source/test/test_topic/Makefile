# 关键修改：把相对路径换成muduo的绝对路径
# 原CFLAG用了../../../相对路径，现在替换为绝对路径
CFLAG= -std=c++11 -I /home/zby/bitrpc/build/release-install-cpp11/include/
# 原LFLAG用了../../../相对路径，现在替换为绝对路径
LFLAG=-L /home/zby/bitrpc/build/release-install-cpp11/lib -lmuduo_net -lmuduo_base -pthread -ljsoncpp

all:server pclient sclient
server: server_topic.cpp
	g++ $(CFLAG) $^ -o $@  $(LFLAG)
pclient: publish_client.cpp
	g++ $(CFLAG) $^ -o $@  $(LFLAG)
sclient: subscribe_client.cpp
	g++ $(CFLAG) $^ -o $@  $(LFLAG)
.PHONY:clean
clean:
	rm -rf server pclient sclient