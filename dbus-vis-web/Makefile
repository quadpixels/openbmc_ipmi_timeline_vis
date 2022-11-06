CXX=g++
pcap_analyzer: pcap_analyzer.cpp
	$(CXX) $^ -o $@ $(CFLAGS) -lpcap

pcap_analyzer.js: pcap_analyzer.cpp
	@echo "Will need to first activate the Emscripten SDK."
	@em++ $^ -o $@ --preload-file evb-ast2500.pcap -lpcap -L/usr/local/lib -I/usr/local/include 

clean:
	@echo "Cleaning . . ."
	@if [ -f "pcap_analyzer" ]; then\
	    rm -v pcap_analyzer; \
		fi
