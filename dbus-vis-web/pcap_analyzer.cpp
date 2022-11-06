#include <stdint.h>

#include <iostream>
#include <string>
#include <vector>

#include <pcap/pcap.h>

int g_num_packets = 0;

bool ParseHeader(unsigned char* data) {
	return true;
}

void MyCallback(unsigned char* user_data, const struct pcap_pkthdr* pkthdr, const unsigned char* packet) {
	int caplen = pkthdr->caplen, len = pkthdr->len;
	printf("caplen=%d, len=%d\n", caplen, len);
	g_num_packets ++;
}

int main(int argc, char** argv) {
	if (argc >= 2 && std::string(argv[1]) == "help") {
		printf("Usage 1: %s (Pipe in a PCAP file)\n", argv[0]);
		printf("Usage 2: %s PCAP_FILE_NAME\n", argv[0]);
		return 0;
	}

	if (argc < 2) {
		std::vector<uint8_t> buf;
		while (std::cin.good()) {
			buf.push_back(std::cin.get());
		}
		printf("%zd bytes\n", buf.size());
		return 0;
	}

	pcap_t* the_pcap = nullptr;
	char errbuf[PCAP_ERRBUF_SIZE];
	the_pcap = pcap_open_offline(argv[1], errbuf);
	if (the_pcap == nullptr) {
		printf("pcap_open_offline failed: %s\n", errbuf);
		return 1;
	}
	if (pcap_loop(the_pcap, 0, MyCallback, nullptr) < 0) {
		printf("pcap_loop failed: %s\n", errbuf);
		return 1;
	}
	pcap_close(the_pcap);
	printf("%d packets in file\n", g_num_packets);
	return 0;
}
