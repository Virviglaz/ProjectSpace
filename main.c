#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "framebuffer.h"
#include "dri_buffer.h"

int main(int argc, char** argv)
{
	printf ("DRI create result: %u\n", dri_createFB(0));

	void * dri_buffer = dri_getFB(0);
	
	printf("DRI buffer at %p\n", dri_buffer);

	printf("Framebuffer: %s\n", FrameBufferInit(0, 1, dri_buffer));
	//printf("Set resolution: %s\n", SetScreenSize(1920, 1080, 2));

	//SetResolution(1920, 1080);
	//dri_init(0);

	extern void space_init (uint16_t objects, uint32_t backColor);
	space_init(argc == 2 ? atoi(argv[1]) : 0, BLACK32);

	FrameBufferDeInit();

	return 0;
}
