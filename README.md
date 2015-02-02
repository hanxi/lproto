#介绍

`lproto` 是一个协议打包库，协议定义格式使用Lua Table格式（有所限制），使用C+Lua实现。

# 协议支持数据类型

* table (the key must be string)
* integer (support 64 bits)
* float (pack using string)
* string
* array (the element must the same type)


## Test data

* CPU : i5-5620 @2.4GHz
* OS : CentOS 6.3
* encode 1M times : 3.33 s
* decode 1M times : 4.4 s
* size : 64 bytes


## Support system

* Only linux now
* Only lua5.2
* It simple to support others systems and others lua versions.