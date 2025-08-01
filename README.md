# Lua Bytecode Parser CE

A versatile Lua 5.3 bytecode parser that supports both standard Lua bytecode and Cheat Engine modified format.

**Key Features:**
- ✅ Read both standard Lua bytecode and Cheat Engine format
- ✅ Write standard Lua bytecode format
- ✅ Clear analysis of bytecode structure
- ✅ Easy-to-use C++ library integration

## Quick Start

### Command Line Usage
```bash
parser.exe input.luac [output.luac]
```

**Example Output:**
```
> parser luac.out

Function Prototype:
  Source: "@test.lua"
  Line Defined: 0
  Last Line Defined: 0
  Num Params: 0
  Is Vararg: 1
  Max Stack Size: 2
  Code (2 instructions):
    0000  0x00000001
    0001  0x00800026
  Constants:
      [0] NUMBER (integer) 1
  Upvalues:
      [0] _ENV (Instack=1, Idx=0)
  Local Variables:
      [0] a (Scope: PC 1-2)
End Function Prototype
```

### Library Integration
```cpp
// Parse bytecode
LuaBytecodeParser parser(bytecode_data);
auto main_proto = parser.parse();

// Write bytecode
std::ofstream out_file("output.luac", std::ios::binary);
LuaBytecodeWriter writer(out_file);
writer.write(*main_proto);
```

---

# Lua 字节码解析器 CE

支持标准 Lua 5.3 字节码和 Cheat Engine 修改格式的解析工具

**核心功能:**
- ✅ 读取标准格式和 CE 修改格式
- ✅ 写入标准格式字节码
- ✅ 清晰的字节码结构分析
- ✅ 简单的 C++ 集成方案

## 快速开始

### 命令行使用
```bash
parser.exe 输入文件.luac [输出文件.luac]
```

**示例输出:**
```
> parser luac.out

函数原型:
  源文件: "@test.lua"
  起始行: 0
  结束行: 0
  参数数量: 0
  可变参数: 是
  最大栈大小: 2
  指令 (2条):
    0000  0x00000001
    0001  0x00800026
  常量:
      [0] 数值 (整型) 1
  上值:
      [0] _ENV (在栈中=1, 索引=0)
  局部变量:
      [0] a (作用域: PC 1-2)
结束函数原型
```

### 库集成
```cpp
// 解析字节码
LuaBytecodeParser parser(bytecode_data);
auto main_proto = parser.parse();

// 写入字节码
std::ofstream out_file("output.luac", std::ios::binary);
LuaBytecodeWriter writer(out_file);
writer.write(*main_proto);
```
