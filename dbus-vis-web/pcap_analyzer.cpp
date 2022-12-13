// How to build and run
//
// 1. CXX version
//   make pcap_analyzer
//   ./pcap_analyzer < evb_ast2500.pcap
//
// 2. Build Emscripten version
//   make pcap_analyzer.js
//   node pcap_analyzer.js < evb_ast2500.pcap

//
// Parsing logic based on dbus-pcap
//

#include <cassert>
#include <cstring>
#include <stdint.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#ifdef DBUS_PCAP_USING_EMSCRIPTEN
#include <emscripten.h>
#endif

#include <pcap/pcap.h>

#include "pcap_analyzer.hpp"

int g_num_packets = 0;

void PrintByteArray(const std::vector<uint8_t>&, const int);

#ifdef DBUS_PCAP_USING_EMSCRIPTEN

extern "C" {

int g_pcap_buf_size = 0;
std::vector<uint8_t> g_pcap_buf;
void StartSendPCAPByteArray(int buf_size) {
	g_pcap_buf_size = buf_size;
}

void SendPCAPByteArrayChunk(char* buf, int len) {
	for (int i=0; i<len; i++) {
		g_pcap_buf.push_back(buf[i]);
	}
}

void EndSendPCAPByteArray() {
	ProcessByteArray(g_pcap_buf);
	g_pcap_buf.clear();
	EM_ASM_ARGS({
		OnFinishParsingDBusPcap();
	});
}

}

#endif // EMSCRIPTEN_VERSION

MessageEndian MessageEndianLookup(uint8_t ch) {
	switch (ch) {
		case 'l': return MessageEndian::LITTLE;
		case 'B': return MessageEndian::BIG;
		default: return MessageEndian::INVALID;
	}
}

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

MessageHeaderType ToMessageHeaderType(int x) {
  switch (x) {
    case 1: return MessageHeaderType::PATH;
    case 2: return MessageHeaderType::INTERFACE;
    case 3: return MessageHeaderType::MEMBER;
    case 4: return MessageHeaderType::ERROR_NAME;
    case 5: return MessageHeaderType::REPLY_SERIAL;
    case 6: return MessageHeaderType::DESTINATION;
    case 7: return MessageHeaderType::SENDER;
    case 8: return MessageHeaderType::SIGNATURE;
    case 9: return MessageHeaderType::UNIX_FDS;
    default: return MessageHeaderType::INVALID;
  }
}

std::string MessageHeaderTypeName(MessageHeaderType x) {
	switch (x) {
		case MessageHeaderType::INVALID: return "INVALID";
		case MessageHeaderType::PATH: return "PATH";
		case MessageHeaderType::INTERFACE: return "INTERFACE";
		case MessageHeaderType::MEMBER: return "MEMBER";
		case MessageHeaderType::ERROR_NAME: return "ERROR_NAME";
		case MessageHeaderType::REPLY_SERIAL: return "REPLY_SERIAL";
		case MessageHeaderType::DESTINATION: return "DESTINATION";
		case MessageHeaderType::SENDER: return "SENDER";
		case MessageHeaderType::SIGNATURE: return "SIGNATURE";
		case MessageHeaderType::UNIX_FDS: return "UNIX_FDS";
		default: return "UNKNOWN";
	}
}

template<class T>
std::string VariantToString(T x) {
	if (std::holds_alternative<std::string>(x)) {
		return std::get<std::string>(x);
	} else if (std::holds_alternative<object_path>(x)) {
		return std::get<object_path>(x).str;
	} else if (std::holds_alternative<uint32_t>(x)) {
		return std::to_string(std::get<uint32_t>(x));
	} else {
		assert(0);
	}
}

// Library funcs
RawMessage::RawMessage(const uint8_t* buf, int len) {
	if (len < 12) {
		assert(false && "Must be at least 12 bytes long");
	}
	endian = MessageEndianLookup(buf[0]);
	for (int i=0; i<len; i++) { data.push_back(buf[i]); }
}

void RawMessage::Print() {
	printf("[RawMessage] Endian:%s\n", MessageEndianStr[int(endian)]);
}

template<typename T>
T BytesToTypeLE(const uint8_t* data) {
	T ret;
	uint8_t* ptr = reinterpret_cast<uint8_t*>(&ret);
	for (int i=0; i<sizeof(T); i++) {
		ptr[i] = data[i];
	}
	return ret;
}

template<typename T>
T BytesToTypeBE(const uint8_t* data) {
	T ret;
	uint8_t* ptr = reinterpret_cast<uint8_t*>(&ret);
	for (int i=0; i<sizeof(T); i++) {
		ptr[i] = data[sizeof(T)-1-i];
	}
	return ret;
}

// Header format: yyyyuua(yv)
// The "yyyyuu" part is Fixed.
FixedHeader::FixedHeader(const RawMessage& raw) {
	std::vector<uint8_t> header = raw.GetHeader();
	endian  = MessageEndianLookup(header[0]);
	type    = MessageType(header[1]);
	flags   = header[2];
	version = header[3];
	if (raw.endian == MessageEndian::LITTLE) {
		length = BytesToTypeLE<uint32_t>(header.data()+4);
		cookie = BytesToTypeLE<uint32_t>(header.data()+8);
	} else {
		length = BytesToTypeBE<uint32_t>(header.data()+4);
		cookie = BytesToTypeBE<uint32_t>(header.data()+8);
	}
}

void FixedHeader::Print() {
	printf("[FixedHeader] type:%s, length:%u, cookie:%u\n",
		MessageTypeStr[int(type)],
		length,
		cookie);
}

std::optional<TypeContainer> do_ParseSignature(const std::string& sig, int& idx) {
	TypeContainer ret;
	char ch = sig[idx];
	std::optional<TypeContainer> s;
	switch (ch) {
		case int(DBusDataType::BYTE):
		case int(DBusDataType::BOOLEAN):
		case int(DBusDataType::INT16):
		case int(DBusDataType::UINT16):
		case int(DBusDataType::INT32):
		case int(DBusDataType::UINT32):
		case int(DBusDataType::INT64):
		case int(DBusDataType::UINT64):
		case int(DBusDataType::DOUBLE):
		case int(DBusDataType::STRING):
		case int(DBusDataType::SIGNATURE):
		case int(DBusDataType::OBJECT_PATH):
		case int(DBusDataType::VARIANT):
		case int(DBusDataType::UNIX_FD):
			ret.type = (DBusDataType)ch;
			idx ++;
			break;
		case int(DBusDataType::ARRAY):
			ret.type = DBusDataType::ARRAY;
			idx ++;
			s = do_ParseSignature(sig, idx);
			if (s.has_value())
				ret.members.push_back(s.value());
			break;
		case '(':
			ret.type = DBusDataType::STRUCT;
			idx ++;
			while (sig.at(idx) != ')') {
				s = do_ParseSignature(sig, idx);
				if (s.has_value())
					ret.members.push_back(s.value());
			}
			idx ++;
			break;
		case '{':
			ret.type = DBusDataType::DICT_ENTRY;
			idx ++;
			while (sig.at(idx) != '}') {
				s = do_ParseSignature(sig, idx);
				if (s.has_value())
					ret.members.push_back(s.value());
			}
			idx ++;
			break;
		default: {
			printf("Signature %c in %s not implemented\n", ch, sig.c_str());
			return std::nullopt;
		}
	}
	return ret;
}

TypeContainer ParseOneSignature(const std::string& sig, int* out_idx) {
	int idx = 0;
	TypeContainer ret;
	if (out_idx) { idx = *out_idx; }
	std::optional<TypeContainer> s = do_ParseSignature(sig, idx);
	if (s.has_value()) {
		ret = s.value();
		if (out_idx) { *out_idx = idx; }
	}
	return ret;
}

std::vector<TypeContainer> ParseSignatures(const std::string& sig) {
	int idx = 0;
	std::vector<TypeContainer> ret;
	while (idx < sig.size()) {
		ret.push_back(ParseOneSignature(sig, &idx));
	}
	return ret;
}

DBusMessageFields ParseFields(MessageEndian endian, AlignedStream* stream) {
	DBusMessageFields ret;
	TypeContainer sig = ParseOneSignature("a(yv)");
	std::optional<DBusType> fields_container = ParseContainer(endian, stream, sig);
	if (!fields_container.has_value()) return ret;
	DBusContainer fields = std::get<DBusContainer>(fields_container.value());
	stream->Align(TypeContainer(DBusDataType::STRUCT));

	// fields is a(yv), v is (yv)
	for (const std::variant<DBusVariant, DBusContainer>& v : fields.values) {
		const DBusContainer& c = std::get<DBusContainer>(v);
		DBusVariant v0 = std::get<DBusVariant>(c.values[0]);
		DBusVariant v1 = std::get<DBusVariant>(c.values[1]);
		uint8_t header_type_id = std::get<uint8_t>(v0);
		MessageHeaderType header_type = ToMessageHeaderType(header_type_id);
		switch (header_type) {
			case MessageHeaderType::PATH:
				ret[header_type] = std::get<object_path>(v1); break;
			case MessageHeaderType::INTERFACE:
			case MessageHeaderType::MEMBER:
			case MessageHeaderType::ERROR_NAME:
			case MessageHeaderType::DESTINATION:
			case MessageHeaderType::SENDER:
			case MessageHeaderType::SIGNATURE:
				ret[header_type] = std::get<std::string>(v1); break;
			case MessageHeaderType::REPLY_SERIAL:
			case MessageHeaderType::UNIX_FDS:
				ret[header_type] = std::get<uint32_t>(v1); break;
			default:
				assert(0 && "Unexpected type in message header");
		}
	}
	return ret;
}

// AlignedStream is written so it can be used for parsing the messagebody.
//std::pair<FixedHeader, DBusMessageFields> 
bool ParseHeaderAndBody(const unsigned char* data, int len, FixedHeader* fixed_out, DBusMessageFields* fields_out,
	std::vector<DBusType>* body_out) {
	// Fixed header: 12 bytes
	RawMessage rawmsg(data, len);
	//rawmsg.Print();
	//PrintByteArray(rawmsg.GetHeader(), 16);
	FixedHeader fixed(rawmsg);
	//fixed.Print();

	if (fixed_out) {
		*fixed_out = fixed;
	}

	AlignedStream s;
	s.buf = &rawmsg.data;
	s.offset = 12;
	DBusMessageFields fields = ParseFields(fixed.endian, &s);

	if (fields_out) {
		// Variable header: Starting from [12]
		*fields_out = fields;
	}
	
	if (body_out) {
		std::string sig;
		for (const auto& entry : fields) {
			if (entry.first == MessageHeaderType::SIGNATURE) {
				sig = std::get<std::string>(entry.second);
				break;
			}
		}

		std::vector<TypeContainer> tcs = ParseSignatures(sig);
		for (const auto& tc : tcs) {
			std::optional<DBusType> dt = ParseType(fixed.endian, &s, tc);
			if (dt.has_value()) {
				body_out->push_back(dt.value());
			} else {
				break; // TODO: Mark this packet as partially parsed due to it being truncated
			}
		}
	}
	return true;
}

// Parse fixed-size DBus types (byte, boolean, [u]int16, [u]int32, [u]int64, double, fd)
std::optional<DBusVariant> ParseFixed(MessageEndian endian, AlignedStream* stream, const TypeContainer& tc) {
	DBusVariant ret;
	stream->Align(tc);
	std::optional<std::vector<uint8_t>> b = stream->Take(tc);
	if (!b.has_value()) {
		return std::nullopt;
	}
	std::vector<uint8_t> bytes = b.value();
	switch (tc.type) {
		case DBusDataType::BYTE: {
			ret = uint8_t(bytes[0]);
			break;
		}
		case DBusDataType::BOOLEAN: {
			ret = bool(bytes[0]);
			break;
		}
		case DBusDataType::INT16: {
			if (endian == MessageEndian::LITTLE) {
				ret = BytesToTypeLE<int16_t>(bytes.data());
			} else {
				ret = BytesToTypeBE<int16_t>(bytes.data());
			}
			break;
		}
		case DBusDataType::UINT16:
			if (endian == MessageEndian::LITTLE) {
				ret = BytesToTypeLE<uint16_t>(bytes.data());
			} else {
				ret = BytesToTypeBE<uint16_t>(bytes.data());
			}
			break;
		case DBusDataType::INT32: {
			if (endian == MessageEndian::LITTLE) {
				ret = BytesToTypeLE<int32_t>(bytes.data());
			} else {
				ret = BytesToTypeBE<int32_t>(bytes.data());
			}
			break;
		}
		case DBusDataType::UINT32:
		case DBusDataType::UNIX_FD:
			if (endian == MessageEndian::LITTLE) {
				ret = BytesToTypeLE<uint32_t>(bytes.data());
			} else {
				ret = BytesToTypeBE<uint32_t>(bytes.data());
			}
			break;
		case DBusDataType::INT64:
			if (endian == MessageEndian::LITTLE) {
				ret = BytesToTypeLE<int64_t>(bytes.data());
			} else {
				ret = BytesToTypeBE<int64_t>(bytes.data());
			}
			break;
		case DBusDataType::UINT64:
			if (endian == MessageEndian::LITTLE) {
				ret = BytesToTypeLE<uint64_t>(bytes.data());
			} else {
				ret = BytesToTypeBE<uint64_t>(bytes.data());
			}
			break;
		case DBusDataType::DOUBLE:
			if (endian == MessageEndian::LITTLE) {
				ret = BytesToTypeLE<double>(bytes.data());
			} else {
				ret = BytesToTypeBE<double>(bytes.data());
			}
			break;
		default: {
			printf("Type %s not implemented\n",
				tc.ToString().c_str());
			assert(0 && "not implemented");
		}
	}
	return ret;
}

DBusContainer ParseArray(MessageEndian endian, AlignedStream* stream, const std::vector<TypeContainer>& members) {
	DBusContainer ret;
	std::optional<DBusVariant> tmp = ParseFixed(endian, stream, TypeContainer(DBusDataType::UINT32));
	if (!tmp.has_value()) return ret;
	uint32_t len = std::get<uint32_t>(tmp.value());
	stream->Align(TypeContainer(DBusDataType::ARRAY));
	int offset_start = stream->offset;
	bool done = false;
	while (!done && stream->offset < offset_start + len) {
		for (const TypeContainer& tc : members) {
			int offset = stream->offset;
			std::optional<DBusType> dt = ParseType(endian, stream, tc);
			if (dt.has_value()) {
				ret.values.push_back(dt.value());
			} else {  // Array is parsed on an "as much as possible" basis
				stream->offset = offset;
				done = true;
				break;
			}
		}
	}
	return ret;
}

std::optional<DBusContainer> ParseStruct(MessageEndian endian, AlignedStream* stream, const std::vector<TypeContainer>& members) {
	DBusContainer ret;
	// Align
	stream->Align(TypeContainer(DBusDataType::STRUCT));
	for (const TypeContainer& tc : members) {
		std::optional<DBusType> dt = ParseType(endian, stream, tc);
		if (dt.has_value())
			ret.values.push_back(dt.value());
		else return std::nullopt;
	}
	return ret;
}

std::optional<DBusVariant> ParseString(MessageEndian endian, AlignedStream* stream, const TypeContainer& sig) {
	int size = 0;
	TypeContainer string_size_ty;
	switch (sig.type) {
		case DBusDataType::STRING: string_size_ty.type = DBusDataType::UINT32; break;
		case DBusDataType::OBJECT_PATH: string_size_ty.type = DBusDataType::UINT32; break;
		case DBusDataType::SIGNATURE: string_size_ty.type = DBusDataType::BYTE; break;
		default: {
			printf("String of type %s not implemented\n", sig.ToString().c_str());
			assert(0);
		}
	}
	std::optional<DBusVariant> f = ParseFixed(endian, stream, string_size_ty);
	if (!f.has_value()) return std::nullopt;
	DBusVariant v = f.value();
	if (std::holds_alternative<uint32_t>(v)) {
		size = int(std::get<uint32_t>(v));
	} else if (std::holds_alternative<uint8_t>(v)) {
		size = int(std::get<uint8_t>(v));
	}
	std::string ret = "";
	if (size == 0) { return ret; } // return ""; returns an empty DBusVariant.
	for (int i=0; i<size; i++) { 
		std::optional<uint8_t> b = stream->TakeOneByte();
		if (!b.has_value()) return ret;
		else ret.push_back(b.value());
	}
	stream->TakeOneByte(); // Take one extra byte if the string is non-empty.
	if (sig.type == DBusDataType::OBJECT_PATH) {
		object_path op;
		op.str = ret;
		return op;
	}
	return ret;
}

std::optional<DBusType> ParseVariant(MessageEndian endian, AlignedStream* stream, const TypeContainer& sig) {
	std::optional<DBusVariant> tmp = ParseString(endian, stream, TypeContainer(DBusDataType::SIGNATURE));
	if (!tmp.has_value()) return std::nullopt;
	DBusVariant vs = tmp.value();
	std::string s;
	if (std::holds_alternative<std::string>(vs)) {
		s = std::get<std::string>(vs);
	} else if (std::holds_alternative<object_path>(vs)) {
		s = std::get<object_path>(vs).str;
	} else {
		return std::nullopt;
	}
	TypeContainer tc = ParseOneSignature(s);
	if (!tc.IsValid()) return std::nullopt;
	return ParseType(endian, stream, tc);
}

std::optional<DBusType> ParseContainer(MessageEndian endian, AlignedStream* stream, const TypeContainer& sig) {
	switch (sig.type) {
		case DBusDataType::ARRAY:
			return ParseArray(endian, stream, sig.members);
			break;
		case DBusDataType::STRUCT:
		case DBusDataType::DICT_ENTRY:
			return ParseStruct(endian, stream, sig.members);
			break;
		case DBusDataType::VARIANT:
			return ParseVariant(endian, stream, sig);
			break;
		default:
			assert(0);
		break;
	}
}

std::optional<DBusType> ParseType(MessageEndian endian, AlignedStream* stream, const TypeContainer& tc) {
	if (IsFixedType(tc.type)) {
		return ParseFixed(endian, stream, tc);
	} else if (IsContainerType(tc.type)) {
		return ParseContainer(endian, stream, tc);
	} else if (IsStringType(tc.type)) {
		return ParseString(endian, stream, tc);
	} else {
		assert(0 && "unimplemented");
	}
}

// TODO: Currently we're showing the "cooked" type, meaning: a FD
// shows up as an UINT32.
//
// Need to make them show up properly as DBus types
void do_PrintDBusType(const DBusType& x, const int indent) {
	for (int i=0; i<indent; i++) { printf("  "); }
	if (std::holds_alternative<DBusVariant>(x)) {
		const DBusVariant& v = std::get<DBusVariant>(x);
		if (std::holds_alternative<bool>(v)) {
			printf("b %s\n", std::get<bool>(v) == true ? "true" : "false");
		} else if (std::holds_alternative<uint8_t>(v)) {
			printf("y 0x%x\n", std::get<uint8_t>(v));
		} else if (std::holds_alternative<uint16_t>(v)) {
			printf("q %u\n", uint32_t(std::get<uint16_t>(v)));
		} else if (std::holds_alternative<int16_t>(v)) {
			printf("n %d\n", int32_t(std::get<int16_t>(v)));
		} else if (std::holds_alternative<uint32_t>(v)) {
			printf("u %u\n", std::get<uint32_t>(v));
		} else if (std::holds_alternative<int32_t>(v)) {
			printf("i %d\n", std::get<int32_t>(v));
		} else if (std::holds_alternative<uint64_t>(v)) {
			printf("t %lu\n", std::get<uint64_t>(v));
		} else if (std::holds_alternative<int64_t>(v)) {
			printf("x %ld\n", std::get<int64_t>(v));
		} else if (std::holds_alternative<double>(v)) {
			printf("d %g\n", std::get<double>(v));
		} else if (std::holds_alternative<std::string>(v)) {
			printf("s %s\n", std::get<std::string>(v).c_str());
		} else if (std::holds_alternative<object_path>(v)) {
			printf("o %s\n", std::get<object_path>(v).str.c_str());
		} else {
			printf("(unknown type)\n");
		}
	} else if (std::holds_alternative<DBusContainer>(x)) {
		const DBusContainer& dc = std::get<DBusContainer>(x);
		printf(">> container\n");
		for (const auto& v : dc.values) {
			do_PrintDBusType(v, indent + 1);
		}
		for (int i=0; i<indent; i++) { printf("  "); }
		printf("<< container\n");
	} else {
		printf("(Unknown variant of DBusType)\n");
	}
}

void PrintDBusType(const DBusType& x) {
	do_PrintDBusType(x, 0);
}

void do_DBusTypeToJSON(const DBusType& x, std::ostream& os, int& idx) {
	if (idx > 0) {
		os << ", ";
	}
	if (std::holds_alternative<DBusVariant>(x)) {
		const DBusVariant& v = std::get<DBusVariant>(x);
		if (std::holds_alternative<bool>(v)) {
			if (std::get<bool>(v) == true) {
				os << "true";
			} else {
				os << "false";
			}
		} else if (std::holds_alternative<uint8_t>(v)) {
			os << std::to_string(std::get<uint8_t>(v));
		} else if (std::holds_alternative<uint16_t>(v)) {
			os << std::to_string(std::get<uint16_t>(v));
		} else if (std::holds_alternative<int16_t>(v)) {
			os << std::to_string(std::get<int16_t>(v));
		} else if (std::holds_alternative<uint32_t>(v)) {
			os << std::to_string(std::get<uint32_t>(v));
		} else if (std::holds_alternative<int32_t>(v)) {
			os << std::to_string(std::get<int32_t>(v));
		} else if (std::holds_alternative<uint64_t>(v)) {
			os << std::to_string(std::get<uint64_t>(v));
		} else if (std::holds_alternative<int64_t>(v)) {
			os << std::to_string(std::get<int64_t>(v));
		} else if (std::holds_alternative<double>(v)) {
			std::string vs = std::to_string(std::get<double>(v));
			if (vs == "nan") { vs = "\"NaN\""; }
			else if (vs == "inf") { vs = "\"Infinity\""; }
			else if (vs == "-inf") { vs = "\"-Infinity\""; }
			os << vs;
		} else if (std::holds_alternative<std::string>(v)) {
			os << "\"" << std::get<std::string>(v) << "\"";
		} else if (std::holds_alternative<object_path>(v)) {
			os << "\"" << std::get<object_path>(v).str << "\"";
		} else {
			os << "undefined";
		}
		idx ++;
	} else if (std::holds_alternative<DBusContainer>(x)) {
		const DBusContainer& dc = std::get<DBusContainer>(x);
		os << "[";
		int idx1 = 0;
		for (int i=0; i<int(dc.values.size()); i++) {
			const auto& v = dc.values[i];
			do_DBusTypeToJSON(v, os, idx1);
		}
		os << "]";
		idx ++;
	} else {
		printf("(Unknown variant of DBusType)\n");
	}
}

std::string DBusTypeToJSON(const DBusType& x) {
	std::stringstream ss;
	int idx = 0;
	do_DBusTypeToJSON(x, ss, idx);
	return ss.str();
}

std::string DBusBodyToJSON(const std::vector<DBusType>& body) {
	std::stringstream ss;
	ss << "[ ";
	for (int i=0; i<int(body.size()); i++) {
		if (i > 0) { ss << ", "; }
		std::string x = DBusTypeToJSON(body[i]);
		if (x == "") x = "null";
		ss << x;
	}
	ss << "]";
	return ss.str();
}

// Process 1 packet
void MyCallback(unsigned char* user_data, const struct pcap_pkthdr* pkthdr, const unsigned char* packet) {
	const struct timeval& ts = pkthdr->ts;
	double sec = ts.tv_sec * 1.0 + ts.tv_usec / 1000000.0;

	//printf("Timestamp: %.6f\n", sec);

	int caplen = pkthdr->caplen, len = pkthdr->len;

	if (caplen >= 12) {
		AlignedStream s;
		MessageEndian endian;
		FixedHeader fixed;
		DBusMessageFields fields;
		std::vector<DBusType> body;

		char* x = getenv("VERBOSE");
		if (x) {
			printf("--------- Packet #%d ----------\n", g_num_packets);
		}

		ParseHeaderAndBody(packet, caplen, &fixed, &fields, &body);

		if (x) {
			fixed.Print();
			if (std::string(x) == "json") {
				printf("%s\n", DBusBodyToJSON(body).c_str());
			} else {
				for (const DBusType& x : body) {
					PrintDBusType(x);
				}
			}
		}

		#ifdef DBUS_PCAP_USING_EMSCRIPTEN
		
		std::string path, iface, member, destination, sender;
		uint32_t reply_serial = 0xFFFFFFFF;
		for (const auto& entry : fields) {
			switch (entry.first) {
				case MessageHeaderType::PATH: {
					path = std::get<object_path>(entry.second).str;
					break;
				}
				case MessageHeaderType::INTERFACE: {
					iface = std::get<std::string>(entry.second);
					break;
				}
				case MessageHeaderType::MEMBER: {
					member = std::get<std::string>(entry.second);
					break;
				}
				case MessageHeaderType::DESTINATION: {
					destination = std::get<std::string>(entry.second);
					break;
				}
				case MessageHeaderType::SENDER: {
					sender = std::get<std::string>(entry.second);
					break;
				}
				case MessageHeaderType::REPLY_SERIAL: {
					reply_serial = std::get<uint32_t>(entry.second);
					break;
				}
				default: break;
			}
		}

		// timestamp, type, serial, reply_serial, sender, destination, path, iface, member
		EM_ASM_ARGS({
			OnNewDBusMessage($0,                // timestamp (number)
							 $1,                // type (number)
							 $2,                // serial (number)
							 $3,                // reply_serial (number)
							 UTF8ToString($4),  // sender (string)
							 UTF8ToString($5),  // destination (string)
							 UTF8ToString($6),  // path (string)
							 UTF8ToString($7),  // iface (string)
							 UTF8ToString($8),  // member (string)
							 UTF8ToString($9),  // body (JSON)
							 );
		}, sec, int(fixed.type), fixed.cookie, reply_serial, 
			sender.c_str(), destination.c_str(), path.c_str(), iface.c_str(), member.c_str(),
			DBusBodyToJSON(body).c_str());
		#endif
	}

	g_num_packets ++;
}

void PrintByteArray(const std::vector<uint8_t>& arr, const int width = 16) {
	const int len = int(arr.size());
	for (int i=0; i<len; i+=width) {
		std::string disp;
		printf("%06x:", i);
		int ub = std::min(len, i+width);
		for (int j=i; j<ub; j++) {
			printf(" %02x", arr[j]);
			if (arr[j] >= 32 && arr[j] < 128) {
				disp.push_back((char)arr[j]);
			} else disp.push_back('.');
		}
		for (int j=ub; j<i+width; j++) {
			printf("   ");
		}
		printf(" | %s\n", disp.c_str());
	}
}

void ProcessByteArray(const std::vector<uint8_t>& buf) {
	std::FILE* tmpf = std::tmpfile();
	char errbuf[PCAP_ERRBUF_SIZE];
	memset(errbuf, 0x00, sizeof(errbuf));
	pcap_t* the_pcap = nullptr;
	printf("%zd bytes\n", buf.size());
	char *v = getenv("VERBOSE");
	if (v && std::atoi(v) > 1)
		PrintByteArray(buf);
	std::filesystem::path p = std::filesystem::path("/proc/self/fd") /
		std::to_string(fileno(tmpf));
	std::string fn = std::filesystem::read_symlink(p);
	printf("Temp file: %s\n", fn.c_str());

	std::fwrite(buf.data(), 1, buf.size(), tmpf);
	std::rewind(tmpf);

	the_pcap = pcap_fopen_offline(tmpf, errbuf);

	if (the_pcap == nullptr) {
		printf("pcap_open_offline failed: %s\n", errbuf);
		return;
	}
	int rc;
	if ((rc = pcap_loop(the_pcap, 0, MyCallback, nullptr)) < 0) {
		printf("pcap_loop failed:, rc=%d, errbuf=%s\n",
			rc, errbuf);
		printf("%d packets processed so far\n", g_num_packets);
		return;
	}
	pcap_close(the_pcap);
	printf("%d packets in file\n", g_num_packets);
}