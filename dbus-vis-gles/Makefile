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

main_gles: main_gles.cpp util.o scene.o camera.o chunk.o rendertarget.o chunkindex.o sprite.o dbus_pcap_scene.o
	g++ $^ $(CXXFLAGS) -o main_gles -lglfw -lGLESv2

main_gles.js: main_gles.cpp util.cpp scene.cpp camera.cpp chunk.cpp rendertarget.cpp chunkindex.cpp sprite.cpp dbus_pcap_scene.cpp
	@echo "Will need to first activate the Emscripten SDK."
	@em++ $^ -O2 -o $@ --use-preload-plugins -s USE_WEBGL2=1 -s USE_GLFW=3 \
		-s FULL_ES3=1 -s FULL_ES2=1 \
		-s ASSERTIONS=2 \
		-s ALLOW_MEMORY_GROWTH=1 \
		--preload-file vox/hwmon.vox \
		--preload-file vox/openbmc.vox \
		--preload-file vox/player.vox \
		--preload-file shaders/hellotriangle.fs \
		--preload-file shaders/hellotriangle.vs \
		--preload-file shaders/palettescene.vs \
		--preload-file shaders/simple_texture.vs \
		--preload-file shaders/simple_texture.fs \
		--preload-file shaders/vert_mvp_palette.vs \
		--preload-file shaders/vert_norm_data_ao.fs \
		--preload-file shaders/vert_norm_data_ao.vs

clean:
	@for x in util.o scene.o rendertarget.o chunk.o camera.o\
	  chunkindex.o dbus_pcap_scene.o sprite.o \
	  main_gles; do [ -f $$x ] && rm -v $$x ; done
