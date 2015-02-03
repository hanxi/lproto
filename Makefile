all : test-proto lproto.so

test-proto : test/test-proto.c proto.c 
	gcc -g -Wall -o $@ $^ -L. -llua -lm -ldl

lproto.so : lproto.c proto.c 
	gcc -O2 -Wall -shared -fPIC -o $@ $^ 

clean :
	rm test-proto lproto.so


