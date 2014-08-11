package.path="lualib/?.lua;"
local lproto = require "lproto"

local protDict = {}

local function registprot(p)
    table.insert(protDict,p)
    return #protDict
end

--[[
-- define protocol dict
--]]
local dp = {
    user = "hanxi",
    money = 100,
    list = { -- it mean's array. the element is the table content.
        a = 1,
        b = "hao",
    },
}
local test_prot_Id = registprot(dp)
prot = lproto.initprot(protDict)

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

--[[
-- the protocol will bu pack
--]]
local p = {
    protId = 1,
    user = "hanxi",
    money = 0xffff0fffff,
    list = {{a=1,b="h"},{a=2,b="s"}},
}
local fd = 1
print("pack protId=",test_prot_Id)
local sz,str = prot:pack(fd,test_prot_Id,p)
print("return : ",sz,str)
local ret,fd,protId,pp = prot:unpack(str,sz)
print("upacke protId=",protId)
print("return : ",ret,fd,protId,pp)
print(serialize(pp))

