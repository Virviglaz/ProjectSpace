#ifndef DRI_BUFFER_H
#define DRI_BUFFER_H

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

int dri_createFB(const char *device);
void * dri_getFB(uint8_t fb_num);
void dri_cleanFB(void);

#endif //DRI_BUFFER_H
