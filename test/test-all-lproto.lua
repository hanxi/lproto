local core = require "lproto.core"
local print_r = require "print_r"

local protocol_mod = {
    testtbl = {
        name = "default name",
        id = 0,
        testtblarray = {
            {
                number = "0",
                type = 1,
            }
        }
    },
    testarray = {
        iarray = {1},
        farray = {1.1},
        sarray = {""},
        aarray = {{0,}},
    }
}

local proto = core.newproto(protocol_mod, "testprot")
print("protocol struct:")
core.printstruct(proto)

local ab = {
    testtbl = {
        name = "Alice",
        id = 10000,
        testtblarray = {
            {
                number = "123456789",
                type = 1,
            },
            {
                number = "87654321",
                type = 2,
            },
        }
    },
    testarray = {
        iarray = {0,1,2,3,4,3,2,10,1},
        farray = {0.1,1.2345,2.2,3.2},
        sarray = {"hello","world"},
        aarray = {{0,1,2,2,2,2,2},{1,1,1,}},
    }
}

local code = core.encode(proto, ab)
print("buffer length:", string.len(code))

local abc = core.decode(proto, code)

local function hex_dump(buf)
  for byte=1, #buf, 16 do
     local chunk = buf:sub(byte, byte+15)
     io.write(string.format('%08X  ',byte-1))
     chunk:gsub('.', function (c) io.write(string.format('%02X ',string.byte(c))) end)
     io.write(string.rep(' ',3*(16-#chunk)))
     io.write(' ',chunk:gsub('%c','.'),"\n") 
  end
end
print("\nbuffer dump:")
hex_dump(code)

print("\ndecode result:")
print_r(abc)

core.deleteproto(proto)

