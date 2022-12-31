all: main_gles

util.o: util.cpp
	g++ $< -c -o $@

scene.o: scene.cpp
	g++ $< -c -o $@

main_gles: main_gles.cpp util.o scene.o
	g++ $^ -o main_gles -lglfw -lGLESv2

clean:
	@for x in util.o scene.o main_gles; do [ -f $$x ] && rm -v $$x ; done
