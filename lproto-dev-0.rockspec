#!/usr/bin/env lua

package = 'lproto'
version = 'dev-0'
source = {
    url = 'git://github.com/hanxi/lproto.git'
}
description = {
    summary = 'Lua protocol with Lua Table',
    detailed = '',
    homepage = 'https://github.com/hanxi/lproto',
    license = 'MIT',
    maintainer = 'hanxi',
}
dependencies = {
    'lua >= 5.1',
}
build = {
    type = "builtin",
    modules = {
        lproto = {
            sources = {"proto.c","lproto.c","ldef.c"},
        }
    }
}
