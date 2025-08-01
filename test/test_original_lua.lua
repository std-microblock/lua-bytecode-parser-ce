-- 变量与基本类型
local b = 3.14
local c = "Lua"
local d = true
local e = nil

-- 表（table）
local t = {1, 2, 3, key = "value"}

-- 函数（匿名函数、闭包）
local function adder(x)
    return function(y) return x + y end
end
local add5 = adder(5)
print("add5(10):", add5(10))

-- 控制结构
if a > 100000 then
    print("a is large")
elseif a > 1000 then
    print("a is medium")
else
    print("a is small")
end

for i = 1, 3 do
    print("for:", i)
end

local i = 1
while i <= 3 do
    print("while:", i)
    i = i + 1
end

repeat
    print("repeat:", i)
    i = i - 1
until i == 1

-- 多返回值
function swap(x, y)
    return y, x
end
local x, y = swap(1, 2)
print("swap:", x, y)

-- 可变参数
function sum(...)
    local s = 0
    for _, v in ipairs({...}) do
        s = s + v
    end
    return s
end
print("sum:", sum(1, 2, 3))

-- 元表与元方法
local mt = {
    __add = function(a, b)
        return {value = a.value + b.value}
    end
}
local obj1 = {value = 10}
local obj2 = {value = 20}
setmetatable(obj1, mt)
setmetatable(obj2, mt)
local obj3 = obj1 + obj2
print("metatable add:", obj3.value)

-- 协程
local co = coroutine.create(function()
    for i = 1, 2 do
        print("coroutine:", i)
        coroutine.yield()
    end
end)
coroutine.resume(co)
coroutine.resume(co)

-- 模块（require）
-- local math = require("math") -- 需要标准库支持

-- 错误处理
local status, err = pcall(function() error("error test") end)
print("pcall:", status, err)

-- 环境（_ENV）
_ENV.test_env = "env_value"
print("ENV:", test_env)

-- goto
local n = 0
::label::
n = n + 1
if n < 2 then
    goto label
end

-- do/end 块
do
    local temp = "scoped"
    print("do/end:", temp)
end

-- 运算符
local logic = (true and false) or not false
print("logic:", logic)

-- 字符串操作
print("string:", string.upper("lua"))

-- 数组操作
local arr = {10, 20, 30}
table.insert(arr, 40)
print("table:", table.concat(arr, ", "))

-- 迭代器
for k, v in pairs(t) do
    print("pairs:", k, v)
end
for i, v in ipairs(arr) do
    print("ipairs:", i, v)
end