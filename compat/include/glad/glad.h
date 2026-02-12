#pragma once

// Minimal glad header shim for compiling vendored game code without bundling OpenGL loaders.
// Only provides types, constants, and function prototypes referenced from headers.

#include <stddef.h>
#include <stdint.h>

#ifndef APIENTRY
#if defined(_WIN32)
#define APIENTRY __stdcall
#else
#define APIENTRY
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef int8_t GLbyte;
typedef uint8_t GLubyte;
typedef int16_t GLshort;
typedef uint16_t GLushort;
typedef int32_t GLint;
typedef uint32_t GLuint;
typedef int32_t GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

#define GL_FALSE 0
#define GL_TRUE 1

#define GL_FLOAT 0x1406
#define GL_UNSIGNED_BYTE 0x1401

#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4

#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_LINEAR 0x2601
#define GL_RGBA 0x1908

#define GL_DEPTH_TEST 0x0B71
#define GL_CULL_FACE 0x0B44

int gladLoadGL(void);

void APIENTRY glEnable(GLenum cap);
void APIENTRY glDisable(GLenum cap);

void APIENTRY glGenVertexArrays(GLsizei n, GLuint* arrays);
void APIENTRY glBindVertexArray(GLuint array);
void APIENTRY glDeleteVertexArrays(GLsizei n, const GLuint* arrays);

void APIENTRY glGenBuffers(GLsizei n, GLuint* buffers);
void APIENTRY glBindBuffer(GLenum target, GLuint buffer);
void APIENTRY glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
void APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers);

void APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
void APIENTRY glEnableVertexAttribArray(GLuint index);

void APIENTRY glGenTextures(GLsizei n, GLuint* textures);
void APIENTRY glBindTexture(GLenum target, GLuint texture);
void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param);
void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
void APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures);

#ifdef __cplusplus
} // extern "C"
#endif

