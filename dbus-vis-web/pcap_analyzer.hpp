#ifndef _PCAP_ANALYZER_HPP
#define _PCAP_ANALYZER_HPP

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

enum class MessageEndian {
  INVALID,
  LITTLE,
  BIG
};

constexpr const char* MessageEndianStr[] = {
  "INVALID", "LITTLE", "BIG"
};

struct RawMessage {
  MessageEndian endian;
  std::vector<uint8_t> header;
  std::vector<uint8_t> data;

  RawMessage(const uint8_t* buf, int len);
  void Print();
};

enum class MessageType {
  INVALID,
  METHOD_CALL,
  METHOD_RETURN,
  ERROR,
  SIGNAL
};

constexpr const char* MessageTypeStr[] = {
  "INVALID", "METHOD_CALL", "METHOD_RETURN", "ERROR", "SIGNAL"
};

enum class MessageFlags {
  NO_REPLY_EXPECTED = 0x01,
  NO_AUTO_START = 0x02,
  ALLOW_INTERACTIVE_AUTHORIZATION = 0x04
};

struct FixedHeader {
  MessageEndian endian;
  MessageType type;
  uint8_t flags;
  uint8_t version;
  uint32_t length;
  uint32_t cookie;

  FixedHeader(const RawMessage& raw);
  void Print();
};

struct object_path {
  std::string str;
};

using DBusVariant = std::variant<bool, uint8_t, uint16_t, int16_t,
  uint32_t, int32_t, uint64_t, int64_t, double, std::string,
  object_path>;

enum class DBusDataType {
  INVALID = 0x0,
  BYTE = 'y',
  BOOLEAN = 'b',
  INT16 = 'n',
  UINT16 = 'q',
  INT32 = 'i',
  UINT32 = 'u',
  INT64 = 'x',
  UINT64 = 't',
  DOUBLE = 'd',
  STRING = 's',
  OBJECT_PATH = 'o',
  SIGNATURE = 'g',
  ARRAY = 'a',
  STRUCT = 'r',
  VARIANT = 'v',
  DICT_ENTRY = 'e',
  UNIX_FD = 'h',
};

bool IsFixedType(DBusDataType type) {
  switch (type) {
    case DBusDataType::BYTE:
    case DBusDataType::BOOLEAN:
    case DBusDataType::INT16:
    case DBusDataType::UINT16:
    case DBusDataType::INT32:
    case DBusDataType::UINT32:
    case DBusDataType::INT64:
    case DBusDataType::UINT64:
    case DBusDataType::DOUBLE:
    case DBusDataType::UNIX_FD:
      return true;
    default:
      return false;
  }
}

bool IsStringType(DBusDataType type) {
  switch (type) {
    case DBusDataType::STRING:
    case DBusDataType::OBJECT_PATH:
    case DBusDataType::SIGNATURE:
      return true;
    default:
      return false;
  }
}

bool IsContainerType(DBusDataType type) {
  switch (type) {
    case DBusDataType::ARRAY:
    case DBusDataType::STRUCT:
    case DBusDataType::VARIANT:
    case DBusDataType::DICT_ENTRY:
      return true;
    default:
      return false;
  }
}

struct TypeContainer {
  DBusDataType type;
  std::vector<TypeContainer> members;

  TypeContainer() : type(DBusDataType::INVALID) {}
  TypeContainer(DBusDataType _ty) : type(_ty) {}

  void do_Print(std::ostream& o) const {
    char buf[100];
    switch (type) {
      case DBusDataType::STRUCT:
        o << "(";
        break;
      default:
        sprintf(buf, "%c", int(type));
        o << buf;
        break;
    }

    for (const auto& m : members) {
      m.do_Print(o);
    }

    switch (type) {
      case DBusDataType::STRUCT:
        o << ")";
        break;
    }
  }

  void Print() const {
    do_Print(std::cout);
  }

  std::string ToString() const {
    std::stringstream ss;
    do_Print(ss);
    return ss.str();
  }
};

// The variable part in a header, with "a(yv)" type
struct HeaderFields {
  std::vector<std::pair<uint8_t, DBusVariant>> values;
};

struct DBusMessage {
  MessageType type;
  std::string sender, destination;
};

struct AlignedStream {
  std::vector<uint8_t>* buf;
  int offset = 0;

  uint8_t TakeOneByte() {
    return buf->at(offset++);
  }

  std::vector<uint8_t> Take(const TypeContainer& tc) {
    std::vector<uint8_t> ret;
    switch (tc.type) {
      case DBusDataType::BYTE:
      case DBusDataType::SIGNATURE:
        ret.push_back(buf->at(offset));
        offset += 1;
        break;
      case DBusDataType::UINT32:
        ret.push_back(buf->at(offset));
        ret.push_back(buf->at(offset+1));
        ret.push_back(buf->at(offset+2));
        ret.push_back(buf->at(offset+3));
        offset += 4;
        break;
      default: {
        printf("Type %s not implemented\n", tc.ToString().c_str());
        assert(0);
      }
    }
    return ret;
  }

  void Align(const TypeContainer& tc) {
    int alignment = 1;
    switch (tc.type) {
      case DBusDataType::STRUCT: alignment = 8; break;
      case DBusDataType::ARRAY:  alignment = 4; break;
      default: {
        assert(0);
        break;
      }
    }
    while ((offset % alignment) != 0) {
      offset ++;
    }
  }
};

// Parsing
void ParseType(MessageEndian endian, AlignedStream* stream, const TypeContainer& tc);

#endif