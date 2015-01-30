test: proto.c
	gcc -g -Wall -o $@ $^ -L. -llua -lm -ldl
