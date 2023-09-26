all: main_gles

CXXFLAGS=-g -std=c++17

animator.o: animator.cpp
	g++ $< -c $(CXXFLAGS) -o $@

pcap_analyzer.o: pcap_analyzer.cpp
	g++ $< -c $(CXXFLAGS) -o $@

sprite.o: sprite.cpp
	g++ $< -c $(CXXFLAGS) -o $@

dbus_pcap_scene.o: dbus_pcap_scene.cpp
	g++ $< -c $(CXXFLAGS) -o $@

accompaniment_scene.o: accompaniment_scene.cpp
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

main_gles: main_gles.cpp util.o scene.o camera.o chunk.o rendertarget.o chunkindex.o sprite.o dbus_pcap_scene.o pcap_analyzer.o animator.o accompaniment_scene.o
	g++ $^ $(CXXFLAGS) -o main_gles -lglfw -lGLESv2 -lpcap -lpthread

main_gles.js: main_gles.cpp util.cpp scene.cpp camera.cpp chunk.cpp rendertarget.cpp chunkindex.cpp sprite.cpp dbus_pcap_scene.cpp animator.cpp accompaniment_scene.cpp
	@echo "Will need to first activate the Emscripten SDK."
	@em++ $^ -O2 -o $@ --use-preload-plugins -s USE_WEBGL2=1 -s USE_GLFW=3 \
		-s FULL_ES3=1 -s FULL_ES2=1 \
		-s ASSERTIONS=2 \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s MODULARIZE -s EXPORT_NAME="MainGLESModule" \
		-s EXPORTED_FUNCTIONS=_DBusServiceFadeOut,_DBusMakeMethodCall,_DBusEmitSignal,_main \
		-s EXPORTED_RUNTIME_METHODS=ccall,cwrap \
		-s TOTAL_MEMORY=128MB \
		-std=c++17 \
		--preload-file vox/bg.vox \
		--preload-file vox/defaultdaemon.vox \
		--preload-file vox/hwmon.vox \
		--preload-file vox/mapper.vox \
		--preload-file vox/openbmc.vox \
		--preload-file vox/player.vox \
		--preload-file vox/em.vox \
		--preload-file vox/ipmi.vox \
		--preload-file vox/sxqn.vox \
		--preload-file vox/narcissus_tazetta.vox \
		--preload-file shaders/hellotriangle.fs \
		--preload-file shaders/hellotriangle.vs \
		--preload-file shaders/palettescene.vs \
		--preload-file shaders/simple_texture.vs \
		--preload-file shaders/simple_texture.fs \
		--preload-file shaders/simple_texture_stencil.fs \
		--preload-file shaders/vert_mvp_palette.vs \
		--preload-file shaders/vert_norm_data_ao.fs \
		--preload-file shaders/vert_norm_data_ao.vs \
		--preload-file shaders/draw_lines.fs \
		--preload-file shaders/draw_lines.vs \
		--preload-file shaders/quad.vs

clean:
	@for x in util.o scene.o rendertarget.o chunk.o camera.o\
	  chunkindex.o dbus_pcap_scene.o sprite.o main_gles.js \
	  pcap_analyzer.o animator.o accompaniment_scene.o \
	  main_gles; do [ -f $$x ] && rm -v $$x ; done
