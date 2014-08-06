package.cpath="luaclib/?.so;"
local prot = require "luaprot"

local p = {
    protId = 1,
    user = "hanxi",
    money = 100,
    list = {{a=1,b="h"},{a=2,b="s"}},
}

ProtDict = {
    [1] = {
        user = "hanxi",
        money = 100,
        list = {
            a = 1,
            b = "hao",
            _keysort = {"a","b"},
        },
        _keysort = {"user","money","list"},
    }
}

-- 序列化
function serialize(obj,n)
    if n==nil then n=1 end
    local lua = ""
    local t = type(obj)
    if t == "number" then
        lua = lua .. obj
    elseif t == "boolean" then
        lua = lua .. tostring(obj)
    elseif t == "string" then
        lua = lua .. string.format("%q", obj)
    elseif t == "table" then
        lua = lua .. "{\n"
        for k, v in pairs(obj) do
            lua = lua .. string.rep("\t",n) .. "[" .. serialize(k) .. "]=" .. serialize(v,n+1) .. ",\n"
        end
        local metatable = getmetatable(obj)
        if metatable ~= nil and type(metatable.__index) == "table" then
            for k, v in pairs(metatable.__index) do
                lua = lua .. string.rep("\t",n) .."[" .. serialize(k) .. "]=" .. serialize(v,n+1) .. ",\n"
            end
        end
        lua = lua .. string.rep("\t",n-1) .. "}"
    elseif t == "nil" then
        return nil
    else
        error("can not serialize a " .. t .. " type.")
    end
    return lua
end


BUFFER_MAX_LEN = 10240
-- return : sz,buffer
function pack(p)
    prot.packinit()
    local protId = 1
    for _,key in ipairs(ProtDict[protId]._keysort) do
        local tp = type(ProtDict[protId][key])
        if tp=="table" then
            local ret = prot.write(#p[key])
            if ret>=BUFFER_MAX_LEN then
                return -1
            end
            for _,v in ipairs(p[key]) do
                for _,k in ipairs(ProtDict[protId][key]._keysort) do
                    print(key,p[key],v,k,v[k])
                    local ret = prot.write(v[k])
                    if ret>=BUFFER_MAX_LEN then
                        return -1
                    end
                end
            end
        else
            local ret = prot.write(p[key])
            if ret>=BUFFER_MAX_LEN then
                return -1
            end
        end
    end
    return prot.getpack()
end


function unpack(sz,buffer)
    if sz>=BUFFER_MAX_LEN then
        return -1
    end
    prot.unpackinit(sz,buffer)
    local p = {}
    local protId = 1
    for _,key in ipairs(ProtDict[protId]._keysort) do
        local tp = type(ProtDict[protId][key])
        if tp=="table" then
            p[key] = {}
            local ret,n = prot.read("number")
            --print("count:",ret,n)
            if ret<0 then
                return -1
            end
            for i=1,n do
                local temp = {}
                for _,k in ipairs(ProtDict[protId][key]._keysort) do
                    local ttp = type(ProtDict[protId][key][k])
                    local ret,v = prot.read(ttp)
                    print(ttp,ret,v)
                    if ret<0 then
                        return -2
                    end
                    temp[k] = v 
                end
                table.insert(p[key],temp)
            end
        else
            local ret,v = prot.read(tp)
            print(tp,ret,v)
            if ret<0 then
                return -3
            end
            p[key] = v
        end
    end
    return 0,p
end

print(serialize(p))
local sz,str = pack(p)
local ret,pp = unpack(sz,str)
print(ret,pp)
print(serialize(pp))

