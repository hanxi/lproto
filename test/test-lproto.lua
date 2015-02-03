local core = require "lproto.core"
local print_r = require "print_r"

local AddressBook = {
    person = {
        {
            name = "default name",
            id = 0,
            phone = {
                {
                    number = "0",
                    type = 1,
                }
            }
        }
    }
}

local proto = core.newproto(AddressBook, "AddressBook")
print("protocal struct:")
core.printstruct(proto)

local ab = {
    person = {
        {
            name = "Alice",
            id = 10000,
            phone = {
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
        {
            name = "Bob",
            id = 20000,
            phone = {
                {
                    number = "01234567890",
                    type = 3,
                }
            }
        }

    }
}

local times = ...

times = assert(tonumber(times))
print("\ntimes:",times)

local code
local t1 = os.clock()
for i=1,times do
    code = core.encode(proto, ab)
end
local t2 = os.clock()
print("encode_time:",t2-t1)
print("buffer length:", string.len(code))

local abc = nil
local t3 = os.clock()
for i=1,times do
    abc = core.decode(proto, code)
end
local t4 = os.clock()
print("decode_time:",t4-t3)

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

