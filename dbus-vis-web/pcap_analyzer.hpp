#ifndef _PCAP_ANALYZER_HPP
#define _PCAP_ANALYZER_HPP

#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
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
  std::vector<uint8_t> data;  // All data, including header
  std::vector<uint8_t> GetHeader() const { // The first 12 bytes of data
    std::vector<uint8_t> ret;
    if (data.size() >= 12) {
      for (int i=0; i<12; i++) ret.push_back(data[i]);
    }
    return ret;
  }

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

enum class MessageHeaderType {
  INVALID = 0,
  PATH = 1,
  INTERFACE = 2,
  MEMBER = 3,
  ERROR_NAME = 4,
  REPLY_SERIAL = 5,
  DESTINATION = 6,
  SENDER = 7,
  SIGNATURE = 8,
  UNIX_FDS = 9,
};

struct FixedHeader {
  MessageEndian endian;
  MessageType type;
  uint8_t flags;
  uint8_t version;
  uint32_t length;
  uint32_t cookie;

  FixedHeader() {}

  FixedHeader(const RawMessage& raw);
  void Print();
};

struct object_path {
  std::string str;
};

using DBusVariant = std::variant<bool, uint8_t, uint16_t, int16_t,
  uint32_t, int32_t, uint64_t, int64_t, double, std::string,
  object_path>;

using DBusMessageFields = std::map<MessageHeaderType, std::variant<object_path, std::string, uint32_t>>;

struct DBusContainer {
  std::vector<std::variant<DBusVariant, DBusContainer>> values;
};

using DBusType = std::variant<DBusVariant, DBusContainer>;

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

struct TypeContainer {
  DBusDataType type;
  DBusVariant value; // for fixed values

  std::vector<TypeContainer> members;

  TypeContainer() : type(DBusDataType::INVALID) {}
  TypeContainer(DBusDataType _ty) : type(_ty) {}

  void do_Print(std::ostream& o) const {
    char buf[100];
    switch (type) {
      case DBusDataType::STRUCT:
        o << "(";
        break;
      case DBusDataType::DICT_ENTRY:
        o << "{";
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
      case DBusDataType::DICT_ENTRY:
        o << "}";
        break;
      default: break;
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
  std::string body_signature;
  std::vector<TypeContainer> body;
};

struct AlignedStream {
  std::vector<uint8_t>* buf;
  int offset = 0;

  uint8_t TakeOneByte() {
    return buf->at(offset++);
  }

  std::vector<uint8_t> Take(const TypeContainer& tc) {
    if (Ended()) {
      throw std::out_of_range("AlignedStream has ended");
    }
    std::vector<uint8_t> ret;
    switch (tc.type) {
      case DBusDataType::BYTE:
      case DBusDataType::SIGNATURE:
      case DBusDataType::BOOLEAN:
        ret.push_back(buf->at(offset));
        offset += 1;
        break;
      case DBusDataType::INT16:
      case DBusDataType::UINT16:
        ret.push_back(buf->at(offset));
        ret.push_back(buf->at(offset+1));
        offset += 2;
        break;
      case DBusDataType::INT32:
      case DBusDataType::UINT32:
        ret.push_back(buf->at(offset));
        ret.push_back(buf->at(offset+1));
        ret.push_back(buf->at(offset+2));
        ret.push_back(buf->at(offset+3));
        offset += 4;
        break;
      case DBusDataType::INT64:
      case DBusDataType::UINT64:
      case DBusDataType::DOUBLE:
        for (int i=0; i<8; i++) {
          ret.push_back(buf->at(offset + i));
        }
        offset += 8;
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
      case DBusDataType::BYTE:
      case DBusDataType::BOOLEAN:
        alignment = 1; break;
      case DBusDataType::STRUCT:
      case DBusDataType::DICT_ENTRY:
        alignment = 8; break;
      case DBusDataType::ARRAY:
      case DBusDataType::STRING:
      case DBusDataType::INT32:
      case DBusDataType::UINT32:
        alignment = 4; break;
      case DBusDataType::UINT16:
        alignment = 2; break;
      case DBusDataType::INT64:
      case DBusDataType::UINT64:
      case DBusDataType::DOUBLE:
        alignment = 8; break;
      default: {
        printf("DBusDataType %c alignment not supported.\n", int(tc.type));
        assert(0);
        break;
      }
    }
    while ((offset % alignment) != 0) {
      offset ++;
    }
  }

  bool Ended() {
    return (offset >= buf->size());
  }
};

// Parsing
DBusVariant ParseFixed(MessageEndian endian, AlignedStream* stream, const TypeContainer& tc);
DBusType ParseType(MessageEndian endian, AlignedStream* stream, const TypeContainer& tc);
TypeContainer ParseOneSignature(const std::string& sig, int* out_idx=nullptr);
std::vector<TypeContainer> ParseSignatures(const std::string& sig);
void ProcessByteArray(const std::vector<uint8_t>& buf);
DBusType ParseContainer(MessageEndian endian, AlignedStream* stream, const TypeContainer& sig);
MessageHeaderType ToMessageHeaderType(int x);
void PrintDBusType(const DBusType& x);
std::string DBusTypeToJSON(const DBusType& x);

#endif