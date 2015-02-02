all : test-buffer test-proto lproto.so

test-buffer : test/test-buffer.c buffer.c
	gcc -g -Wall -o $@ $^

test-proto : test/test-proto.c proto.c buffer.c
	gcc -g -Wall -o $@ $^ -L. -llua -lm -ldl

lproto.so : lproto.c proto.c buffer.c
	gcc -O2 -Wall -shared -fPIC -o $@ $^ 

clean :
	rm test-buffer test-proto lproto.so


