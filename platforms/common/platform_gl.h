#pragma once

#ifdef TANGRAM_ANDROID
#include <GLES2/gl2platform.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
// Values defined in androidPlatform.cpp
extern PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT;
extern PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT;
extern PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT;

#define glDeleteVertexArrays glDeleteVertexArraysOESEXT
#define glGenVertexArrays glGenVertexArraysOESEXT
#define glBindVertexArray glBindVertexArrayOESEXT
#endif // TANGRAM_ANDROID

#ifdef TANGRAM_IOS
#define GL_SILENCE_DEPRECATION
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#define glDeleteVertexArrays glDeleteVertexArraysOES
#define glGenVertexArrays glGenVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#endif // TANGRAM_IOS

#ifdef TANGRAM_OSX
#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
// Resolve aliased names in OS X
#define glClearDepthf glClearDepth
#define glDepthRangef glDepthRange
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#endif // TANGRAM_OSX

#ifdef TANGRAM_LINUX
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#endif // TANGRAM_LINUX

#ifdef TANGRAM_RPI
#include <unistd.h>
#include <string.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

// Dummy VertexArray functions
static void glBindVertexArray(GLuint array) {}
static void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) {}
static void glGenVertexArrays(GLsizei n, GLuint *arrays) {}
extern "C" {
GL_APICALL void *GL_APIENTRY glMapBuffer(GLenum target, GLenum access);
GL_APICALL GLboolean GL_APIENTRY glUnmapBuffer(GLenum target);
};

#endif // TANGRAM_RPI

#if defined(TANGRAM_ANDROID) || defined(TANGRAM_IOS)
    #define glMapBuffer glMapBufferOES
    #define glUnmapBuffer glUnmapBufferOES
#endif // defined(TANGRAM_ANDROID) || defined(TANGRAM_IOS) || defined(TANGRAM_RPI)
