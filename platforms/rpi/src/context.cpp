#include "context.h"
#include "platform.h"

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <fstream>
#include <termios.h>
#include <linux/input.h>


#define ARRAY_SIZE(__arr) (sizeof(__arr)/sizeof(__arr[0]))

// Main global variables
//-------------------------------
#define check() assert(glGetError() == 0)
EGLDisplay display;
EGLSurface surface;
EGLContext context;

struct Viewport {
    int x = 0;
    int y = 0;
    int width = 240;
    int height = 320;
};
Viewport viewport;

static bool bRender;
static unsigned char keyPressed;

#define CSHOW(att) res = eglGetConfigAttrib(d,c,att,&val); 		\
			if (EGL_TRUE == res) {				\
				printf(#att " == %d\n", val);		\
			} else {					\
				printf("error 0x%x reading " #att "\n",	\
					eglGetError());			\
			}

static void dumpConfig(EGLDisplay d, EGLConfig c)
{
	EGLBoolean res;
	EGLint val;
	CSHOW(EGL_ALPHA_SIZE);
	CSHOW(EGL_ALPHA_MASK_SIZE);
	CSHOW(EGL_BIND_TO_TEXTURE_RGB);
	CSHOW(EGL_BIND_TO_TEXTURE_RGBA);
	CSHOW(EGL_BLUE_SIZE);
	CSHOW(EGL_BUFFER_SIZE);
	CSHOW(EGL_COLOR_BUFFER_TYPE);
	CSHOW(EGL_CONFIG_CAVEAT);
	CSHOW(EGL_CONFIG_ID);
	CSHOW(EGL_CONFORMANT);
	CSHOW(EGL_DEPTH_SIZE);
	CSHOW(EGL_GREEN_SIZE);
	CSHOW(EGL_LEVEL);
	CSHOW(EGL_LUMINANCE_SIZE);
	CSHOW(EGL_MAX_PBUFFER_WIDTH);
	CSHOW(EGL_MAX_PBUFFER_HEIGHT);
	CSHOW(EGL_MAX_PBUFFER_PIXELS);
	CSHOW(EGL_MAX_SWAP_INTERVAL);
	CSHOW(EGL_MIN_SWAP_INTERVAL);
	CSHOW(EGL_NATIVE_RENDERABLE);
	CSHOW(EGL_NATIVE_VISUAL_ID);
	CSHOW(EGL_NATIVE_VISUAL_TYPE);
	CSHOW(EGL_RED_SIZE);
	CSHOW(EGL_RENDERABLE_TYPE);
	CSHOW(EGL_SAMPLE_BUFFERS);
	CSHOW(EGL_SAMPLES);
	CSHOW(EGL_STENCIL_SIZE);
	CSHOW(EGL_SURFACE_TYPE);
	CSHOW(EGL_TRANSPARENT_TYPE);
	CSHOW(EGL_TRANSPARENT_RED_VALUE);
	CSHOW(EGL_TRANSPARENT_GREEN_VALUE);
	CSHOW(EGL_TRANSPARENT_BLUE_VALUE);
}

// OpenGL ES
//--------------------------------

//==============================================================================
//  Creation of EGL context (lines 241-341) from:
//
//      https://github.com/raspberrypi/firmware/blob/master/opt/vc/src/hello_pi/hello_triangle2/triangle2.c#L100
//
/*
Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//==============================================================================
void createSurface(int x, int y, int width, int height) {

	printf("%s: %dx%d at [%d,%d]\n", __func__, width, height, x, y);
	EGLBoolean result = 0;
       	EGLConfig configs[32];
	EGLint num_configs;

	static const EGLint attribute_list[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_BUFFER_SIZE, 24,
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
		EGL_NONE
	};
	
	static const EGLint config_attribs[] = {
		EGL_CONFIG_ID, 5,
		EGL_NONE
	};

	static const EGLint context_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	
	static const EGLint surface_attributes[] = {
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
		EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
		EGL_NONE
	};

	// get an EGL surfaceless display connection
/*
	display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
                                        EGL_DEFAULT_DISPLAY,
					attribute_list);
*/
        display = eglGetDisplay(EGL_PLATFORM_SURFACELESS_MESA);
	assert(display != EGL_NO_DISPLAY);
	printf("display %p\n", display);

	// initialize the EGL display connection
	result = eglInitialize(display, NULL, NULL);
	assert(EGL_FALSE != result);

#if 0
	if (eglGetConfigs(display, configs, ARRAY_SIZE(configs), &num_configs)) {
		printf("Got %u configs\n", num_configs);
		EGLint i;
		for (i=0; i < num_configs; i++) {
			printf("--------config[%d]\n", i);
			dumpConfig(display, configs[i]);
		}
	} else
		printf("Error 0x%x getting configs\n", eglGetError());
#endif
 
	// get an appropriate EGL frame buffer configuration
	EGLConfig config = NULL;
	EGLint num_config = 0;
	result = eglChooseConfig(display, config_attribs, &config, 1, &num_config);

	assert(EGL_FALSE != result);
	assert(0 != config);
	printf("config %p\n", config);
	dumpConfig(display, config);

	// get an appropriate EGL frame buffer configuration
	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(EGL_FALSE != result);

	// create an EGL rendering context
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
	assert(context != EGL_NO_CONTEXT);
	printf("have context %p\n", context);

	// Set viewport size.
	viewport.x = x;
	viewport.y = y;
	viewport.width = 240;
	viewport.height = 320;

	surface = eglCreatePbufferSurface(display, config, surface_attributes);
// 	surface = eglCreateWindowSurface(display, config, 0, NULL);
	assert(surface != EGL_NO_SURFACE);
	assert(surface != 0);
	if (surface) {
		printf("have surface %p\n", surface);
	} else {
		printf("error 0x%x getting surface\n", eglGetError());
		exit(1);
	}

	// connect the context to the surface
	result = eglMakeCurrent(display, surface, surface, context);
	assert(EGL_FALSE != result);
	
	setWindowSize(viewport.width, viewport.height);
	check();
}

void swapSurface(unsigned char *fb) {
    printf("%s\n", __func__);

	EGLint value;
 	GLsizei width;
 	GLsizei height;

	if (EGL_TRUE == eglQuerySurface(display, surface, EGL_WIDTH, &value)) {
		printf("width %d\n", value);
		width = value;
	} else {
		printf("error 0x%x querying width for display %p, surface %p\n",
		       eglGetError(), display, surface);
		return;
	}
	if (EGL_TRUE == eglQuerySurface(display, surface, EGL_HEIGHT, &value)) {
		printf("height %d\n", value);
		height = value;
	} else {
		printf("error 0x%x querying height for display %p, surface %p\n\n",
		       eglGetError(), display, surface);
		return;
	}
 
	if (EGL_TRUE == eglQuerySurface(display, surface, EGL_TEXTURE_FORMAT, &value)) {
		printf("format %d\n", value);
	} else {
		printf("error 0x%x querying format for display %p, surface %p\n",
		       eglGetError(), display, surface);
	}

	if (EGL_TRUE == eglQuerySurface(display, surface, EGL_TEXTURE_TARGET, &value)) {
		printf("type %d\n", value);
	} else {
		printf("error 0x%x querying format for display %p, surface %p\n",
		       eglGetError(), display, surface);
	}

	unsigned const num_bytes = 320*240*4;
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, fb);
 
	FILE *fout = fopen("pixels.rgba", "wb");
	fwrite(fb, 1, 320*240*4, fout);
	fclose(fout);

	eglSwapBuffers(display, surface);
}

void destroySurface() {
    printf("%s\n", __func__);
    eglSwapBuffers(display, surface);

    // Release OpenGL resources
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
}


int getKey() {
    int character;
    struct termios orig_term_attr;
    struct termios new_term_attr;

    /* set the terminal to raw mode */
    tcgetattr(fileno(stdin), &orig_term_attr);
    memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
    new_term_attr.c_lflag &= ~(ECHO|ICANON);
    new_term_attr.c_cc[VTIME] = 0;
    new_term_attr.c_cc[VMIN] = 0;
    tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

    /* read a character from the stdin stream without blocking */
    /*   returns EOF (-1) if no character is available */
    character = fgetc(stdin);

    /* restore the original terminal attributes */
    tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);

    return character;
}

void pollInput() {
    int key = getKey();
    if (key != -1 && key != keyPressed){
        onKeyPress(key);
    }
    keyPressed = key;
}

void setWindowSize(int _width, int _height) {
    printf("%s: %dx%d\n", __func__, _width, _height);
    viewport.width = _width;
    viewport.height = _height;
    glViewport((float)viewport.x, (float)viewport.y, (float)viewport.width, (float)viewport.height);
    onViewportResize(viewport.width, viewport.height);
}

int getWindowWidth(){
	return 240;
}

int getWindowHeight(){
	return 320;
}

unsigned char getKeyPressed(){
    return keyPressed;
}

void setRenderRequest(bool _render) {
    bRender = _render;
}

bool getRenderRequest() {
    return bRender;
}
