all: ps
clean:
	rm -rf *.o
main.o: main.c
	gcc -c -o main.o main.c
framebuffer.o: framebuffer.c
	gcc -c -o framebuffer.o framebuffer.c
space.o: space.c
	gcc -c -o space.o space.c -lm
ps: main.o framebuffer.o space.o
	gcc -o ps main.o framebuffer.o space.o -lm