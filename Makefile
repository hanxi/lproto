all : test-proto lproto.so

CC= gcc -std=gnu99

test-proto : test/test-proto.c proto.c 
	$(CC) -g -Wall -o $@ $^ -L. -llua -lm -ldl

lproto.so : lproto.c proto.c 
	$(CC) -O2 -Wall -shared -fPIC -o $@ $^ 

clean :
	rm test-proto lproto.so


