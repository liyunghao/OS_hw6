all:
	g++ 0616312.cpp -o 0616312.out `pkg-config fuse --cflags --libs`
