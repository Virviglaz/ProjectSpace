#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "framebuffer.h"

int main(int argc, char** argv)
{
	printf("Framebuffer: %s\n", FrameBufferInit(0));

	extern void space_init (uint16_t objects, uint32_t backColor);
	space_init(argc == 2 ? atoi(argv[1]) : 0, BLACK32);

	FrameBufferDeInit();

	return 0;
}
