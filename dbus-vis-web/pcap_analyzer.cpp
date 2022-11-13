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

#ifdef EMSCRIPTEN_VERSION
#include <emscripten.h>
#endif

#include <pcap/pcap.h>

#include "pcap_analyzer.hpp"

int g_num_packets = 0;

void PrintByteArray(const std::vector<uint8_t>&, const int);

#ifdef EMSCRIPTEN_VERSION

extern "C" {

// How to call from JavaScript side:
// Module.ccall("func1", "undefined", ["array", "number"], [new Uint8Array([0xff, 0xfa, 0xfc]), 3])
void func1(char* buf, int buf_size) {
	std::vector<uint8_t> b(buf_size);
	memcpy(b.data(), buf, buf_size);
	PrintByteArray(b, 16);
	EM_ASM_ARGS({
		OnNewDBusMessage(UTF8ToString($0), UTF8ToString($1));
	}, "sender", "destination");
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

// Library funcs
RawMessage::RawMessage(const uint8_t* buf, int len) {
	if (len < 12) {
		assert(false && "Must be at least 12 bytes long");
	}
	endian = MessageEndianLookup(buf[0]);
	for (int i=0; i<12; i++) { header.push_back(buf[i]); }
	for (int i=12; i<len; i++) { data.push_back(buf[i]); }
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
	endian  = MessageEndianLookup(raw.header[0]);
	type    = MessageType(raw.header[1]);
	flags   = raw.header[2];
	version = raw.header[3];
	if (raw.endian == MessageEndian::LITTLE) {
		length = BytesToUint32LE(raw.header.data()+4);
		cookie = BytesToUint32LE(raw.header.data()+8);
	} else {
		length = BytesToUint32BE(raw.header.data()+4);
		cookie = BytesToUint32BE(raw.header.data()+8);
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

bool ParseHeader(const unsigned char* data, int len) {
	RawMessage rawmsg(data, len);
	//rawmsg.Print();
	PrintByteArray(rawmsg.header, 16);
	FixedHeader fixed(rawmsg);
	fixed.Print();

	return true;
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

void ParseArray(MessageEndian endian, AlignedStream* stream, const std::vector<TypeContainer>& members) {
	uint32_t len = std::get<uint32_t>(ParseFixed(endian, stream,
		TypeContainer(DBusDataType::UINT32)));
	stream->Align(TypeContainer(DBusDataType::ARRAY));
	int offset = stream->offset;
	while (stream->offset < offset + len) {
		for (const TypeContainer& tc : members) {
			ParseType(endian, stream, tc);
		}
	}
}

void ParseStruct(MessageEndian endian, AlignedStream* stream, const std::vector<TypeContainer>& members) {
	// Align
	stream->Align(TypeContainer(DBusDataType::STRUCT));
	for (const TypeContainer& tc : members) {
		ParseType(endian, stream, tc);
	}
}

std::string ParseString(MessageEndian endian, AlignedStream* stream, const TypeContainer& sig) {
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
	printf("String of size %d: %s\n", size, ret.c_str());
	stream->TakeOneByte(); // Take one extra byte.
	return ret;
}

void ParseVariant(MessageEndian endian, AlignedStream* stream, const TypeContainer& sig) {
	std::string vs = ParseString(endian, stream, TypeContainer(DBusDataType::SIGNATURE));
	TypeContainer tc = ParseSignature(vs);
	ParseType(endian, stream, tc);
}

void ParseContainer(MessageEndian endian, AlignedStream* stream, const TypeContainer& sig) {
	switch (sig.type) {
		case DBusDataType::ARRAY:
			ParseArray(endian, stream, sig.members);
			break;
		case DBusDataType::STRUCT:
			ParseStruct(endian, stream, sig.members);
			break;
		case DBusDataType::DICT_ENTRY:
			assert(0);
		case DBusDataType::VARIANT:
			ParseVariant(endian, stream, sig);
			break;
		break;
	}
}

void ParseType(MessageEndian endian, AlignedStream* stream, const TypeContainer& tc) {
	if (IsFixedType(tc.type)) {
		ParseFixed(endian, stream, tc);
	} else if (IsContainerType(tc.type)) {
		ParseContainer(endian, stream, tc);
	} else if (IsStringType(tc.type)) {
		ParseString(endian, stream, tc);
	} else {
		assert(0 && "unimplemented");
	}
}

// Process 1 packet
void MyCallback(unsigned char* user_data, const struct pcap_pkthdr* pkthdr, const unsigned char* packet) {
	int caplen = pkthdr->caplen, len = pkthdr->len;

	if (caplen >= 12) {
		ParseHeader(packet, caplen);
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

void Tests() {
	{
		std::string utf8string = "哈"; // code point 21704, {0xe5, 0x93, 0x88} in UTF-8, {0xb9, 0xfe} in GBK
		printf("%s (length=%d)\n", utf8string.c_str(), int(utf8string.size()));
	}
	{
		std::vector<uint8_t> bytes = { 1, 2, 3, 4 };
		AlignedStream stream;
		stream.buf = &bytes;
		TypeContainer tc;
		tc.type = DBusDataType::UINT32;
		printf("%u\n", std::get<uint32_t>(ParseFixed(MessageEndian::LITTLE,
			&stream, tc)));
	}

	{
		TypeContainer tc = ParseSignature("a(yv)");
//		tc.Print();
	}

	{
		std::vector<uint8_t> bytes = {
			0x00, 0x00, 0x00, 0x00, 0x9d, 0x00, 0x00, 0x00,
			0x01, 0x01, 0x6f, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x2f, 0x78, 0x79, 0x7a, 0x2f, 0x6f, 0x70, 0x65,
			0x6e, 0x62, 0x6d, 0x63, 0x5f, 0x70, 0x72, 0x6f, 0x6a, 0x65, 0x63, 0x74, 0x2f, 0x73, 0x65, 0x6e,
			0x73, 0x6f, 0x72, 0x73, 0x2f, 0x75, 0x74, 0x69, 0x6c, 0x69, 0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e,
			0x2f, 0x43, 0x50, 0x55, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x73, 0x00, 0x1f, 0x00, 0x00, 0x00,
			0x6f, 0x72, 0x67, 0x2e, 0x66, 0x72, 0x65, 0x65, 0x64, 0x65, 0x73, 0x6b, 0x74, 0x6f, 0x70, 0x2e,
			0x44, 0x42, 0x75, 0x73, 0x2e, 0x50, 0x72, 0x6f, 0x70, 0x65, 0x72, 0x74, 0x69, 0x65, 0x73, 0x00,
			0x03, 0x01, 0x73, 0x00, 0x11, 0x00, 0x00, 0x00, 0x50, 0x72, 0x6f, 0x70, 0x65, 0x72, 0x74, 0x69,
			0x65, 0x73, 0x43, 0x68, 0x61, 0x6e, 0x67, 0x65, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x08, 0x01, 0x67, 0x00, 0x08, 0x73, 0x61, 0x7b, 0x73, 0x76, 0x7d, 0x61, 0x73, 0x00, 0x00, 0x00,
			0x07, 0x01, 0x73, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3a, 0x31, 0x2e, 0x37, 0x00, 0x00, 0x00, 0x00,
		};
		AlignedStream stream;
		stream.buf = &bytes;
		stream.offset = 4;
		TypeContainer tc = ParseSignature("a(yv)");
		ParseType(MessageEndian::LITTLE, &stream, tc);
	}
}

int main(int argc, char** argv) {

	if (getenv("TEST")) {
		printf("Running tests\n");
		Tests();
		return 0;
	}

	if (argc >= 2 && std::string(argv[1]) == "help") {
		printf("Usage 1: %s (Pipe in a PCAP file)\n", argv[0]);
		printf("Usage 2: %s PCAP_FILE_NAME\n", argv[0]);
		return 0;
	}

	std::vector<uint8_t> buf;
	std::istream* is;

	if (argc < 2) {
		is = &std::cin;
	} else {
		is = new std::ifstream(argv[1]);
	}
	while (is->good()) { buf.push_back(is->get()); }

	ProcessByteArray(buf);

	return 0;
}
