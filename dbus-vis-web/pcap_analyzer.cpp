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

// How to call from JavaScript side:
// Module.ccall("func1", "undefined", ["array", "number"], [new Uint8Array([0xff, 0xfa, 0xfc]), 3])
void func1(char* buf, int buf_size) {
	printf("buf_size=%d\n", buf_size);
	std::vector<uint8_t> b(buf_size);
	memcpy(b.data(), buf, buf_size);
	//PrintByteArray(b, 16);

	std::vector<uint8_t> buf1;
	for (int i=0; i<buf_size; i++) {
		buf1.push_back(buf[i]);
	}
	ProcessByteArray(buf1);

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

uint32_t BytesToUint32LE(const uint8_t* data) {
	uint32_t ret = data[0] |
								 (data[1] << 8) |
								 (data[2] << 16) |
								 (data[3] << 24);
	return ret;
}

uint32_t BytesToUint32BE(const uint8_t* data) {
	uint32_t ret = data[3] |
								 (data[2] << 8) |
								 (data[1] << 16) |
								 (data[0] << 24);
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
		length = BytesToUint32LE(header.data()+4);
		cookie = BytesToUint32LE(header.data()+8);
	} else {
		length = BytesToUint32BE(header.data()+4);
		cookie = BytesToUint32BE(header.data()+8);
	}
}

void FixedHeader::Print() {
	printf("[FixedHeader] type:%s, length:%u, cookie:%u\n",
		MessageTypeStr[int(type)],
		length,
		cookie);
}

TypeContainer do_ParseSignature(const std::string& sig, int& idx) {
	TypeContainer ret;
	char ch = sig[idx];
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
			ret.members.push_back(do_ParseSignature(sig, idx));
			break;
		case '(':
			ret.type = DBusDataType::STRUCT;
			idx ++;
			while (sig.at(idx) != ')') {
				ret.members.push_back(do_ParseSignature(sig, idx));
			}
			idx ++;
			break;
		default: {
			printf("Signature %c in %s not implemented\n", ch, sig.c_str());
			assert(0);
		}
	}
	return ret;
}

TypeContainer ParseSignature(const std::string& sig) {
	int idx = 0;
	return do_ParseSignature(sig, idx);
}

DBusMessageFields ParseFields(MessageEndian endian, AlignedStream* stream) {
	TypeContainer sig = ParseSignature("a(yv)");
	DBusContainer fields = std::get<DBusContainer>(ParseContainer(endian, stream, sig));
	stream->Align(TypeContainer(DBusDataType::STRUCT));

	DBusMessageFields ret;

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

std::pair<FixedHeader, DBusMessageFields> ParseHeader(const unsigned char* data, int len) {
	// Fixed header: 12 bytes
	RawMessage rawmsg(data, len);
	//rawmsg.Print();
	//PrintByteArray(rawmsg.GetHeader(), 16);
	FixedHeader fixed(rawmsg);
	fixed.Print();

	// Variable header: Starting from [12]
	AlignedStream s;
	std::vector<uint8_t> buf;
	s.buf = &rawmsg.data;
	s.offset = 12;
	DBusMessageFields fields = ParseFields(fixed.endian, &s);

	for (const auto& f : fields) {
		printf("%s: %s\n", MessageHeaderTypeName(f.first).c_str(), 
			VariantToString(f.second).c_str());
	}

	return std::make_pair(fixed, fields);
}

// Parse fixed-size DBus types (byte, boolean, [u]int16, [u]int32, [u]int64, double, fd)
DBusVariant ParseFixed(MessageEndian endian, AlignedStream* stream, const TypeContainer& tc) {
	DBusVariant ret;
	std::vector<uint8_t> bytes = stream->Take(tc);
	switch (tc.type) {
		case DBusDataType::BYTE: {
			ret = uint8_t(bytes[0]);
			break;
		}
		case DBusDataType::UINT32:
			if (endian == MessageEndian::LITTLE) {
				ret = BytesToUint32LE(bytes.data());
			} else {
				ret = BytesToUint32LE(bytes.data());
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
	uint32_t len = std::get<uint32_t>(ParseFixed(endian, stream,
		TypeContainer(DBusDataType::UINT32)));
	stream->Align(TypeContainer(DBusDataType::ARRAY));
	int offset = stream->offset;
	while (stream->offset < offset + len) {
		for (const TypeContainer& tc : members) {
			ret.values.push_back(ParseType(endian, stream, tc));
		}
	}
	return ret;
}

DBusContainer ParseStruct(MessageEndian endian, AlignedStream* stream, const std::vector<TypeContainer>& members) {
	DBusContainer ret;
	// Align
	stream->Align(TypeContainer(DBusDataType::STRUCT));
	for (const TypeContainer& tc : members) {
		ret.values.push_back(ParseType(endian, stream, tc));
	}
	return ret;
}

DBusVariant ParseString(MessageEndian endian, AlignedStream* stream, const TypeContainer& sig) {
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
	DBusVariant v = ParseFixed(endian, stream, string_size_ty);
	if (std::holds_alternative<uint32_t>(v)) {
		size = int(std::get<uint32_t>(v));
	} else if (std::holds_alternative<uint8_t>(v)) {
		size = int(std::get<uint8_t>(v));
	}
	if (size == 0) { return ""; }
	std::string ret;
	for (int i=0; i<size; i++) { ret.push_back(stream->TakeOneByte()); }
	stream->TakeOneByte(); // Take one extra byte if the string is non-empty.

	if (sig.type == DBusDataType::OBJECT_PATH) {
		object_path op;
		op.str = ret;
		return op;
	}
	return ret;
}

DBusType ParseVariant(MessageEndian endian, AlignedStream* stream, const TypeContainer& sig) {
	DBusVariant vs = ParseString(endian, stream, TypeContainer(DBusDataType::SIGNATURE));
	std::string s;
	if (std::holds_alternative<std::string>(vs)) {
		s = std::get<std::string>(vs);
	} else {
		s = std::get<object_path>(vs).str;
	}
	TypeContainer tc = ParseSignature(s);
	return ParseType(endian, stream, tc);
}

DBusType ParseContainer(MessageEndian endian, AlignedStream* stream, const TypeContainer& sig) {
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

DBusType ParseType(MessageEndian endian, AlignedStream* stream, const TypeContainer& tc) {
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

// Process 1 packet
void MyCallback(unsigned char* user_data, const struct pcap_pkthdr* pkthdr, const unsigned char* packet) {
	const struct timeval& ts = pkthdr->ts;
	double sec = ts.tv_sec * 1.0 + ts.tv_usec / 1000000.0;
	printf("Timestamp: %.6f\n", sec);

	int caplen = pkthdr->caplen, len = pkthdr->len;

	if (caplen >= 12) {
		auto [fixed, fields] = ParseHeader(packet, caplen);
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
							 UTF8ToString($8)   // member (string)
							 );
		}, sec, int(fixed.type), fixed.cookie, reply_serial, 
			sender.c_str(), destination.c_str(), path.c_str(), iface.c_str(), member.c_str());
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
	memset(errbuf, sizeof(errbuf), 0);
	pcap_t* the_pcap = nullptr;
	printf("%zd bytes\n", buf.size());
	if (getenv("VERBOSE"))
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