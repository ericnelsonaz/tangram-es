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
    int width = 320;
    int height = 240;
};
Viewport viewport;

#define MOUSE_ID "mouse0"
static int mouse_fd = -1;
struct Mouse {
    float x = 0.f;
    float y = 0.f;
    float velX = 0.f;
    float velY = 0.f;
    int button = 0;
};
static Mouse mouse;
static bool bRender;
static unsigned char keyPressed;

// Mouse stuff
//--------------------------------
std::string searchForDevice(const std::string& device) {
    std::ifstream file;
    std::string buffer;
    std::string address = "NONE";

    file.open("/proc/bus/input/devices");

    if (!file.is_open()) {
        return "NOT FOUND";
    }

    while (!file.eof()) {
        getline(file, buffer);
        std::size_t found = buffer.find(device);
        if (found != std::string::npos) {
            std::string tmp = buffer.substr(found + device.size() + 1);
            std::size_t foundBegining = tmp.find("event");
            if (foundBegining != std::string::npos) {
                address = "/dev/input/" + tmp.substr(foundBegining);
                address.erase(address.size() - 1, 1);
            }
            break;
        }
    }

    file.close();
    return address;
}

void closeMouse() {
    if (mouse_fd > 0) {
        close(mouse_fd);
    }
    mouse_fd = -1;
}

int initMouse() {
    closeMouse();

    mouse.x = viewport.width * 0.5;
    mouse.y = viewport.height * 0.5;
    std::string mouseAddress = searchForDevice(MOUSE_ID);
    std::cout << "Mouse [" << mouseAddress << "]" << std::endl;
    mouse_fd = open(mouseAddress.c_str(), O_RDONLY | O_NONBLOCK);

    return mouse_fd;
}

bool readMouseEvent(struct input_event *mousee) {
    int bytes;
    if (mouse_fd > 0) {
        bytes = read(mouse_fd, mousee, sizeof(struct input_event));
        if (bytes == -1) {
            return false;
        } else if (bytes == sizeof(struct input_event)) {
            return true;
        }
    }
    return false;
}

bool updateMouse() {
    if (mouse_fd < 0) {
        return false;
    }

    struct input_event mousee;
    while (readMouseEvent(&mousee)) {

        mouse.velX = 0;
        mouse.velY = 0;

        float x = 0.0f, y = 0.0f;
        int button = 0;

        switch (mousee.type) {
            // Update Mouse Event
            case EV_KEY:
                switch (mousee.code) {
                    case BTN_LEFT:
                        if (mousee.value == 1) {
                            button = 1;
                        }
                        break;
                    case BTN_RIGHT:
                        if (mousee.value == 1) {
                            button = 2;
                        }
                        break;
                    case BTN_MIDDLE:
                        if (mousee.value == 1) {
                            button = 3;
                        }
                        break;
                    default:
                        button = 0;
                        break;
                }
                if (button != mouse.button) {
                    mouse.button = button;
                    if (mouse.button == 0) {
                        onMouseRelease(mouse.x, mouse.y);
                    } else {
                        onMouseClick(mouse.x, mouse.y, mouse.button);
                    }
                }
                break;
            case EV_REL:
                switch (mousee.code) {
                    case REL_X:
                        mouse.velX = mousee.value;
                        break;
                    case REL_Y:
                        mousee.value = mousee.value * -1;
                        mouse.velY = mousee.value;
                        break;
                    // case REL_WHEEL:
                    //     if (mousee.value > 0)
                    //         std::cout << "Mouse wheel Forward" << std::endl;
                    //     else if(mousee.value < 0)
                    //         std::cout << "Mouse wheel Backward" << std::endl;
                    //     break;
                    default:
                        break;
                }
                mouse.x += mouse.velX;
                mouse.y += mouse.velY;

                // Clamp values
                if (mouse.x < 0) { mouse.x = 0; }
                if (mouse.y < 0) { mouse.y = 0; }
                if (mouse.x > viewport.width) { mouse.x = viewport.width; }
                if (mouse.y > viewport.height) { mouse.y = viewport.height; }

                if (mouse.button != 0) {
                    onMouseDrag(mouse.x, mouse.y, mouse.button);
                } else {
                    onMouseMove(mouse.x, mouse.y);
                }
                break;
            case EV_ABS:
                switch (mousee.code) {
                    case ABS_X:
                        x = ((float)mousee.value / 4095.0f) * viewport.width;
                        mouse.velX = x - mouse.x;
                        mouse.x = x;
                        break;
                    case ABS_Y:
                        y = (1.0 - ((float)mousee.value / 4095.0f)) * viewport.height;
                        mouse.velY = y - mouse.y;
                        mouse.y = y;
                        break;
                    default:
                        break;
                }
                if (mouse.button != 0) {
                    onMouseDrag(mouse.x, mouse.y, mouse.button);
                } else {
                    onMouseMove(mouse.x, mouse.y);
                }
                break;
            default:
                break;
        }
    }
    return true;
}


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
	viewport.width = 320;
	viewport.height = 240;

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
	
	initMouse();
}

void swapSurface() {
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
	unsigned char *data = new unsigned char [320*240*4];
	memset(data, 0, num_bytes);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
 
	FILE *fout = fopen("pixels.rgba", "wb");
	fwrite(data, 1, 320*240*4, fout);
	fclose(fout);

	eglSwapBuffers(display, surface);

	delete [] data;
}

void destroySurface() {
    printf("%s\n", __func__);
    closeMouse();
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
    updateMouse();

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
	return 320;
}

int getWindowHeight(){
	return 240;
}

float getMouseX(){
    return mouse.x;
}

float getMouseY(){
    return mouse.y;
}

float getMouseVelX(){
    return mouse.velX;
}

float getMouseVelY(){
    return mouse.velY;
}

int getMouseButton(){
    return mouse.button;
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
