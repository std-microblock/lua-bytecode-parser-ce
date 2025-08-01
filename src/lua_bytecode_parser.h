#ifndef LUA_BYTECODE_PARSER_H
#define LUA_BYTECODE_PARSER_H

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define LUAC_VERSION_53 0x53
#define LUAC_FORMAT_53 0
#define LUAC_DATA_53 "\x19\x93\r\n\x1a\n"
#define LUAC_INT_53 0x5678
#define LUAC_NUM_53 (lua_Number)370.5

typedef std::uint32_t Instruction;

typedef double lua_Number;
typedef std::int64_t lua_Integer;

enum LuaType {
  LUA_TNIL = 0,
  LUA_TBOOLEAN = 1,
  LUA_TNUMFLT = 3,
  LUA_TNUMINT = 0x13,
  LUA_TSHRSTR = 4,
  LUA_TLNGSTR = 0x14
};

struct LuaConstant {
  LuaType type;
  union {
    bool bval;
    lua_Number nval;
    lua_Integer ival;
    std::string sval;
  };

  LuaConstant() : type(LUA_TNIL) {}

  ~LuaConstant() {
    if (type == LUA_TSHRSTR || type == LUA_TLNGSTR) {
      sval.~basic_string();
    }
  }

  LuaConstant(const LuaConstant &other) : type(other.type) {
    switch (type) {
    case LUA_TBOOLEAN:
      bval = other.bval;
      break;
    case LUA_TNUMFLT:
      nval = other.nval;
      break;
    case LUA_TNUMINT:
      ival = other.ival;
      break;
    case LUA_TSHRSTR:
    case LUA_TLNGSTR:
      new (&sval) std::string(other.sval);
      break;
    default:
      break;
    }
  }

  LuaConstant(LuaConstant &&other) noexcept : type(other.type) {
    switch (type) {
    case LUA_TBOOLEAN:
      bval = other.bval;
      break;
    case LUA_TNUMFLT:
      nval = other.nval;
      break;
    case LUA_TNUMINT:
      ival = other.ival;
      break;
    case LUA_TSHRSTR:
    case LUA_TLNGSTR:
      new (&sval) std::string(std::move(other.sval));
      break;
    default:
      break;
    }
    other.type = LUA_TNIL;
  }

  LuaConstant &operator=(const LuaConstant &other) {
    if (this != &other) {

      if (type == LUA_TSHRSTR || type == LUA_TLNGSTR) {
        sval.~basic_string();
      }
      type = other.type;
      switch (type) {
      case LUA_TBOOLEAN:
        bval = other.bval;
        break;
      case LUA_TNUMFLT:
        nval = other.nval;
        break;
      case LUA_TNUMINT:
        ival = other.ival;
        break;
      case LUA_TSHRSTR:
      case LUA_TLNGSTR:
        new (&sval) std::string(other.sval);
        break;
      default:
        break;
      }
    }
    return *this;
  }

  LuaConstant &operator=(LuaConstant &&other) noexcept {
    if (this != &other) {

      if (type == LUA_TSHRSTR || type == LUA_TLNGSTR) {
        sval.~basic_string();
      }
      type = other.type;
      switch (type) {
      case LUA_TBOOLEAN:
        bval = other.bval;
        break;
      case LUA_TNUMFLT:
        nval = other.nval;
        break;
      case LUA_TNUMINT:
        ival = other.ival;
        break;
      case LUA_TSHRSTR:
      case LUA_TLNGSTR:
        new (&sval) std::string(std::move(other.sval));
        break;
      default:
        break;
      }
      other.type = LUA_TNIL;
    }
    return *this;
  }
};

struct LuaUpvaldesc {
  std::string name;
  std::uint8_t instack;
  std::uint8_t idx;
};

struct LuaLocVar {
  std::string varname;
  std::uint32_t startpc;
  std::uint32_t endpc;
};

struct LuaProto {
  std::string source;
  std::uint32_t linedefined;
  std::uint32_t lastlinedefined;
  std::uint8_t numparams;
  std::uint8_t is_vararg;
  std::uint8_t maxstacksize;

  std::vector<Instruction> code;
  std::vector<LuaConstant> constants;
  std::vector<LuaUpvaldesc> upvalues;
  std::vector<std::shared_ptr<LuaProto>> protos;

  std::vector<std::uint32_t> lineinfo;
  std::vector<LuaLocVar> locvars;
  std::vector<std::string> upvalue_names;

  LuaProto()
      : linedefined(0), lastlinedefined(0), numparams(0), is_vararg(0),
        maxstacksize(0) {}
};

class LuaBytecodeParser {
public:
  LuaBytecodeParser(const std::vector<char> &bytecode_data);
  std::shared_ptr<LuaProto> parse();

private:
  const std::vector<char> &data_;
  size_t offset_;
  std::int64_t encrypt_key_;
  bool is_ce_bytecode_;

  void check_literal(const std::string &expected, const std::string &error_msg);
  void check_size(size_t expected_size, const std::string &type_name);
  void check_header();

  inline std::uint8_t read_byte() { return read<std::uint8_t>(); }
  inline std::uint32_t read_uint32() { return read<std::uint32_t>(); }
  inline lua_Integer read_uint64() { return read<std::uint64_t>(); }
  inline lua_Number read_double() { return read<lua_Number>(); }
  std::string read_string();
  template <typename T> T read() {
    T value;
    read_block(reinterpret_cast<char *>(&value), sizeof(T));
    return value;
  }
  void read_block(char *buffer, size_t size);

  void load_code(LuaProto &f);
  void load_constants(LuaProto &f);
  void load_upvalues(LuaProto &f);
  void load_protos(LuaProto &f);
  void load_debug(LuaProto &f);
  void load_function(LuaProto &f, const std::string &psource);

  void error(const std::string &msg);
};

class LuaBytecodeFormatter {
public:
  LuaBytecodeFormatter(std::ostream &os);
  void format(const LuaProto &proto);

private:
  std::ostream &os_;
  int indent_level_;

  void indent();
  void format_instruction(const Instruction &instr, int pc);
  void format_constant(const LuaConstant &constant, int index);
  void format_upvaldesc(const LuaUpvaldesc &upval, int index);
  void format_locvar(const LuaLocVar &locvar, int index);
  void format_proto(const LuaProto &proto);
};

class LuaBytecodeWriter {
public:
  LuaBytecodeWriter(std::ostream &os);
  void write(const LuaProto &proto);

private:
  std::ostream &os_;

  void write_byte(std::uint8_t val);
  void write_int(std::uint32_t val);
  void write_number(lua_Number val);
  void write_integer(lua_Integer val);
  void write_string(const std::string &s);
  void write_block(const char *buffer, size_t size);

  void write_header();
  void write_code(const LuaProto &f);
  void write_constants(const LuaProto &f);
  void write_upvalues(const LuaProto &f);
  void write_protos(const LuaProto &f);
  void write_debug(const LuaProto &f);
  void write_function(const LuaProto &f, const std::string &psource);
};

#endif