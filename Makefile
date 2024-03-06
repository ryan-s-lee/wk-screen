all:
	g++ src/radixtree.cpp -O0 -ggdb

debug:
	g++ src/radixtree.cpp -O0 -ggdb -D DEBUG
