all: main_gles

CXXFLAGS=-g

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

main_gles: main_gles.cpp util.o scene.o camera.o chunk.o rendertarget.o chunkindex.o
	g++ $^ $(CXXFLAGS) -o main_gles -lglfw -lGLESv2

clean:
	@for x in util.o scene.o rendertarget.o chunk.o camera.o main_gles; do [ -f $$x ] && rm -v $$x ; done
