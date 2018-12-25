FLAGS=`pkg-config --cflags --libs libdrm`
FLAGS+=-Wall -O0 -g
FLAGS+=-D_FILE_OFFSET_BITS=64

all: ps
clean:
	rm -rf *.o
main.o: main.c
	gcc -c -o main.o main.c
framebuffer.o: framebuffer.c
	gcc -c -o framebuffer.o framebuffer.c -lm
dri_buffer.o: dri_buffer.c
	gcc -c -o dri_buffer.o dri_buffer.c $(FLAGS)
space.o: space.c
	gcc -c -o space.o space.c -lm
ps: main.o framebuffer.o space.o dri_buffer.o
	gcc -o ps main.o framebuffer.o space.o dri_buffer.o -lm $(FLAGS)
