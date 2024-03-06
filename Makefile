all:
	g++ src/radixtree.cpp

debug:
	g++ src/radixtree.cpp -O0 -ggdb -D DEBUG

so:
	g++ -shared -o radixtree.so -fpic -Wall -Werror src/radixtree.cpp
