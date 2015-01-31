test: proto.c buffer.c
	gcc -g -Wall -o $@ $^ -L. -llua -lm -ldl
