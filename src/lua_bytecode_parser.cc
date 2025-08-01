#include "lua_bytecode_parser.h"
#include <algorithm>
#include <stdexcept>

#define LUA_SIGNATURE_53 "\x1BLua"
#define LUAC_VERSION_53_MAJOR 0x5
#define LUAC_VERSION_53_MINOR 0x3
#define LUAC_FORMAT_53 0
#define LUAC_DATA_53 "\x19\x93\r\n\x1a\n"
#define LUAC_INT_53 0x5678
#define LUAC_NUM_53 (lua_Number)370.5

template <typename T> T bytes_to_value(const char *bytes) {
  T value;
  std::copy(bytes, bytes + sizeof(T), reinterpret_cast<char *>(&value));
  return value;
}

LuaBytecodeParser::LuaBytecodeParser(const std::vector<char> &bytecode_data)
    : data_(bytecode_data), offset_(0) {}

void LuaBytecodeParser::error(const std::string &msg) {
  throw std::runtime_error("Bytecode parsing error: " + msg);
}

std::uint8_t LuaBytecodeParser::read_byte() {
  if (offset_ + sizeof(std::uint8_t) > data_.size()) {
    error("truncated (byte)");
  }
  std::uint8_t value = bytes_to_value<std::uint8_t>(&data_[offset_]);
  offset_ += sizeof(std::uint8_t);
  return value;
}

std::uint32_t LuaBytecodeParser::read_int() {
  if (offset_ + sizeof(std::uint32_t) > data_.size()) {
    error("truncated (int)");
  }
  std::uint32_t value = bytes_to_value<std::uint32_t>(&data_[offset_]);
  offset_ += sizeof(std::uint32_t);
  return value;
}

lua_Number LuaBytecodeParser::read_number() {
  if (offset_ + sizeof(lua_Number) > data_.size()) {
    error("truncated (number)");
  }
  lua_Number value = bytes_to_value<lua_Number>(&data_[offset_]);
  offset_ += sizeof(lua_Number);
  return value;
}

lua_Integer LuaBytecodeParser::read_integer() {
  if (offset_ + sizeof(lua_Integer) > data_.size()) {
    error("truncated (integer)");
  }
  lua_Integer value = bytes_to_value<lua_Integer>(&data_[offset_]);
  offset_ += sizeof(lua_Integer);
  return value;
}

void LuaBytecodeParser::read_block(char *buffer, size_t size) {
  if (offset_ + size > data_.size()) {
    error("truncated (block)");
  }
  std::copy(data_.begin() + offset_, data_.begin() + offset_ + size, buffer);
  offset_ += size;
}

std::string LuaBytecodeParser::read_string() {
  size_t size = read_byte();
  if (size == 0xFF) {
    size = read_integer();
  }

  if (size == 0) {
    return "";
  } else {

    std::string s(size - 1, '\0');
    read_block(&s[0], size - 1);
    return s;
  }
}

void LuaBytecodeParser::check_literal(const std::string &expected,
                                      const std::string &error_msg) {
  std::string buffer(expected.length(), '\0');
  read_block(&buffer[0], expected.length());

  if (buffer != expected) {
    error(error_msg);
  }
}

void LuaBytecodeParser::check_size(size_t expected_size,
                                   const std::string &type_name) {
  std::uint8_t actual_size = read_byte();
  if (actual_size != expected_size) {
    error(type_name + " size mismatch");
  }
}

void LuaBytecodeParser::check_header() {

  check_literal(LUA_SIGNATURE_53 + 1,
                "not a Lua 5.3 bytecode file (magic mismatch)");

  std::uint8_t version_byte = read_byte();
  std::uint8_t major_version = (version_byte >> 4) & 0x0F;
  std::uint8_t minor_version = version_byte & 0x0F;
  if (major_version != LUAC_VERSION_53_MAJOR ||
      minor_version != LUAC_VERSION_53_MINOR) {
    error("version mismatch");
  }

  if (read_byte() != LUAC_FORMAT_53) {
    error("format mismatch");
  }

  check_literal(LUAC_DATA_53, "corrupted data section");

  check_size(sizeof(int), "int");
  check_size(sizeof(size_t), "size_t");
  check_size(sizeof(Instruction), "Instruction");
  check_size(sizeof(lua_Integer), "lua_Integer");
  check_size(sizeof(lua_Number), "lua_Number");

  if (read_integer() != LUAC_INT_53) {
    error("endianness mismatch");
  }
  if (read_number() != LUAC_NUM_53) {
    error("float format mismatch");
  }
}

void LuaBytecodeParser::load_code(LuaProto &f) {
  int n = read_int();
  f.code.resize(n);
  read_block(reinterpret_cast<char *>(f.code.data()), n * sizeof(Instruction));
}

void LuaBytecodeParser::load_constants(LuaProto &f) {
  std::uint32_t n = read_int();
  f.constants.resize(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    LuaType t = static_cast<LuaType>(read_byte());
    f.constants[i].type = t;
    switch (t) {
    case LUA_TNIL:
      break;
    case LUA_TBOOLEAN:
      f.constants[i].bval = (read_byte() != 0);
      break;
    case LUA_TNUMFLT:
      f.constants[i].nval = read_number();
      break;
    case LUA_TNUMINT:
      f.constants[i].ival = read_integer();
      break;
    case LUA_TSHRSTR:
    case LUA_TLNGSTR:
      new (&f.constants[i].sval) std::string(read_string());
      break;
    default:
      error("unknown constant type: " + std::to_string(static_cast<int>(t)));
    }
  }
}

void LuaBytecodeParser::load_upvalues(LuaProto &f) {
  int n = read_int();
  f.upvalues.resize(n);
  for (int i = 0; i < n; ++i) {
    f.upvalues[i].instack = read_byte();
    f.upvalues[i].idx = read_byte();
  }
}

void LuaBytecodeParser::load_protos(LuaProto &f) {
  int n = read_int();
  f.protos.resize(n);
  for (int i = 0; i < n; ++i) {
    f.protos[i] = std::make_shared<LuaProto>();
    load_function(*f.protos[i], f.source);
  }
}

void LuaBytecodeParser::load_debug(LuaProto &f) {
  std::uint32_t n_lineinfo = read_int();
  f.lineinfo.resize(n_lineinfo);
  read_block(reinterpret_cast<char *>(f.lineinfo.data()),
             n_lineinfo * sizeof(std::uint32_t));

  std::uint32_t n_locvars = read_int();
  f.locvars.resize(n_locvars);
  for (std::uint32_t i = 0; i < n_locvars; ++i) {
    f.locvars[i].varname = read_string();
    f.locvars[i].startpc = read_int();
    f.locvars[i].endpc = read_int();
  }

  std::uint32_t n_upvalue_names = read_int();
  f.upvalue_names.resize(n_upvalue_names);
  for (std::uint32_t i = 0; i < n_upvalue_names; ++i) {
    f.upvalue_names[i] = read_string();
  }
}

void LuaBytecodeParser::load_function(LuaProto &f, const std::string &psource) {
  f.source = read_string();
  if (f.source.empty()) {
    f.source = psource;
  }
  f.linedefined = read_int();
  f.lastlinedefined = read_int();
  f.numparams = read_byte();
  f.is_vararg = read_byte();
  f.maxstacksize = read_byte();

  load_code(f);
  load_constants(f);
  load_upvalues(f);
  load_protos(f);
  load_debug(f);
}

std::shared_ptr<LuaProto> LuaBytecodeParser::parse() {

  if (read_byte() != LUA_SIGNATURE_53[0]) {
    error("not a Lua 5.3 bytecode file (signature byte mismatch)");
  }

  check_header();

  std::uint8_t nupvalues_main = read_byte();

  std::shared_ptr<LuaProto> main_proto = std::make_shared<LuaProto>();
  load_function(*main_proto, "");

  if (nupvalues_main != main_proto->upvalues.size()) {
    error("main closure upvalue count mismatch with main prototype");
  }

  return main_proto;
}

LuaBytecodeFormatter::LuaBytecodeFormatter(std::ostream &os)
    : os_(os), indent_level_(0) {}

void LuaBytecodeFormatter::indent() {
  for (int i = 0; i < indent_level_; ++i) {
    os_ << "  ";
  }
}

void LuaBytecodeFormatter::format_instruction(const Instruction &instr,
                                              int pc) {

  indent();
  os_ << std::setw(4) << std::setfill('0') << pc << "  ";
  os_ << "0x" << std::hex << std::setw(8) << std::setfill('0') << instr
      << std::dec << std::endl;
}

void LuaBytecodeFormatter::format_constant(const LuaConstant &constant,
                                           int index) {
  indent();
  os_ << "  [" << index << "] ";
  switch (constant.type) {
  case LUA_TNIL:
    os_ << "NIL" << std::endl;
    break;
  case LUA_TBOOLEAN:
    os_ << "BOOLEAN " << (constant.bval ? "true" : "false") << std::endl;
    break;
  case LUA_TNUMFLT:
    os_ << "NUMBER (float) " << constant.nval << std::endl;
    break;
  case LUA_TNUMINT:
    os_ << "NUMBER (integer) " << constant.ival << std::endl;
    break;
  case LUA_TSHRSTR:
  case LUA_TLNGSTR:
    os_ << "STRING \"" << constant.sval << "\"" << std::endl;
    break;
  default:
    os_ << "UNKNOWN_TYPE (" << static_cast<int>(constant.type) << ")"
        << std::endl;
    break;
  }
}

void LuaBytecodeFormatter::format_upvaldesc(const LuaUpvaldesc &upval,
                                            int index) {
  indent();
  os_ << "  Upvalue [" << index << "]: ";
  os_ << "Instack=" << (int)upval.instack << ", Idx=" << (int)upval.idx;
  if (index < upval.name.length() && !upval.name.empty()) {
    os_ << ", Name=\"" << upval.name << "\"";
  }
  os_ << std::endl;
}

void LuaBytecodeFormatter::format_locvar(const LuaLocVar &locvar, int index) {
  indent();
  os_ << "  LocalVar [" << index << "]: ";
  os_ << "Name=\"" << locvar.varname << "\", StartPC=" << locvar.startpc
      << ", EndPC=" << locvar.endpc << std::endl;
}

void LuaBytecodeFormatter::format_proto(const LuaProto &proto) {
  indent();
  os_ << "Function Prototype:" << std::endl;
  indent_level_++;

  indent();
  os_ << "Source: \"" << proto.source << "\"" << std::endl;
  indent();
  os_ << "Line Defined: " << proto.linedefined << std::endl;
  indent();
  os_ << "Last Line Defined: " << proto.lastlinedefined << std::endl;
  indent();
  os_ << "Num Params: " << (int)proto.numparams << std::endl;
  indent();
  os_ << "Is Vararg: " << (int)proto.is_vararg << std::endl;
  indent();
  os_ << "Max Stack Size: " << (int)proto.maxstacksize << std::endl;

  indent();
  os_ << "Code (" << proto.code.size() << " instructions):" << std::endl;
  indent_level_++;
  for (size_t i = 0; i < proto.code.size(); ++i) {
    format_instruction(proto.code[i], i);
  }
  indent_level_--;

  indent();
  os_ << "Constants (" << proto.constants.size() << "):" << std::endl;
  indent_level_++;
  for (size_t i = 0; i < proto.constants.size(); ++i) {
    format_constant(proto.constants[i], i);
  }
  indent_level_--;

  indent();
  os_ << "Upvalues (" << proto.upvalues.size() << "):" << std::endl;
  indent_level_++;
  for (size_t i = 0; i < proto.upvalues.size(); ++i) {

    std::string upval_name =
        (i < proto.upvalue_names.size()) ? proto.upvalue_names[i] : "";
    LuaUpvaldesc temp_upval = proto.upvalues[i];
    temp_upval.name = upval_name;
    format_upvaldesc(temp_upval, i);
  }
  indent_level_--;

  indent();
  os_ << "Local Variables (" << proto.locvars.size() << "):" << std::endl;
  indent_level_++;
  for (size_t i = 0; i < proto.locvars.size(); ++i) {
    format_locvar(proto.locvars[i], i);
  }
  indent_level_--;

  indent();
  os_ << "Nested Prototypes (" << proto.protos.size() << "):" << std::endl;
  indent_level_++;
  for (size_t i = 0; i < proto.protos.size(); ++i) {
    format_proto(*proto.protos[i]);
  }
  indent_level_--;

  indent_level_--;
  indent();
  os_ << "End Function Prototype" << std::endl;
}

void LuaBytecodeFormatter::format(const LuaProto &proto) {
  format_proto(proto);
}

LuaBytecodeWriter::LuaBytecodeWriter(std::ostream &os) : os_(os) {}

void LuaBytecodeWriter::write_byte(std::uint8_t val) {
  os_.write(reinterpret_cast<const char *>(&val), sizeof(val));
}

void LuaBytecodeWriter::write_int(std::uint32_t val) {
  os_.write(reinterpret_cast<const char *>(&val), sizeof(val));
}

void LuaBytecodeWriter::write_number(lua_Number val) {
  os_.write(reinterpret_cast<const char *>(&val), sizeof(val));
}

void LuaBytecodeWriter::write_integer(lua_Integer val) {
  os_.write(reinterpret_cast<const char *>(&val), sizeof(val));
}

void LuaBytecodeWriter::write_block(const char *buffer, size_t size) {
  os_.write(buffer, size);
}

void LuaBytecodeWriter::write_string(const std::string &s) {
  if (s.empty()) {
    write_byte(0);
    return;
  }
  
  size_t size = s.length() + 1;
  if (size < 0xFF) {
    write_byte(static_cast<std::uint8_t>(size));
  } else {
    write_byte(0xFF);
    write_integer(size);
  }
  write_block(s.c_str(), s.length());
}

void LuaBytecodeWriter::write_header() {
  write_byte(LUA_SIGNATURE_53[0]);
  write_block(LUA_SIGNATURE_53 + 1, strlen(LUA_SIGNATURE_53 + 1));
  write_byte(LUAC_VERSION_53);
  write_byte(LUAC_FORMAT_53);
  write_block(LUAC_DATA_53, strlen(LUAC_DATA_53));

  write_byte(sizeof(int));
  write_byte(sizeof(size_t));
  write_byte(sizeof(Instruction));
  write_byte(sizeof(lua_Integer));
  write_byte(sizeof(lua_Number));

  write_integer(LUAC_INT_53);
  write_number(LUAC_NUM_53);
}

void LuaBytecodeWriter::write_code(const LuaProto &f) {
  write_int(f.code.size());
  write_block(reinterpret_cast<const char *>(f.code.data()),
              f.code.size() * sizeof(Instruction));
}

void LuaBytecodeWriter::write_constants(const LuaProto &f) {
  write_int(f.constants.size());
  for (const auto &constant : f.constants) {
    write_byte(static_cast<std::uint8_t>(constant.type));
    switch (constant.type) {
    case LUA_TNIL:
      break;
    case LUA_TBOOLEAN:
      write_byte(constant.bval ? 1 : 0);
      break;
    case LUA_TNUMFLT:
      write_number(constant.nval);
      break;
    case LUA_TNUMINT:
      write_integer(constant.ival);
      break;
    case LUA_TSHRSTR:
    case LUA_TLNGSTR:
      write_string(constant.sval);
      break;
    default:

      throw std::runtime_error("unknown constant type during write: " +
                               std::to_string(static_cast<int>(constant.type)));
    }
  }
}

void LuaBytecodeWriter::write_upvalues(const LuaProto &f) {
  write_int(f.upvalues.size());
  for (const auto &upval : f.upvalues) {
    write_byte(upval.instack);
    write_byte(upval.idx);
  }
}

void LuaBytecodeWriter::write_protos(const LuaProto &f) {
  write_int(f.protos.size());
  for (const auto &proto : f.protos) {
    write_function(*proto, f.source);
  }
}

void LuaBytecodeWriter::write_debug(const LuaProto &f) {
  write_int(f.lineinfo.size());
  write_block(reinterpret_cast<const char *>(f.lineinfo.data()),
              f.lineinfo.size() * sizeof(std::uint32_t));

  write_int(f.locvars.size());
  for (const auto &locvar : f.locvars) {
    write_string(locvar.varname);
    write_int(locvar.startpc);
    write_int(locvar.endpc);
  }

  write_int(f.upvalue_names.size());
  for (const auto &name : f.upvalue_names) {
    write_string(name);
  }
}

void LuaBytecodeWriter::write_function(const LuaProto &f,
                                       const std::string &psource) {

  if (f.source == psource || f.source.empty()) {
    write_string("");
  } else {
    write_string(f.source);
  }
  write_int(f.linedefined);
  write_int(f.lastlinedefined);
  write_byte(f.numparams);
  write_byte(f.is_vararg);
  write_byte(f.maxstacksize);

  write_code(f);
  write_constants(f);
  write_upvalues(f);
  write_protos(f);
  write_debug(f);
}

void LuaBytecodeWriter::write(const LuaProto &proto) {
  write_header();

  write_byte(proto.upvalues.size());
  write_function(proto, "");
}