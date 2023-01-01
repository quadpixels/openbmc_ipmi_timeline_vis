all: main_gles

CXXFLAGS=-g

sprite.o: sprite.cpp
	g++ $< -c $(CXXFLAGS) -o $@

dbus_pcap_scene.o: dbus_pcap_scene.cpp
	g++ $< -c $(CXXFLAGS) -o $@

chunkindex.o: chunkindex.cpp
	g++ $< -c $(CXXFLAGS) -o $@

rendertarget.o: rendertarget.cpp
	g++ $< -c $(CXXFLAGS) -o $@

chunk.o: chunk.cpp
	g++ $< -c $(CXXFLAGS) -o $@

camera.o: camera.cpp
	g++ $< -c $(CXXFLAGS) -o $@

util.o: util.cpp
	g++ $< -c $(CXXFLAGS) -o $@

scene.o: scene.cpp
	g++ $< -c $(CXXFLAGS) -o $@

main_gles: main_gles.cpp util.o scene.o camera.o chunk.o rendertarget.o chunkindex.o sprite.o
	g++ $^ $(CXXFLAGS) -o main_gles -lglfw -lGLESv2

clean:
	@for x in util.o scene.o rendertarget.o chunk.o camera.o\
	  chunkindex.o dbus_pcap_scene.o sprite.o \
	  main_gles; do [ -f $$x ] && rm -v $$x ; done
