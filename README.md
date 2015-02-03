# Introduction

lproto is short names of Lua Protocol, It's a protocol pack library, the protocol definition by `Lua Table` syntax(some limits).

lproto 是 Lua Protocol的简写，是一个协议打包库，协议定义格式使用`Lua Table`格式（有所限制）。


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
* Only lua5.2
* It simple to support others systems and others lua versions.


## Compile and test

```
$ make
$ lua test/test-lproto.lua
```

## Others

* print_r come from <https://github.com/cloudwu/sproto/blob/master/print_r.lua>
* lproto.c api like this <https://github.com/cloudwu/sproto/blob/master/lsproto.c>
* So Thanks cloudwu's [sproto](https://github.com/cloudwu/sproto).