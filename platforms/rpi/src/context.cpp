#include "context.h"
#include "platform.h"

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <fstream>
#include <termios.h>
#include <linux/input.h>


// Main global variables
//-------------------------------
#define check() assert(glGetError() == 0)
EGLDisplay display;
EGLSurface surface;
EGLContext context;

struct Viewport {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
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

}

void swapSurface() {
    eglSwapBuffers(display, surface);
}

void destroySurface() {
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
    viewport.width = _width;
    viewport.height = _height;
    glViewport((float)viewport.x, (float)viewport.y, (float)viewport.width, (float)viewport.height);
    onViewportResize(viewport.width, viewport.height);
}

int getWindowWidth(){
    return viewport.width;
}

int getWindowHeight(){
    return viewport.height;
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
