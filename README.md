<<<<<<< HEAD
lproto
======

lua protocal : simple protocal by lua table .

blog:<http://oldblog.hanxi.info/2014/08/06/original-lua-protocal/>  (chinese)

new lproto is here <https://github.com/hanxi/blog/issues/2>

#### compile and run in linux


    $ make
    $ lua test/test.lua

=======
# Introduction

lproto is short names of Lua Protocol, It's a protocol pack library, the protocol definition by `Lua Table` syntax(some limits).

lproto 是 Lua Protocol的简写，是一个协议打包库，协议定义格式使用`Lua Table`格式（有所限制）。

Source in here:

<http://git.oschina.net/hanxi/lproto>

<https://github.com/hanxi/lproto>


# Support Data Types

* table (the key must be string)
* integer (support 64 bits)
* float (pack using string)
* string
* array (the element must the same type)


## Test data

* CPU : i5-5620 @2.4GHz
* OS : CentOS 6.3
* encode 1M times : 1.67 s
* decode 1M times : 3.06 s
* size : 64 bytes


## Support system

* Only linux now
* support lua5.1 lua5.2.3 lua5.3.0
* I think it's simple to support others systems, you can try it.


## Compile and test

```
$ make
$ lua test/test-all-lproto.lua
```

## Others

* print_r come from <https://github.com/cloudwu/sproto/blob/master/print_r.lua>
* lproto.c api like this <https://github.com/cloudwu/sproto/blob/master/lsproto.c>
* So Thanks cloudwu's [sproto](https://github.com/cloudwu/sproto).
>>>>>>> github-dev
