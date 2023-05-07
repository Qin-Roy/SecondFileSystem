#crossover，10043602，Operating System Project 1
#tutor：邓蓉
myshell:main.cpp
	g++ -o filesystem TestAPI.cpp DiskDriver.cpp BufferManager.cpp main.cpp File.cpp FileSystem.cpp INode.cpp Kernel.cpp OpenFileManager.cpp -std=c++11

clean:
	rm filesystem
