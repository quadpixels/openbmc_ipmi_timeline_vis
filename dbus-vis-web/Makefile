CXX=g++
pcap_analyzer: pcap_analyzer.cpp pcap_analyzer.hpp
	$(CXX) pcap_analyzer.cpp -o $@ $(CFLAGS) -lpcap -std=c++17 -g

pcap_analyzer.js: pcap_analyzer.cpp
	@echo "Will need to first activate the Emscripten SDK."
	@em++ $^ -o $@ --preload-file evb-ast2500.pcap -lpcap -L/usr/local/lib -I/usr/local/include -std=c++17 -sEXPORTED_FUNCTIONS=_func1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap

clean:
	@echo "Cleaning . . ."
	@if [ -f "pcap_analyzer" ]; then\
	    rm -v pcap_analyzer; \
		fi
