#include "buffer.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>

void buffer_test()
{
    struct buffer *buf = buffer_new();
    buffer_push_back_data(buf,"fuckyou",7);
    buffer_push_front_data(buf,"ddddddd",7);
    buffer_push_front_data(buf,"xxxxxxx",7);
    // xxxxxxxdddddddfuckyou
    buffer_merge(buf,1000);
    buffer_print(buf,"test_push");
    assert(buffer_get_length(buf)==21);

    char data[1024] = {0};
    int l = buffer_pop_front_data(buf,data,9);
    printf("popfront:%d:%s\n",l,data);
    assert(strcmp(data,"xxxxxxxdd")==0);

    l = buffer_remove_front(buf,4);
    printf("removefront:%d\n",l);
    assert(buffer_get_length(buf)==8);

    memset(data,0,1024);
    l = buffer_copy_front_data(buf,data,9);
    printf("copyfrontdata:%d:%s\n",l,data);
    assert(strcmp(data,"dfuckyou")==0);

    buffer_expand(buf,1204);
    buffer_merge(buf,10);
    l = buffer_copy_front_data(buf,data,100);
    printf("copyfrontdata:%d:%s\n",l,data);
    buffer_print(buf,"test_merge");
    assert(strcmp(data,"dfuckyou")==0);
    assert(buffer_get_length(buf)==8);

    buffer_expand(buf,1205);
    buffer_merge(buf,10);
    buffer_print(buf,"test_expand");

    struct buffer *newbuf = buffer_new();
    buffer_push_back_buffer(newbuf,buf);
    buffer_print(newbuf,"test_push_back_buffer:newbuf");
    buffer_print(buf,"test_push_back_buffer:buf");
    assert(buffer_get_length(newbuf)==8);
    assert(buffer_get_length(buf)==0);

    buffer_push_front_buffer(buf,newbuf);
    buffer_print(newbuf,"test_push_front_buffer:newbuf");
    buffer_print(buf,"test_push_front_buffer:buf");
    assert(buffer_get_length(newbuf)==0);
    assert(buffer_get_length(buf)==8);

    buffer_pop_front_buffer(buf, newbuf, 5);
    buffer_print(buf,"test_pop_front_buffer:buf");
    buffer_print(newbuf,"test_pop_front_buffer:newbuf");
    assert(buffer_get_length(buf)==3);
    assert(buffer_get_length(newbuf)==5);

    memset(data,0,1024);
    l = buffer_copy_front_data(buf,data,100);
    printf("copyfrontdata:%d:%s\n",l,data);
    assert(strcmp(data,"you")==0);

    // test read write
    struct buffer *buf2 = buffer_new();
    int fd = open("t.txt",O_APPEND|O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
    printf("fd=%d\n",fd);
    int n = buffer_read(buf2,fd,27);
    buffer_print(buf2,"test_read");
    printf("read n=%d\n",n);
    buffer_write(buf2,fd,20);
    close(fd);
    buffer_delete(&buf);
    buffer_delete(&newbuf);
    buffer_delete(&buf2);

    struct buffer *buf3 = buffer_new();
    int64_t a[] = {
        0x00,0x0F,0xF0,0xFF,
        0x0F00,0x0F0F,0x0FF0,0x0FFF,
        0xF000,0xF00F,0xF0F0,0xF0FF,
        0xFF00,0xFF0F,0xFFF0,0xFFFF,
        0xFF00FF,0xFF0FFF,0xFFF0,0xFFFFFF,
        0xFF00FFFF,0xFF0FFFFF,0xFFF0FF,0xFFFFFFFF,
        0xFF00FFFFFF,0xFF0FFFFFFF,0xFFFFF0FF,0xFFFFFFFFFF,
        0xFFFFF00FFFFFF,0xFFFFF0FFFFFFF,0xFF000FFF0FF,0xFFF000FFFFFFF,
        0xFFFFF00FFFFF00F,0xFFFFF000FFFFFFF,0xFFFF000FFF0FF,0xFF34F000FFFFFFF,
        0x7FFFF00FFFFF00F,0x7FFFFF000FFFFFFF,0x7FFFF000FFF0FF,0x7FF34F000FFFFFFF,
    };
    int i=0;
    for (i=0; i<sizeof(a)/sizeof(a[0]); i++) {
        int64_t v1 = a[i];
        buffer_push_back_integer(buf3,v1);
        int len = buffer_get_length(buf3);
        int64_t v2 = 0;
        buffer_pop_front_integer(buf3,&v2);
        printf("buf3:%d:%d:writeinteger:%" PRId64 ",%" PRIx64 ",readinteger:%" PRId64 ",%" PRIx64 "\n",i,len,v1,v1,v2,v2);
    }
    int64_t maxn = 0x7FFFFFFFFFFFFFFF;
    printf("max int64_t : %" PRId64 "\nmin int64_t : %" PRId64 "\n",maxn,maxn+1);

    int64_t v1 = 1;
    int64_t v2 = 2;
    buffer_push_back_integer(buf3,v1);
    buffer_push_back_integer(buf3,v2);
    buffer_dump(buf3,"buf3");
    buffer_pop_front_integer(buf3,&v1);
    buffer_pop_front_integer(buf3,&v2);
    printf("buffer_pop_front_integer:%" PRId64 ",%" PRId64 "\n",v1,v2);

    buffer_delete(&buf3);
}

int main(int argc, char *argv[])
{
    buffer_test();
    return 0;
}

