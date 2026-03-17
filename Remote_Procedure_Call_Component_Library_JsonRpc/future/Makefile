all:package_task async
async: async.cpp
	g++  $^ -o $@  -std=c++11 -lpthread
package_task: package_task.cpp
	g++  $^ -o $@  -std=c++11 -lpthread
.PHONY:clean
clean:
	rm -rf async package_task