#include "pcap_analyzer.hpp"

#include <fstream>

int main(int argc, char** argv) {
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
