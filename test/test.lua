package.path="lualib/?.lua;"
local lproto = require "lproto"

local protDict = {}

local function registProt(p)
    table.insert(protDict,p)
    return #protDict
end

local dp = {
    user = "hanxi",
    money = 100,
    list = {
        a = 1,
        b = "hao",
    },
}
local test_prot_Id = registProt(dp)

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

local p = {
    protId = 1,
    user = "hanxi",
    money = 0xffff0fffff,
    list = {{a=1,b="h"},{a=2,b="s"}},
}
prot = lproto.initProt(protDict)
local sz,str = prot:pack(test_prot_Id,p)
local ret,pp = prot:unpack(str,sz)
print(ret,pp)
print(serialize(pp))

